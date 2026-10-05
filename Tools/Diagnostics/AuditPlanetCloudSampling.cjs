// CPU quadrature audit only. No GPU, rendered appearance, temporal or cost acceptance.
// Run: node Tools/Diagnostics/AuditPlanetCloudSampling.cjs [new-report.json]
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const project = path.resolve(__dirname, '../..');
const sourcePath = path.join(project, 'Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h');
const source = fs.readFileSync(sourcePath, 'utf8');
const modelPath = path.join(__dirname, 'AuditPlanetCloudDensity.cjs');
const modelSource = fs.readFileSync(modelPath, 'utf8');

// Reuse the existing source-checked CloudField CPU model, without running its
// unrelated 50,000-point statistical audit or allowing it to write a report.
const boundary = 'let rng = 19686;';
assert.equal(modelSource.split(boundary).length, 2, 'Density-model boundary changed');
const context = { require, __dirname, console, process: { argv: [] } };
vm.runInNewContext(modelSource.split(boundary)[0]
    + '\nglobalThis.cloudModel={physicalDensity,noise,rot,add,mul,expected};', context);
const { physicalDensity, noise, rot, add, mul, expected } = context.cloudModel;
for (const required of [
    'float SamplePhase=.05+.90*float(Phase&65535u)/65535.;',
    'int Samples=(int)clamp(ceil((End-Start)/max(Thickness,.001)*16.),16.,32.);',
    'float RayLength=End-Start, DistributionScale=max(Thickness*.5,.05);',
    'float Growth=pow(1.+RayLength/DistributionScale,1./float(Samples));',
    'float SampleDistance=NextStart+SamplePhase*Step;',
    'float Footprint=max(RayFootprint*SampleDistance,Step*.5);',
    'return lerp(.68,.43,weatherBody)+.48*heightDistance*heightDistance;',
    'float mean=.5+.72*coarseWeight*(coarse-.5)+.28*fineWeight*(fine-.5);',
    '.5184*(1.-coarseWeight*coarseWeight)',
    '.0784*(1.-fineWeight*fineWeight)',
    'const float ExtinctionPerKm=8.*max(CloudDensity,0.);'
]) assert(source.includes(required), 'V27 audit contract changed: ' + required);

function refinedSourceAudit() {
    const refinedPath = path.join(project,'Source/APS_ALPHA/Editor/APSPlanetCloudRefinedHlsl.h');
    const refined = fs.readFileSync(refinedPath,'utf8');
    assert(refined.includes('FString Shader = APSPlanetCloudHlsl::Code();'));
    assert(refined.includes('return Shader.ReplaceInline(Before, After, ESearchCase::CaseSensitive) == 1;'));
    const baseParts = [...source.matchAll(/TEXT\(R"HLSL\(([\s\S]*?)\)HLSL"\)/g)];
    assert.equal(baseParts.length,6,'Shared shader literal layout changed; review concatenation');
    const baseline = baseParts.map(part => part[1]).join('');
    const token = String.raw`TEXT\((?:R"HLSL\([\s\S]*?\)HLSL"|"[^"\r\n]*")\)`;
    const replacementPattern = new RegExp(String.raw`if\s*\(!ReplaceOne\(\s*(${token})\s*,\s*(${token})\s*\)\)\s*return FString\(\);`,'g');
    const replacements = [...refined.matchAll(replacementPattern)];
    assert.equal(replacements.length,3,'Expected exactly three fail-closed candidate splices');
    assert.equal((refined.match(/\bif\s*\(!ReplaceOne\(/g)||[]).length,3);
    const decode = literal => literal.startsWith('TEXT(R"HLSL(')
        ? literal.slice('TEXT(R"HLSL('.length,-')HLSL")'.length)
        : JSON.parse(literal.slice(5,-1));
    const expectedBefore = [
        'float BillowFrequencyScale=rcp(clamp(WeatherScale,.25,3.));',
        'float SampleDistance=NextStart+SamplePhase*Step;',
        'float LD=F.filteredDensity(F.advect(LP,WindRotation.xy),SeedOffset,Footprint,LT,BillowFrequencyScale);'
    ];
    const expectedAfterAnchors = [
        ['float BillowFrequencyScale=rcp(clamp(WeatherScale,.25,3.));',
            'float PixelFootprintAtEntry=RayFootprint*Start*BillowFrequencyScale;',
            'float FarPhaseBlend=smoothstep(1.6,3.2,PixelFootprintAtEntry);',
            'float StablePhase=lerp(SamplePhase,.5+.2*(SamplePhase-.5),FarPhaseBlend);'],
        ['float SampleDistance=NextStart+StablePhase*Step;'],
        ['float LightFootprint=max(Footprint,LightLength*.25);',
            'float LD=F.filteredDensity(F.advect(LP,WindRotation.xy),SeedOffset,LightFootprint,LT,BillowFrequencyScale);']
    ];
    const stripComments = text => text.replace(/\/\/[^\r\n]*/g,'')
        .split(/\r?\n/).map(line => line.trim()).filter(Boolean).join('\n');
    let generated = baseline;
    const spliceCounts = [];
    for (const [index,replacement] of replacements.entries()) {
        const before = decode(replacement[1]), after = decode(replacement[2]);
        assert.equal(before,expectedBefore[index],'Candidate anchor differs from audited splice');
        assert.equal(stripComments(after),expectedAfterAnchors[index].join('\n'),
            'Candidate replacement has unaudited executable code');
        const count = generated.split(before).length-1;
        assert.equal(count,1,'Candidate must replace exactly one baseline occurrence');
        spliceCounts.push(count);
        generated = generated.replace(before,after);
    }
    for (const name of ['PixelFootprintAtEntry','FarPhaseBlend','StablePhase','LightFootprint']) {
        assert(!new RegExp(String.raw`\b${name}\b`).test(baseline),'New identifier collides with baseline');
        assert.equal((generated.match(new RegExp(String.raw`\bfloat ${name}\s*=`,'g'))||[]).length,1);
    }
    for (const anchors of expectedAfterAnchors) for (const anchor of anchors)
        assert.equal(generated.split(anchor).length-1,1,'Generated candidate anchor missing or duplicated');
    assert(!generated.includes('NextStart+SamplePhase*Step'));
    assert(!generated.includes(expectedBefore[2]));
    for (const retained of ['float mean=.5+.72*coarseWeight*(coarse-.5)+.28*fineWeight*(fine-.5);',
        '.5184*(1.-coarseWeight*coarseWeight)', '.0784*(1.-fineWeight*fineWeight)',
        'int Samples=(int)clamp(ceil((End-Start)/max(Thickness,.001)*16.),16.,32.);',
        '[unroll] for(int j=0;j<2;j++)']) assert(generated.includes(retained));
    assert(!generated.includes('.61803398875'),'Rejected golden phase must not enter V31');
    return { scope: 'C++ literal reconstruction/source checks only; not C++ or HLSL compilation',
        source: path.relative(project,refinedPath).replaceAll('\\','/'),
        sourceSha256: crypto.createHash('sha256').update(refined).digest('hex'),
        baselineHlslSha256: crypto.createHash('sha256').update(baseline).digest('hex'),
        generatedHlslSha256: crypto.createHash('sha256').update(generated).digest('hex'),
        spliceCounts, failClosed: true, noNewNameCollisions: true,
        baselineOctaveMixRetained: true, noGoldenPhase: true, viewAndLightBudgetsRetained: true };
}
const refinedSource = refinedSourceAudit();

const PHASE_COUNT = 512;
const GOLDEN_STEP = .61803398875;
const EXTINCTION_PER_KM = 8;
const THICKNESS_KM = 2.212;
const sat = x => Math.max(0, Math.min(1, x));
const gaussian = x => Math.exp(-(x*x));
const fraction = x => x - Math.floor(x);
const alpha = integral => -Math.expm1(-integral*EXTINCTION_PER_KM);
const phases = Array.from({ length: PHASE_COUNT }, (_, i) => (i+.5)/PHASE_COUNT);
function refinedPhase(baselinePhase,entryDomainFootprint) {
    const t = sat((entryDomainFootprint-1.6)/(3.2-1.6));
    const blend = t*t*(3-2*t);
    return baselinePhase+(.5+.2*(baselinePhase-.5)-baselinePhase)*blend;
}

function geometricBins(lengthKm, thicknessKm) {
    const count = Math.max(16, Math.min(32, Math.ceil(lengthKm/thicknessKm*16)));
    const scale = Math.max(thicknessKm*.5, .05);
    let growth = Math.pow(1+lengthKm/scale, 1/count);
    let step = scale*(growth-1);
    if (growth < 1.00001) { growth = 1; step = lengthKm/count; }
    let start = 0;
    const bins = [];
    for (let k=0; k<count; k++) {
        const width = Math.max(0, Math.min(step, lengthKm-start));
        bins.push({ start, width });
        start += width;
        step *= growth;
    }
    assert(Math.abs(start-lengthKm) < 1e-9, 'Geometric bins must span the same ray');
    return bins;
}

function integrate(fixture, bins, hash01, mode) {
    let result = 0;
    for (let k=0; k<bins.length; k++) {
        const { start, width } = bins[k];
        let phase = .05+.9*fraction(hash01+(mode === 'golden' ? k*GOLDEN_STEP : 0));
        if (mode === 'far_narrow' && (fixture.minimumPixelFootprintKm||0)>=1.6)
            phase = .5+.2*(phase-.5);
        if (mode === 'far_blend') phase = refinedPhase(phase,fixture.minimumPixelFootprintKm||0);
        const distance = start+phase*width;
        result += fixture.density(distance, width)*width;
    }
    return result;
}

function reference(fixture, bins, subdivisions) {
    if (fixture.exactIntegral !== undefined) return fixture.exactIntegral;
    let result = 0;
    // Freeze each production bin's integration footprint. Using the tiny
    // reference step as the footprint would compare different density fields.
    for (const { start, width } of bins)
        for (let j=0; j<subdivisions; j++)
            result += fixture.density(start+width*(j+.5)/subdivisions, width)
                *width/subdivisions;
    return result;
}

function statistics(values, target) {
    const mean = values.reduce((sum, x) => sum+x, 0)/values.length;
    const variance = values.reduce((sum, x) => sum+(x-mean)**2, 0)/values.length;
    const bias = mean-target;
    return { mean, meanBias: bias, variance, standardDeviation: Math.sqrt(variance),
        rmse: Math.sqrt(variance+bias*bias), minimum: Math.min(...values),
        maximum: Math.max(...values) };
}

const fixtures = [
    { name: 'smooth_broad_vertical_lobe', kind: 'synthetic', lengthKm: THICKNESS_KM,
        density: x => Math.sin(Math.PI*x/THICKNESS_KM)**2,
        exactIntegral: THICKNESS_KM*.5 },
    { name: 'smooth_narrow_vertical_lobe', kind: 'synthetic', lengthKm: THICKNESS_KM,
        density: x => gaussian((x/THICKNESS_KM-.46)/.16) },
    { name: 'sparse_hard_interval', kind: 'synthetic', lengthKm: THICKNESS_KM,
        density: x => x>.72 && x<1.07 ? .4 : 0,
        exactIntegral: (1.07-.72)*.4 },
    { name: 'grazing_broad_lobes', kind: 'synthetic', lengthKm: 214,
        density: x => .02*gaussian((x-18)/9)+.012*gaussian((x-85)/26) },
    { name: 'grazing_fine_gaps', kind: 'synthetic', lengthKm: 214,
        density: x => .03*gaussian((x-28)/25)*sat(.5+.5*Math.sin(x*2.3)) }
];

const radiusKm = 6750, bottomKm = 6, innerKm = radiusKm+bottomKm;
const seed = [13,24,35];
const bodyThreshold = (height, body) => {
    const distance = Math.abs((height-.46)/(height >= .46 ? .54 : .46));
    return .68+(.43-.68)*body+.48*distance*distance;
};
const densityAt = (point, height, body, footprint) => physicalDensity(
    point, seed, footprint, bodyThreshold(height, body), 1);

// Fixed weather-body fixtures isolate the integrator and density field. They
// do not claim to reproduce the runtime weather map, scene or screenshot.
for (const body of [.35,.6,1]) {
    for (const [x,y] of [[17.2,-38.1],[177.2,123.4],[-451.7,98.4]]) {
        fixtures.push({ name: `cloudfield_vertical_body${body}_xy${x}_${y}`,
            kind: 'CloudField_CPU', lengthKm: THICKNESS_KM,
            density: (at, step) => densityAt([x,y,innerKm+at],
                at/THICKNESS_KM, body, step*.5) });
    }
    // A spherical cloud-shell chord with the same 214 km grazing span used
    // in the shader's integration rationale. The real geometric height varies.
    const lengthKm = 214, half = lengthKm*.5, outerKm = innerKm+THICKNESS_KM;
    const z = Math.sqrt(outerKm*outerKm-half*half);
    fixtures.push({ name: `cloudfield_grazing_body${body}`, kind: 'CloudField_CPU', lengthKm,
        density: (at, step) => {
            const p = [at-half,0,z];
            const height = sat((Math.hypot(...p)-innerKm)/THICKNESS_KM);
            return densityAt(p, height, body, Math.max(at*.001,step*.5));
        } });
}

const rows = fixtures.map(fixture => {
    const bins = geometricBins(fixture.lengthKm, THICKNESS_KM);
    const target = reference(fixture, bins, 1024);
    const coarseReference = reference(fixture, bins, 512);
    const modes = {};
    for (const mode of ['common', 'golden']) {
        const values = phases.map(phase => integrate(fixture, bins, phase, mode));
        assert(values.every(x => Number.isFinite(x) && x>=0));
        modes[mode] = { densityIntegral: statistics(values, target),
            alpha: statistics(values.map(alpha), alpha(target)) };
    }
    const baseline = modes.common.densityIntegral;
    const candidate = modes.golden.densityIntegral;
    const ratio = baseline.standardDeviation>1e-12
        ? candidate.standardDeviation/baseline.standardDeviation : null;
    const outcome = ratio === null ? 'no_phase_variation'
        : ratio < .99 ? 'lower_variance' : ratio > 1.01 ? 'higher_variance' : 'similar';
    return { name: fixture.name, kind: fixture.kind, lengthKm: fixture.lengthKm,
        samples: bins.length, referenceDensityIntegral: target,
        referenceAlpha: alpha(target),
        referenceConvergenceDelta: Math.abs(target-coarseReference),
        standardDeviationRatio: ratio, outcome, ...modes };
});

// Separate far-only proposal. The gate uses the MINIMUM screen footprint,
// not the possibly large marching step: close clouds keep their full phase.
// At feature scale 1, a 1.6 km screen footprint makes BOTH octave weights zero.
function farPhaseAudit() {
    const farFixtures = [], filteredSigma = .135*Math.hypot(.72,.28);
    const farDensity = (height,body) => expected(.5,filteredSigma,bodyThreshold(height,body));
    const testedFootprints = [1.6,1.8,2,2.4,2.8,3.2,5,10];
    for (const minimumPixelFootprintKm of testedFootprints) {
        // Verify the cheap far-field algebra against the imported model.
        for (const body of [.35,.6,1]) for (const height of [0,.3,.46,.8,1]) {
            const actual = densityAt([17.2,-38.1,innerKm+height*THICKNESS_KM],
                height,body,minimumPixelFootprintKm);
            assert(Math.abs(actual-farDensity(height,body))<1e-12,
                'Fixed far footprint must fully filter both billow octaves');
        }
        for (const body of [.35,.6,1]) {
            // Test both ground-facing and orbit-facing traversal of the same
            // vertical column because the geometric steps are asymmetric.
            for (const direction of ['ascending','descending'])
                farFixtures.push({ name: `far_vertical_${direction}_body${body}_pixel${minimumPixelFootprintKm}`,
                    geometry: 'vertical_'+direction, body, minimumPixelFootprintKm,
                    lengthKm: THICKNESS_KM,
                    density: at => farDensity(direction === 'ascending'
                        ? at/THICKNESS_KM : 1-at/THICKNESS_KM,body) });
            const lengthKm = 214, half = lengthKm*.5, outerKm = innerKm+THICKNESS_KM;
            const z = Math.sqrt(outerKm*outerKm-half*half);
            farFixtures.push({ name: `far_grazing_body${body}_pixel${minimumPixelFootprintKm}`,
                geometry: 'grazing', body, minimumPixelFootprintKm, lengthKm,
                density: at => farDensity(sat((Math.hypot(at-half,z)-innerKm)/THICKNESS_KM),body) });
        }
    }
    // Confirm that normal fixtures do not accidentally use a ray-step gate.
    for (const fixture of fixtures) {
        const bins = geometricBins(fixture.lengthKm,THICKNESS_KM);
        for (const phase of [.01,.5,.99]) {
            assert.equal(integrate(fixture,bins,phase,'far_narrow'),
                integrate(fixture,bins,phase,'common'),'Near phase must remain unchanged');
            assert.equal(integrate(fixture,bins,phase,'far_blend'),
                integrate(fixture,bins,phase,'common'),'V31 near phase must remain unchanged');
        }
    }
    let maximumPhaseBoundaryDifference = 0;
    const epsilon = 1e-6;
    for (const hash of phases) {
        const phase = .05+.9*hash;
        assert.equal(refinedPhase(phase,1.6),phase);
        assert(Math.abs(refinedPhase(phase,3.2)-(.5+.2*(phase-.5)))<1e-15);
        for (const boundary of [1.6,3.2]) maximumPhaseBoundaryDifference = Math.max(
            maximumPhaseBoundaryDifference, Math.abs(refinedPhase(phase,boundary+epsilon)
                -refinedPhase(phase,boundary-epsilon)));
        let previousDistance = Math.abs(phase-.5);
        for (let i=0;i<=64;i++) {
            const current = refinedPhase(phase,1.6+1.6*i/64);
            const distance = Math.abs(current-.5);
            assert(distance<=previousDistance+1e-15,'Blend must monotonically approach its narrower range');
            assert(current>=.05 && current<=.95);
            previousDistance = distance;
        }
    }
    assert(maximumPhaseBoundaryDifference<1e-9,'Smoothstep boundary is discontinuous');
    const references = new Map();
    const results = farFixtures.map(fixture => {
        const bins = geometricBins(fixture.lengthKm,THICKNESS_KM);
        const key = fixture.geometry+'_'+fixture.body;
        if (!references.has(key)) references.set(key,[reference(fixture,bins,4096),reference(fixture,bins,2048)]);
        const [target,coarseTarget] = references.get(key);
        const modes = {};
        for (const mode of ['common','far_narrow','far_blend']) {
            const values = phases.map(phase => integrate(fixture,bins,phase,mode));
            modes[mode] = { densityIntegral: statistics(values,target),
                alpha: statistics(values.map(alpha),alpha(target)) };
        }
        const baselineSD = modes.common.densityIntegral.standardDeviation;
        return { name: fixture.name, geometry: fixture.geometry, body: fixture.body,
            minimumPixelFootprintKm: fixture.minimumPixelFootprintKm, samples: bins.length,
            referenceDensityIntegral: target, referenceAlpha: alpha(target),
            referenceConvergenceDelta: Math.abs(target-coarseTarget),
            densityStandardDeviationRatio: modes.far_narrow.densityIntegral.standardDeviation/baselineSD,
            absoluteOpacityBiasWithinOnePercent: Math.abs(modes.far_narrow.alpha.meanBias)<=.01,
            blendDensityStandardDeviationRatio: modes.far_blend.densityIntegral.standardDeviation/baselineSD,
            blendAbsoluteOpacityBiasWithinOnePercent: Math.abs(modes.far_blend.alpha.meanBias)<=.01,
            ...modes };
    });
    return { proposalOnly: true, phase: '.5+.2*(baselinePhase-.5), common for the whole ray',
        gate: 'Minimum spatial pixel footprint * billow frequency >= 1.6; fixtures use frequency 1',
        baselinePhaseRange: [.05,.95], candidatePhaseRange: [.41,.59],
        fixedPixelFootprintsKm: testedFootprints, unchangedNearFixtures: fixtures.length,
        opacityBiasLimit: .01,
        maximumCandidateAbsoluteAlphaBias: Math.max(...results.map(row => Math.abs(row.far_narrow.alpha.meanBias))),
        maximumReferenceConvergenceDelta: Math.max(...results.map(row => row.referenceConvergenceDelta)),
        rejectedForOpacityBias: results.some(row => !row.absoluteOpacityBiasWithinOnePercent),
        implementedBlend: { phase: 'lerp(baseline,.5+.2*(baseline-.5),smoothstep(1.6,3.2,entryDomainFootprint))',
            maximumPhaseBoundaryDifference, boundaryDifferenceEpsilon: epsilon,
            monotoneAndBounded: true,
            maximumAbsoluteAlphaBias: Math.max(...results.map(row => Math.abs(row.far_blend.alpha.meanBias))),
            rejectedForOpacityBias: results.some(row => !row.blendAbsoluteOpacityBiasWithinOnePercent) },
        limitations: [
            'These constant weather-body fixtures do not test a changing regional/weather envelope or camera movement',
            'All footprints yield the same fully filtered density field; varying footprint only exercises the phase blend',
            'No lighting or transmittance early exit; this is a bounded numerical check, not visual acceptance',
            'Smooth phase continuity does not establish the rendered transition quality'],
        rows: results };
}
const farPhase = farPhaseAudit();

// Independent proposal only: assess redistribution from .72/.28 to .64/.36
// without combining it with the rejected golden-phase experiment above.
// Both octave sums are one, but their total variance is DIFFERENT. Matching
// sigma coefficients preserves the filter's model, not the baseline density.
function octaveMixAudit() {
    const sampleCount = 4096, baselineWeights = [.72,.28], candidateWeights = [.64,.36];
    let state = 19686;
    const random = () => {
        state = (Math.imul(state,1664525)+1013904223) >>> 0;
        return state/4294967296;
    };
    const fields = [];
    for (let i=0; i<sampleCount; i++) {
        const p = [random()*1000,random()*1000,random()*1000];
        const c = noise(add(mul(p,.32),seed));
        const f = noise(add(rot(mul(p,.93)),seed));
        fields.push([.72*c+.28*f,.64*c+.36*f]);
    }
    const sigma = weights => .135*Math.hypot(...weights);
    const baselineSigma = sigma(baselineWeights), candidateSigma = sigma(candidateWeights);
    const thresholdRows = [.43,.48,.53,.6,.65,.68].map(threshold => {
        const baseline = fields.map(pair => sat((pair[0]-threshold)*7));
        const candidate = fields.map(pair => sat((pair[1]-threshold)*7));
        const baselineMean = baseline.reduce((a,b) => a+b)/sampleCount;
        const candidateMean = candidate.reduce((a,b) => a+b)/sampleCount;
        const pairedDelta = statistics(candidate.map((x,i) => x-baseline[i]),0);
        const baselineFilteredMean = expected(.5,baselineSigma,threshold);
        const candidateFilteredMean = expected(.5,candidateSigma,threshold);
        return { threshold, baselineResolvedMean: baselineMean, candidateResolvedMean: candidateMean,
            resolvedMeanDelta: candidateMean-baselineMean,
            pairedDeltaStandardError: pairedDelta.standardDeviation/Math.sqrt(sampleCount),
            baselineFullyFilteredMean: baselineFilteredMean,
            candidateFullyFilteredMean: candidateFilteredMean,
            filteredMeanDelta: candidateFilteredMean-baselineFilteredMean,
            baselineFilterMeanError: baselineFilteredMean-baselineMean,
            candidateFilterMeanError: candidateFilteredMean-candidateMean };
    });
    return { proposalOnly: true, sampleCount, baselineWeights, candidateWeights,
        baselineVarianceCoefficients: baselineWeights.map(x => x*x),
        candidateVarianceCoefficients: candidateWeights.map(x => x*x),
        baselineSigma, candidateSigma,
        modeledVarianceRatio: (candidateSigma/baselineSigma)**2,
        modeledStandardDeviationRatio: candidateSigma/baselineSigma,
        limitations: 'Random world-point statistics, not shell opacity, lighting, silhouettes or visual acceptance',
        rows: thresholdRows };
}
const octaveMix = octaveMixAudit();

const countOutcomes = input => input.reduce((counts,row) => {
    counts[row.outcome] = (counts[row.outcome]||0)+1;
    return counts;
}, {});
const report = {
    scope: 'CPU quadrature/source contracts only; no GPU, render, temporal or performance acceptance',
    source: path.relative(project, sourcePath).replaceAll('\\','/'),
    sourceSha256: crypto.createHash('sha256').update(source).digest('hex'),
    modelSha256: crypto.createHash('sha256').update(modelSource).digest('hex'),
    phases: PHASE_COUNT,
    phaseDistribution: 'Equally spaced stratum midpoints in hash01 [0,1); deterministic, not measured screen pixels',
    baselinePhase: '.05+.90*hash01 (same at every k)',
    candidatePhase: '.05+.90*frac(hash01+k*.61803398875)',
    preserved: ['same density field', 'same seed', 'same geometric bins', 'same 16-32 samples',
        'same integration footprint', 'same 8/km extinction'],
    limitations: ['No weather-map lookup, lighting, scene depth, temporal reprojection or early transmittance exit',
        'Vertical fixtures prescribe flat-column height, as in the existing opacity audit; grazing fixtures use spherical height',
        'CPU double precision and source checks do not execute HLSL',
        'Dense reference retains each production bin footprint to isolate quadrature',
        'Both modes retain the baseline exclusion of the outer 5 percent of each bin',
        'Alpha includes Beer-Lambert nonlinearity, so a density-mean match does not imply an alpha-mean match'],
    outcomes: countOutcomes(rows),
    cloudFieldOutcomes: countOutcomes(rows.filter(row => row.kind === 'CloudField_CPU')),
    conclusion: 'Golden per-step phase rejected for this candidate because multiple CloudField fixtures regress',
    refinedSource, farPhase, octaveMix, rows
};
if (process.argv[2]) fs.writeFileSync(process.argv[2], JSON.stringify(report,null,2)+'\n', { flag: 'wx' });
console.log(JSON.stringify({ scope: report.scope, phases: PHASE_COUNT,
    outcomes: report.outcomes, cloudFieldOutcomes: report.cloudFieldOutcomes,
    maxReferenceConvergenceDelta: Math.max(...rows.map(row => row.referenceConvergenceDelta)),
    refinedSource, farPhase, octaveMix,
    rows: rows.map(row => ({ name: row.name, samples: row.samples, outcome: row.outcome,
        standardDeviationRatio: row.standardDeviationRatio,
        baselineSD: row.common.densityIntegral.standardDeviation,
        candidateSD: row.golden.densityIntegral.standardDeviation,
        baselineMeanBias: row.common.densityIntegral.meanBias,
        candidateMeanBias: row.golden.densityIntegral.meanBias,
        baselineAlphaBias: row.common.alpha.meanBias,
        candidateAlphaBias: row.golden.alpha.meanBias }))
},null,2));
