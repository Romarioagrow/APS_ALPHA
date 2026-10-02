// CPU model audit only: does not prove GPU binding, cloud appearance or cost.
const fs = require('node:fs');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const path = require('node:path');
const project = path.resolve(__dirname, '../..');
const source = fs.readFileSync(path.join(project, 'Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h'), 'utf8');
assert(source.includes('float filteredDensity('), 'density-filter candidate is not installed');
assert(source.includes('float sigma=.135*sqrt('), 'audit requires the calibrated .135 single octave sigma');
assert(source.includes('float coarseWeight=1.-smoothstep(.25,.5,footprint*.32);'), 'coarse octave cutoff differs from CPU model');
assert(source.includes('float fineWeight=1.-smoothstep(.25,.5,footprint*.93);'), 'fine octave cutoff differs from CPU model');
assert(source.includes('float BillowFrequencyScale=rcp(clamp(WeatherScale,.25,3.));'), 'weather size must drive physical billows');
assert(source.includes('p*=frequencyScale;') && source.includes('footprint*=frequencyScale;'), 'domain and footprint must scale together');
for (const call of [
    'F.filteredDensity(F.advect(P,WindRotation.xy),SeedOffset,Footprint,BodyThreshold,BillowFrequencyScale)',
    'F.filteredDensity(F.advect(LP,WindRotation.xy),SeedOffset,Footprint,LT,BillowFrequencyScale)'])
    assert(source.includes(call), 'camera and light must sample the same scaled field');

const sat=x=>Math.max(0,Math.min(1,x)), mix=(a,b,t)=>a+(b-a)*t;
const add=(a,b)=>a.map((v,i)=>v+b[i]), mul=(a,s)=>a.map(v=>v*s), dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0);
const len=a=>Math.sqrt(dot(a,a)), norm=a=>mul(a,1/len(a));
const rot=p=>[.8*p[1]+.6*p[2],-.8*p[0]+.36*p[1]-.48*p[2],-.6*p[0]-.48*p[1]+.64*p[2]];
function gradient(c,d){let h=Math.imul(c[0],1597334677)^Math.imul(c[1],3812015801)^Math.imul(c[2],2798796415); h^=h>>>16;h=Math.imul(h,2246822519);h^=h>>>13;h&=15;const u=h<8?d[0]:d[1],v=h<4?d[1]:(h===12||h===14?d[0]:d[2]);return ((h&1)?-u:u)+((h&2)?-v:v);}
function noise(p){const i=p.map(Math.floor),d=p.map((v,k)=>v-i[k]),f=d.map(v=>v*v*v*(v*(v*6-15)+10));let result=0;for(let z=0;z<2;z++)for(let y=0;y<2;y++)for(let x=0;x<2;x++){result+=gradient([i[0]+x,i[1]+y,i[2]+z],[d[0]-x,d[1]-y,d[2]-z])*(x?f[0]:1-f[0])*(y?f[1]:1-f[1])*(z?f[2]:1-f[2]);}return .5+.5*result;}
const smooth=(a,b,x)=>{const t=sat((x-a)/(b-a));return t*t*(3-2*t);};
function billow(p,seed,footprint=0){const fw=1-smooth(.25,.5,footprint*.93);return .72*noise(add(mul(p,.32),seed))+.28*mix(.5,fw>.001?noise(add(rot(mul(p,.93)),seed)):.5,fw);}
const oldMean=t=>.25*sat((.4-t)*7)+.5*sat((.5-t)*7)+.25*sat((.6-t)*7);

function normalCDF(z){const t=1/(1+.2316419*Math.abs(z)),d=.3989422804*Math.exp(-.5*z*z);const tail=d*t*(.319381530+t*(-.356563782+t*(1.781477937+t*(-1.821255978+t*1.330274429))));return z>=0?1-tail:tail;}
function correctedMean(threshold){const sigma=.1043,a=(threshold-.5)/sigma,b=(threshold+1/7-.5)/sigma;return sat(7*((.5-threshold)*(normalCDF(b)-normalCDF(a))+sigma*.3989422804*(Math.exp(-.5*a*a)-Math.exp(-.5*b*b)))+1-normalCDF(b));}

function expected(mean, sigma, threshold) {
    if (sigma < .001) return sat((mean - threshold) * 7);
    const a = (threshold - mean) / sigma, b = (threshold + 1/7 - mean) / sigma;
    const ca = normalCDF(a), cb = normalCDF(b);
    return sat(7 * ((mean - threshold) * (cb - ca)
        + sigma * .3989422804 * (Math.exp(-.5*a*a) - Math.exp(-.5*b*b))) + 1 - cb);
}
const octaveWeight = (footprint, frequency) => 1 - smooth(.25, .5, footprint * frequency);
for (const frequency of [.32, .93]) {
    assert.equal(octaveWeight(0, frequency), 1, 'zero physical footprint must retain the original octave');
    for (const domainFootprint of [.5, .75, 1, 2, 100])
        assert.equal(octaveWeight(domainFootprint / frequency, frequency), 0, 'unresolved octave must vanish at and above .5');
}
function filter(coarse, fine, footprint, threshold) {
    const cw = octaveWeight(footprint, .32);
    const fw = octaveWeight(footprint, .93);
    const mean = .5 + .72*cw*(coarse-.5) + .28*fw*(fine-.5);
    const sigma = .135*Math.sqrt(Math.max(0, .5184*(1-cw*cw)+.0784*(1-fw*fw)));
    return expected(mean, sigma, threshold);
}
// CPU algebra of the source contract above, NOT execution of the HLSL/GPU.
const frequencyScale = size => 1 / Math.max(.25, Math.min(3, size));
function physicalDensity(p, seed, footprint, threshold, size) {
    const scale = frequencyScale(size), q = mul(p, scale);
    return filter(noise(add(mul(q,.32),seed)), noise(add(rot(mul(q,.93)),seed)),
        footprint*scale, threshold);
}
let maxScaleError = 0, scaleChecks = 0;
for (const p of [[0,0,0],[17.2,-38.1,6750.3],[-4590.7,68.2,97.4]])
    for (const seed of [[0,0,0],[13,24,35]])
        for (const footprint of [0,.05,.5,1,2,8])
            for (const threshold of [.43,.60,.68]) {
                const baseline=filter(noise(add(mul(p,.32),seed)),noise(add(rot(mul(p,.93)),seed)),footprint,threshold);
                assert.equal(physicalDensity(p,seed,footprint,threshold,1),baseline,'default field must remain identical');
                for (const size of [.25,.5,1,1.67,2,3]) {
                    // Enlarging the physical world and pixel together must give
                    // the same filtered field, including its nonlinear threshold.
                    const density=physicalDensity(mul(p,size),seed,footprint*size,threshold,size);
                    const error=Math.abs(density-baseline);
                    assert(error<1e-9,'feature size changed filtering/seed instead of spatial scale');
                    maxScaleError=Math.max(maxScaleError,error); scaleChecks++;
                }
                assert.equal(physicalDensity(p,seed,footprint,threshold,.01),physicalDensity(p,seed,footprint,threshold,.25));
                assert.equal(physicalDensity(p,seed,footprint,threshold,99),physicalDensity(p,seed,footprint,threshold,3));
            }
let rng = 19686;
const random = () => { rng=(Math.imul(rng,1664525)+1013904223)>>>0; return rng/4294967296; };
const count = 50000, seed=[13,24,35], samples=[];
for (let i=0; i<count; ++i) {
    const p=[random()*1000,random()*1000,random()*1000];
    samples.push([noise(add(mul(p,.32),seed)), noise(add(rot(mul(p,.93)),seed))]);
}
const footprints=[0,.5,1,2,4,8,32];
const rows=[.43,.48,.53,.55,.57,.59,.6,.62,.65,.68].map(threshold=>{
    const reference=samples.reduce((s,[c,f])=>s+sat((.72*c+.28*f-threshold)*7),0)/count;
    const filtered=footprints.map(footprint=>{
        const mean=samples.reduce((s,[c,f])=>s+filter(c,f,footprint,threshold),0)/count;
        assert(Math.abs(mean-reference)<.015, 'filtered statistical density changed by more than .015');
        return {footprintKm:footprint,mean,absoluteError:Math.abs(mean-reference)};
    });
    return {threshold,reference,oldFar:oldMean(threshold),filtered};
});
for(const [c,f] of samples.slice(0,1000)) for(const t of [.43,.6,.68]) {
    assert(Math.abs(filter(c,f,0,t)-sat((.72*c+.28*f-t)*7))<1e-12, 'resolved field changed');
}
for(const sigma of [0,.0005,.005,.04,.1043]) {
    let previous=1;
    for(let i=-10;i<=110;i++){
        const density=expected(.5,sigma,i*.01);
        assert(Number.isFinite(density)&&density>=0&&density<=1&&density<=previous+1e-7);
        previous=density;
    }
}
const report={
    scope:'CPU statistical density filter only; not GPU/render/performance acceptance',
    sourceSha256:crypto.createHash('sha256').update(source).digest('hex'),
    samples:count,footprintsKm:footprints,
    featureScale:{scaleChecks,maxScaleError,defaultFieldIdentical:true,cameraLightScaleShared:true},
    resolvedFieldUnchanged:true,unresolvedOctavesRemoved:true,cutoffDomainFootprint:.5,monotoneAndBounded:true,rows
};
if(process.argv[2])fs.writeFileSync(process.argv[2],JSON.stringify(report,null,2),{flag:'wx'});
console.log(JSON.stringify({samples:count,resolvedFieldUnchanged:true,unresolvedOctavesRemoved:true,cutoffDomainFootprint:.5,monotoneAndBounded:true,
    featureScale:report.featureScale,
    maxMeanError:Math.max(...rows.flatMap(r=>r.filtered.map(v=>v.absoluteError))),
    tails:rows.filter(r=>r.threshold>=.6).map(r=>({threshold:r.threshold,reference:r.reference,old:r.oldFar,corrected:r.filtered.at(-1).mean}))}));
