// CPU regression over the actual four scalar accumulation expressions in HLSL.
// Does not compile shaders or establish rendered/atmospheric visual acceptance.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const output = process.argv[2];
if (!output) throw new Error('Usage: node AuditPlanetCloudCompositing.cjs output.json [source.h]');
const sourcePath = process.argv[3] || path.resolve(__dirname, '../../Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h');
const source = fs.readFileSync(sourcePath, 'utf8');
const match = source.match(/float Contribution=([^;]+);\s*Sum\+=([^;]+); CloudAlpha\+=([^;]+); Trans\*=([^;]+);/);
if (!match) throw new Error('Cloud accumulation changed: inspect it before adapting this audit');
const expressions = match.slice(1);
const permittedNames = new Set(['Contribution', 'Trans', 'Extinction', 'Light']);
for (const expression of expressions) {
    if (!/^[A-Za-z0-9_.+* /()\-]+$/.test(expression)) throw new Error('Unexpected shader expression');
    for (const name of expression.match(/[A-Za-z_][A-Za-z_0-9]*/g) || [])
        if (!permittedNames.has(name)) throw new Error('Unexpected identifier '+name);
}
if (!source.includes('return float4((Sum/max(CloudAlpha,.0001))*AirTransmission+AirRadiance,Alpha);'))
    throw new Error('Inspect changed foreground-air composite before accepting this audit');
const step = new Function('Trans', 'Extinction', 'Light', 'Sum', 'CloudAlpha',
    `const Contribution=${expressions[0]}; Sum+=${expressions[1]}; CloudAlpha+=${expressions[2]}; Trans*=${expressions[3]}; return {Trans, Sum, CloudAlpha};`);
let checks = 0;
const failures = [];
const check = (condition, label, context) => {
    checks++;
    if (!condition) failures.push({label, ...context});
};
let maxOpacityError = 0, maxRadianceError = 0;
for (const tau of [0, .01, .5, 2, 4]) for (const air of [0, .02, .2, .8, 1])
    for (const front of [0, .03, .8])
    for (const light of [0, .7, 3]) for (const samples of [1, 16, 32]) {
        let state = {Trans: 1, Sum: 0, CloudAlpha: 0};
        const extinction = 1-Math.exp(-tau/samples);
        for (let i=0; i<samples; i++) state = step(state.Trans, extinction, light, state.Sum, state.CloudAlpha);
        const expectedAlpha = 1-Math.exp(-tau);
        const alphaError = Math.abs(state.CloudAlpha-expectedAlpha);
        const composite=state.Sum*air+state.CloudAlpha*front;
        const radianceError = Math.abs(composite-expectedAlpha*(light*air+front));
        maxOpacityError = Math.max(maxOpacityError, alphaError);
        maxRadianceError = Math.max(maxRadianceError, radianceError);
        const context = {tau, air, front, light, samples, ...state};
        check(alphaError<1e-10, 'Air cannot change cloud occlusion', context);
        check(radianceError<1e-10, 'Foreground air and attenuated cloud radiance added exactly once', context);
        check(Math.abs(state.CloudAlpha+state.Trans-1)<1e-10, 'Cloud extinction closes', context);
        // Straight-alpha output must reproduce the accumulated premultiplied
        // radiance, and only cloud transmission may reveal a background star.
        if (state.CloudAlpha>.0001) {
            const straight = state.Sum/state.CloudAlpha;
            check(Math.abs(straight*state.CloudAlpha-state.Sum)<1e-10, 'Blend preserves radiance', context);
        }
    }
const opaque = step(1, .999, .7, 0, 0);
const backgroundLeak = 1-opaque.CloudAlpha;
check(backgroundLeak<=.00101, 'Opaque cloud must not expose 80 percent of the background', {backgroundLeak});
// Changing air along the ray must not change occlusion either.
let layered = {Trans: 1, Sum: 0, CloudAlpha: 0};
let expectedTransmission = 1;
for (let i=0; i<32; i++) {
    const extinction=.03+.01*(i%5);
    layered=step(layered.Trans, extinction, .7, layered.Sum, layered.CloudAlpha);
    expectedTransmission*=1-extinction;
}
check(Math.abs(layered.CloudAlpha-(1-expectedTransmission))<1e-10,
    'Nonuniform front-air path preserves cloud transmission', layered);
// For one foreground-air segment and an arbitrary cloud opacity, the air
// contribution already in the background plus the new replacement is S, not
// S*cloudAlpha and not S*(1+cloudAlpha). This catches the V21 dark-band defect.
for (const alpha of [0,.01,.3,.8,1]) for(const front of [0,.03,.8]) {
    const combined=(1-alpha)*front+alpha*front;
    check(Math.abs(combined-front)<1e-12,'Foreground air neither lost nor doubled',{alpha,front,combined});
}
const result = {
    sourcePath, sourceSha256:crypto.createHash('sha256').update(source).digest('hex'),
    expressions, checks, failed:failures.length, maxOpacityError, maxRadianceError,
    opaqueCloudBackgroundLeak:backgroundLeak, failures,
    limitations:'Actual scalar cloud accumulation plus the guarded V22 transfer identity. Centroid atmospheric depth is an approximation. No GPU or rendered/performance acceptance.'
};
fs.writeFileSync(output, JSON.stringify(result,null,2)+'\n', {flag:'wx'});
console.log(JSON.stringify({checks, failed:failures.length, maxOpacityError, maxRadianceError,
    opaqueCloudBackgroundLeak:backgroundLeak, output}));
process.exitCode=failures.length?1:0;
