// Read-only paired cloud timings. Writes only a fresh report supplied by caller.
// Camera-route cost is not ship gameplay or visual acceptance.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');

const read = (root, name) => fs.readFileSync(path.join(root, name), 'utf8').replace(/^\uFEFF/, '');
const json = (root, name) => JSON.parse(read(root, name));
function canonicalCommand(command) {
    return command.trim()
        .replace(/ -(?:UserDir|abslog|ReportExportPath)="[^"]*"/gi, '')
        .replace(/aps\.Surface\.Clouds [01]/g, 'aps.Surface.Clouds ON_OFF')
        .replace(/\+APS\.Gameplay\.World\.PlanetSurface\.Clouds(?=["\s])/g, '');
}
function percentile(values, p) {
    const sorted = values.filter(x => Number.isFinite(x) && x >= 0).sort((a,b) => a-b);
    return sorted.length ? sorted[Math.max(0, Math.ceil(sorted.length*p)-1)] : null;
}
function sameCloudAsset(off, on) {
    for (const asset of [off,on]) {
        assert(asset.ObjectPath && asset.Path && /^[A-Fa-f0-9]{64}$/.test(asset.Hash), 'Incomplete baked cloud identity');
    }
    assert.equal(off.ObjectPath,on.ObjectPath,'Different cloud material object paths');
    assert.equal(off.Hash.toUpperCase(),on.Hash.toUpperCase(),'Cloud material changed between ON/OFF runs');
}
function routeLayers(log, required) {
    const lines=log.split(/\r?\n/).filter(line => line.includes('[APS.CloudRouteLayers]'));
    if (!lines.length) { assert(!required,'Layered timing run lacks post-route layer bounds'); return null; }
    assert.equal(lines.length,1,'Ambiguous route layer metadata');
    const match=lines[0].match(/\[APS\.CloudRouteLayers\] layered=([01]) boundsKm=(\S+)(?:\s|$)/);
    assert(match,'Malformed route layer metadata');
    const bounds=match[2].split(';').map(pair => {
        const cells=pair.split(',');
        assert(cells.length===2 && cells.every(cell => cell.length),'Incomplete cloud deck bounds');
        const values=cells.map(Number);
        assert(values.every(Number.isFinite) && values[0]<values[1],'Invalid cloud deck bounds');
        return values;
    });
    assert.equal(bounds.length,match[1]==='1'?3:1,'Route layer count differs from its mode');
    for(let i=1;i<bounds.length;i++) assert(bounds[i][0]>=bounds[i-1][1],'Unsorted or overlapping cloud decks');
    return {layered:match[1]==='1',bounds};
}
function cloudRegions(bounds) {
    return {all:r => true, above:r => r.height_km>bounds.at(-1)[1],
        inside:r => bounds.some(([bottom,top]) => r.height_km>=bottom && r.height_km<=top),
        below:r => r.height_km<bounds[0][0]};
}
function frames(text) {
    const [header, ...lines] = text.trim().split(/\r?\n/);
    const keys = header.split(',');
    for (const key of ['frame','t','height_km','dt_ms','wall_ms','game_ms','render_ms','gpu_ms'])
        assert(keys.includes(key), 'Missing CSV column '+key);
    const rows = lines.map(line => {
        const cells = line.split(',');
        assert.equal(cells.length, keys.length, 'Incomplete frame CSV');
        const row = Object.fromEntries(keys.map((key,i) => [key, Number(cells[i])]));
        assert(Object.values(row).every(Number.isFinite), 'Non-finite frame CSV');
        return row;
    });
    for (let i=1; i<rows.length; i++)
        assert(rows[i].frame > rows[i-1].frame && rows[i].t >= rows[i-1].t, 'Duplicated or reversed frame/time');
    assert(rows.length >= 100, 'Incomplete route');
    return rows;
}
function load(root, enabled) {
    const command = read(root, 'command.txt');
    assert(command.includes('-APSProbeCloudFlightPerf'), 'Screenshots/logging route is not a timing run');
    assert(command.includes('aps.Surface.Clouds '+Number(enabled)+','), 'ON/OFF not explicitly selected');
    assert(!command.includes('-APSCloudDiagnostics'), 'Verbose cloud logging changes timing');
    const report = json(root, 'report/index.json');
    assert(report.tests.some(t => t.fullTestPath === 'APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics'
        && t.state === 'Success'), 'Rendered route did not complete successfully');
    assert(!report.failed && !report.notRun && !report.inProcess, 'Automation suite is not fully successful');
    const log = read(root, 'gameplay.log');
    assert(/\[APS\.CloudFlight\] frames=\d+ above=1 inside=1 below=1 safety=1 perf=1/.test(log), 'Missing completed cloud route');
    const dll = json(root, 'dll.json'), native = json(root, 'native-dll.json');
    assert(dll.Hash && native.Hash, 'Missing build identity');
    const asset = json(root, 'cloud-asset.json');
    const currentHash = crypto.createHash('sha256').update(fs.readFileSync(asset.Path)).digest('hex');
    assert.equal(currentHash,asset.Hash.toLowerCase(),'Baked cloud asset changed after capture');
    const device = report.devices[0];
    return {root, command, dll:dll.Hash, nativeDll:native.Hash,
        device:JSON.stringify([device.gPU,device.cPUModel,device.renderMode,device.rHI]),
        asset, log, rows:frames(read(root, 'Saved/Diagnostics/CloudFlight.csv'))};
}
function metrics(rows) {
    assert(rows.length >= 20, 'Height region undersampled');
    const value = key => rows.map(r => r[key]);
    return {frames:rows.length, fromT:rows[0].t, toT:rows.at(-1).t,
        frameP50:percentile(value('dt_ms'),.5), frameP95:percentile(value('dt_ms'),.95),
        frameP99:percentile(value('dt_ms'),.99), worstFrame:percentile(value('dt_ms'),1),
        wallP99:percentile(value('wall_ms'),.99), gameP50:percentile(value('game_ms'),.5),
        renderP50:percentile(value('render_ms'),.5), gpuP50:percentile(value('gpu_ms'),.5),
        gpuP95:percentile(value('gpu_ms'),.95), over33ms:rows.filter(r => r.dt_ms > 1000/30).length,
        over50ms:rows.filter(r => r.dt_ms > 50).length};
}
if (process.argv[2] === '--self-test') {
    assert.equal(percentile([-1,1,2,3,4],.5),2);
    assert.equal(percentile([-1],.99),null);
    assert.equal(canonicalCommand('-x -UserDir="off" aps.Surface.Clouds 0, Automation X"'),
        canonicalCommand('-x -UserDir="on" aps.Surface.Clouds 1, Automation X"'));
    assert.notEqual(canonicalCommand('-ResX=1600'),canonicalCommand('-ResX=1920'));
    assert.throws(() => frames('frame,t,height_km\n1,0,1'));
    const asset={ObjectPath:'/Game/Cloud.Cloud',Path:'cloud.uasset',Hash:'A'.repeat(64)};
    sameCloudAsset(asset,{...asset,Hash:asset.Hash.toLowerCase()});
    assert.throws(() => sameCloudAsset(asset,{...asset,Hash:'B'.repeat(64)}));
    assert.throws(() => sameCloudAsset(asset,{...asset,ObjectPath:'/Game/Other.Other'}));
    assert.throws(() => sameCloudAsset(asset,{...asset,Hash:''}));
    const metadata=bounds => '[APS.CloudRouteLayers] layered=1 boundsKm='+bounds;
    const decks=routeLayers(metadata('3,5;6,9;10,11'),true);
    assert.deepEqual(routeLayers('[APS.CloudRouteLayers] layered=0 boundsKm=3,5',true),{layered:false,bounds:[[3,5]]});
    assert.equal(routeLayers('',false),null);
    assert.throws(() => routeLayers('',true));
    for(const bad of ['NaN,5;6,9;10,11','5,3;6,9;10,11','3,7;6,9;10,11','6,9;3,5;10,11'])
        assert.throws(() => routeLayers(metadata(bad),true));
    const regions=cloudRegions(decks.bounds), at=height_km => ({height_km});
    assert(regions.inside(at(7)) && regions.inside(at(10.5)) && !regions.above(at(10.5)));
    assert(!regions.inside(at(5.5)) && !regions.above(at(5.5)) && !regions.below(at(5.5)));
    assert(regions.above(at(12)) && regions.below(at(2)));
    assert.throws(() => assert.deepEqual(decks,routeLayers(metadata('3,5;6,9;10,12'),true),'OFF/ON route layer bounds differ'));
    console.log('Cloud timing summary self-test PASS');
} else {
    const [offRoot, onRoot, output] = process.argv.slice(2);
    assert(offRoot && onRoot && output, 'Usage: node SummarizePlanetCloudPair.cjs offRun onRun freshOutput.json');
    const off=load(offRoot,false), on=load(onRoot,true);
    assert.equal(off.dll,on.dll,'Different project DLLs');
    assert.equal(off.nativeDll,on.nativeDll,'Different WorldScape DLLs');
    assert.equal(off.device,on.device,'Different rendering device');
    assert.equal(canonicalCommand(off.command),canonicalCommand(on.command),'Not a matched ON/OFF route');
    sameCloudAsset(off.asset,on.asset);
    const layer=on.log.match(/\[APS\.Clouds\][^\n]* bottom=(\S+) thickness=(\S+)[^\n]* parent=(\S+)/);
    assert(layer,'Missing actual cloud layer/binding');
    assert.equal(layer[3],on.asset.ObjectPath,'Rendered parent differs from captured baked asset');
    const offLayers=routeLayers(off.log,/(?:^|\s)-APSCloudLayeredCandidate(?:\s|$)/.test(off.command));
    const onLayers=routeLayers(on.log,/(?:^|\s)-APSCloudLayeredCandidate(?:\s|$)/.test(on.command));
    assert.deepEqual(offLayers,onLayers,'OFF/ON route layer bounds differ');
    const bounds=onLayers ? onLayers.bounds : [[Number(layer[1]),Number(layer[1])+Number(layer[2])]];
    const bottom=bounds[0][0], top=bounds.at(-1)[1], regions=cloudRegions(bounds);
    const delta=(a,b) => a === null || b === null ? null : b-a;
    const summaries=Object.entries(regions).map(([region,filter]) => {
        // Exclude the first 2 seconds equally; retain hitches after warmup.
        const a=metrics(off.rows.filter(r => r.t>=2 && filter(r)));
        const b=metrics(on.rows.filter(r => r.t>=2 && filter(r)));
        return {region,off:a,on:b,deltaMs:{gpuP50:delta(a.gpuP50,b.gpuP50),
            gpuP95:delta(a.gpuP95,b.gpuP95),frameP95:delta(a.frameP95,b.frameP95)}};
    });
    const result={off:offRoot,on:onRoot,dll:on.dll,nativeDll:on.nativeDll,
        material:layer[3],materialHash:on.asset.Hash,layerBottomKm:bottom,layerTopKm:top,summaries,
        routeLayers:onLayers,interLayerGapFrames:Object.fromEntries([['off',off],['on',on]].map(([name,run]) =>
            [name,run.rows.filter(r => r.t>=2 && r.height_km>=bottom && r.height_km<=top && !regions.inside(r)).length])),
        limitations:'Matched camera route only; no VRAM, no ship dynamics, no visual acceptance. Negative GPU deltas are measurements, not claimed speedups. No automatic pass budget.'};
    fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n',{flag:'wx'});
    console.log(JSON.stringify(result));
}
