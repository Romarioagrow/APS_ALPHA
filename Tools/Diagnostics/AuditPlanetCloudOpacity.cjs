// CPU normal-incidence cloud-column audit. Not GPU, sky/depth or visual proof.
// Compare the SAME world columns at every footprint; do not confuse conserved
// mean density with conserved Beer-Lambert opacity through the whole layer.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const assert=require('node:assert/strict'),crypto=require('node:crypto');
const project=path.resolve(__dirname,'../..');
const modelFile=path.join(__dirname,'AuditPlanetCloudDensity.cjs');
const text=fs.readFileSync(modelFile,'utf8');
assert.equal(text.split('let rng = 19686;').length,2,'Density model boundary changed');
const context={require,__dirname,console,process};
vm.runInNewContext(text.split('let rng = 19686;')[0]
    +'\nglobalThis.model={noise,filter,rot,add,mul};',context);
const {noise,filter,rot,add,mul}=context.model;
const hlsl=fs.readFileSync(path.join(project,'Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h'),'utf8');
assert(hlsl.includes('float SamplePhase=.05+.90*float(Phase&65535u)/65535.;'));
assert(hlsl.includes('const float ExtinctionPerKm=8.*max(CloudDensity,0.);'));
// Controlled fixture: retain the current runtime density multiplier explicitly.
const modelDensity=1,extinctionPerKm=8*Math.max(modelDensity,0);
const seed=[13,24,35],radius=6750,thickness=2.212,bottom=6;
let randomSeed=27183;
const random=()=>{randomSeed=(Math.imul(randomSeed,1664525)+1013904223)>>>0;return randomSeed/4294967296;};
// Same flat local normal-incidence columns, no spherical clipping or scene depth.
// Positions span many noise cells; the constant weather envelope is controlled.
const points=Array.from({length:1024},()=>[random()*2000,random()*2000]);
function alpha(x,y,phase,footprint,weatherBody){
    const growth=Math.pow(3,1/16);
    let step=thickness*.5*(growth-1),start=0,transmission=1;
    for(let i=0;i<16;i++){
        const at=start+phase*step,h=1-at/thickness;
        const p=[x,y,radius+bottom+thickness-at];
        const threshold=.68+(.43-.68)*weatherBody
            +.48*Math.pow(Math.abs((h-.46)/(h>=.46?.54:.46)),2);
        const coarse=noise(add(mul(p,.32),seed));
        const fine=noise(add(rot(mul(p,.93)),seed));
        const density=filter(coarse,fine,Math.max(footprint,step*.5),threshold);
        transmission*=Math.exp(-density*step*extinctionPerKm);
        start+=step;step*=growth;
    }
    return 1-transmission;
}
const rows=[];
for(const weatherBody of [.35,.6,1]){
    let reference=0;
    for(const [x,y] of points) reference+=alpha(x,y,.5,0,weatherBody)/points.length;
    for(const footprintKm of [0,.5,1,2,4]){
        let sum=0,phaseError=0,phaseMax=0;
        for(const [x,y] of points){
            const values=[.05,.5,.95].map(phase=>alpha(x,y,phase,footprintKm,weatherBody));
            const mean=values.reduce((a,b)=>a+b)/values.length;
            sum+=mean;
            for(const v of values){phaseError+=(v-mean)**2;phaseMax=Math.max(phaseMax,Math.abs(v-mean));}
        }
        const meanAlpha=sum/points.length;
        rows.push({weatherBody,footprintKm,referenceMeanAlpha:reference,meanAlpha,
            meanAlphaDelta:meanAlpha-reference,phaseRms:Math.sqrt(phaseError/(3*points.length)),phaseMax});
    }
}
const report={scope:'CPU controlled column only; no GPU, weather map, spherical/depth clipping, sky or rendered acceptance',
    samples:points.length,modelDensity,extinctionPerKm,sourceHash:crypto.createHash('sha256').update(hlsl).digest('hex'),rows};
if(process.argv[2])fs.writeFileSync(process.argv[2],JSON.stringify(report,null,2)+'\n',{flag:'wx'});
console.log(JSON.stringify(report));
