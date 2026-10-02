// CPU/source audit only; does NOT establish UHT, shader compilation, appearance or FPS.
const fs=require('node:fs'), path=require('node:path'), assert=require('node:assert/strict'), crypto=require('node:crypto');
const root=path.resolve(__dirname,'../..');
const read=p=>fs.readFileSync(path.join(root,p),'utf8');
const shader=read('Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h');
// TEXT expands to UTF-16 on Windows. Keep a margin below MSVC's literal
// limit so shader edits cannot reintroduce C2026 before the Unreal build.
const hlslParts=[...shader.matchAll(/TEXT\(R"HLSL\(([\s\S]*?)\)HLSL"\)/g)];
assert(hlslParts.length>0,'cloud HLSL literals not found');
const hlslLiteralBytes=hlslParts.map(part=>Buffer.byteLength(part[1],'utf16le')+2);
for(const [index,bytes] of hlslLiteralBytes.entries())
 assert(bytes<=16000,`cloud HLSL literal ${index+1} exceeds safe MSVC budget: ${bytes} bytes; split the literal (C2026)`);
const settings=read('Source/APS_ALPHA/Core/Planetary/APSPlanetCloudSettings.h');
const names=['CoverageScale','DensityScale','FeatureScale','AltitudeScale','WindScale','StormScale','SeedOffset'];
for(const n of names) assert(settings.includes(n),'missing reflected setting '+n);
assert.equal((settings.match(/UPROPERTY/g)||[]).length,7);
for(const f of ['Core/Model/GeneratedWorld.h','Core/Saves/GeneratedWorldData.h','Core/Structs/PlanetGenerationModel.h',
 'Core/Structs/MoonGenerationModel.h','Actors/Astro/PlanetaryBody.h'])
 assert(read('Source/APS_ALPHA/'+f).includes('FAPSPlanetCloudSettings CloudSettings'),'missing model link '+f);
for(const f of ['Generation/AstroGeneratorBodyOverrides.cpp','Generation/PlanetGenerator.cpp',
 'Generation/MoonGenerator.cpp','Core/Saves/APSWorldSaveSnapshot.cpp'])
 assert(read('Source/APS_ALPHA/'+f).includes('CloudSettings'),'missing replay link '+f);
const vm=read('Source/APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.cpp');
const cloudOnly=vm.slice(vm.indexOf('void UWorldGenerationViewModel::RefreshCloudsPreview()'),vm.indexOf('void UWorldGenerationViewModel::ExecutePlanetAppearancePreviewRefresh()'));
for(const unwanted of ['RequestPreview(', 'RefreshPlanetAppearancePreview(', 'RefreshPreviewPlanetAppearance(', 'Atmosphere->'])
 assert(!cloudOnly.includes(unwanted),'cloud drag mutates non-cloud path '+unwanted);
assert(cloudOnly.includes('if(!Timers.IsTimerActive(CloudAppearanceTimerHandle))'),
 'continuous cloud slider drag must not keep postponing the pending refresh');
for(const required of ['float weather(', 'n=vortex(', 'F.advect(P,WindRotation.xy)', 'F.advect(LP,WindRotation.xy)',
 'Contribution*Light*Albedo', '8.*max(CloudDensity,0.)','16.,32.'])assert(shader.includes(required),'shader contract missing '+required);
const flight=read('Tools/Diagnostics/RunPlanetFieldsFlight.ps1');
const menu=read('Tools/Diagnostics/RunPlanetMenuContinuity.ps1');
for(const runner of [flight,menu])assert(runner.includes('-ini:Engine:[SystemSettings]:aps.Surface.CloudWeather=1'),
 'weather mode must be active before body creation, not only delayed ExecCmds');
assert(flight.includes('if($CloudWeather -and (!$CloudFlight -or $CloudDefault))'), 'weather flight needs a matched OFF leg');
assert(menu.includes('!($Clouds -xor $CloudOff)'), 'weather menu needs explicit ON/OFF, not ambiguous default');
const probe=read('Source/APS_ALPHA/Tests/APSCloudFlightProbe.h');
for(const token of ['GetScalarParameterValue','GetVectorParameterValue','P->CloudSettings=SavedSettings',
 '!Performance() && !LastCounter','Clear-sky coverage retained a cloud owner'])
 assert(probe.includes(token),'missing deferred runtime binding/lifecycle assertion '+token);

const sat=x=>Math.max(0,Math.min(1,x)), mix=(a,b,t)=>a+(b-a)*t;
const add=(a,b)=>a.map((v,i)=>v+b[i]), mul=(a,s)=>a.map(v=>v*s), dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0);
const len=a=>Math.sqrt(dot(a,a)), norm=a=>mul(a,1/len(a)), cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
const rot=p=>[.8*p[1]+.6*p[2],-.8*p[0]+.36*p[1]-.48*p[2],-.6*p[0]-.48*p[1]+.64*p[2]];
const smooth=(a,b,x)=>{const t=sat((x-a)/(b-a));return t*t*(3-2*t);};
function gradient(c,d){let h=Math.imul(c[0],1597334677)^Math.imul(c[1],3812015801)^Math.imul(c[2],2798796415);h^=h>>>16;h=Math.imul(h,2246822519);h^=h>>>13;h&=15;const u=h<8?d[0]:d[1],v=h<4?d[1]:(h===12||h===14?d[0]:d[2]);return ((h&1)?-u:u)+((h&2)?-v:v);}
function noise(p){const i=p.map(Math.floor),d=p.map((v,k)=>v-i[k]),f=d.map(v=>v*v*v*(v*(v*6-15)+10));let r=0;for(let z=0;z<2;z++)for(let y=0;y<2;y++)for(let x=0;x<2;x++)r+=gradient([i[0]+x,i[1]+y,i[2]+z],[d[0]-x,d[1]-y,d[2]-z])*(x?f[0]:1-f[0])*(y?f[1]:1-f[1])*(z?f[2]:1-f[2]);return .5+.5*r;}
function vortex(n,a,k){const w=1-smooth(0,.20,1-dot(n,a)),angle=k*w*w,s=Math.sin(angle),c=Math.cos(angle);return add(add(mul(n,c),mul(cross(a,n),s)),mul(a,dot(a,n)*(1-c)));}
function weather(n,seed,size,band,swirl){
 const axis=norm([.7+seed[0]*.006,.4+seed[1]*.006,.65]);
 n=vortex(n,axis,3.5*swirl);n=vortex(n,norm([-axis[1],axis[0],-.6]),-2.8*swirl);
 const freq=5/Math.max(size,.25),warp=noise(add(rot(mul(n,2.7)),seed));
 let q=add(add(mul(n,freq),[1.8*n[2]*n[2],1.1*n[0]*n[2],0]),seed);
 q=add(q,[warp-.5,(warp-.5)*.7,0]);
 return mix(noise(q),noise([q[0]*.45,q[1]*.45,q[2]*2.3]),sat(band)*.65)*.88+noise(add(mul(rot(q),2.1),[17,3,41]))*.12;
}
const seed=[13,24,35],axis=norm([.7,.4,.65]),count=4096;
let maxRadiusError=0,swirlDelta=0,min=1,max=0;
for(let i=0;i<count;i++){
 const z=1-2*(i+.5)/count,a=i*Math.PI*(3-Math.sqrt(5)),r=Math.sqrt(1-z*z),n=[r*Math.cos(a),r*Math.sin(a),z];
 assert(len(add(vortex(n,axis,0),mul(n,-1)))<1e-12);
 const turned=vortex(n,axis,7);maxRadiusError=Math.max(maxRadiusError,Math.abs(len(turned)-1));
 const w=weather(n,seed,1,.25,.7),quiet=weather(n,seed,1,.25,0);
 assert(Number.isFinite(w));min=Math.min(min,w);max=Math.max(max,w);swirlDelta+=Math.abs(w-quiet);
 let previous=0;
 for(let j=0;j<=20;j++){const t=mix(.80,.18,j/20),shape=smooth(t-.10,t+.18,w);assert(shape>=previous-1e-12);previous=shape;}
}
assert(maxRadiusError<1e-12);assert(swirlDelta/count>1e-4);
let maxSeamDelta=0;
for(const z of [-.999,-.5,0,.5,.999]){
 const x=Math.sqrt(1-z*z);
 const a=norm([-x,1e-8,z]),b=norm([-x,-1e-8,z]);
 for(const size of [.25,1,3])maxSeamDelta=Math.max(maxSeamDelta,Math.abs(weather(a,seed,size,.65,1)-weather(b,seed,size,.65,1)));
}
assert(maxSeamDelta<1e-5,'longitude seam');
const result={kind:'CPU algebra + source contracts, NOT Unreal/render acceptance',samples:count,
 hlslLiteralBytes,
 shaderSha256:crypto.createHash('sha256').update(shader).digest('hex'),weatherRange:[min,max],
 maxRadiusError,maxSeamDelta,meanSwirlChange:swirlDelta/count,checks:'PASS'};
console.log(JSON.stringify(result,null,2));
if(process.argv[2])fs.writeFileSync(process.argv[2],JSON.stringify(result,null,2),{flag:'wx'});
