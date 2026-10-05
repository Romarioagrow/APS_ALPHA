param(
    [ValidateSet('Water','Frozen','Terrestrial','Oasis','Ice','Tundra','Nordic','Rocky','Desert','Sand','HighMountain','Forest','Savanna','SuperEarth','Pangea','Volcanic','Metallic','Crystal','Greenhouse','Dwarf','Ocean','Ammonia','Metal','Carbon','Archipelago','Rogue','Basalt','Sulfur')][string]$Family='Terrestrial',
    [ValidateSet('Fields','Atmosphere','KeyShadow','StarShadow','NormalHex','Combined','Published')][string]$Isolation='Fields',
    [ValidateSet('Control','Candidate')][string]$SurfaceUnification,
    [switch]$DefaultAtmosphere,
    [switch]$Daylight,
    [switch]$CanonicalNormalBandwidth,
    [switch]$Clouds,
    [switch]$CloudWeather,
    [switch]$CloudWeatherCandidate,
    [switch]$CloudLayeredCandidate,
    [ValidateSet('0.5','1','2')][string]$CloudFeatureScale,
    [switch]$CloudDefault,
    [switch]$CloudFlight,
    [switch]$CloudHorizon,
    [switch]$CloudGround,
    [switch]$CloudAerialOff,
    [switch]$TerrestrialPalette,
    [switch]$FoliagePrototype,
    [switch]$FoliageFlight,
    [switch]$FoliageOff,
    [switch]$FoliageReentry,
    [switch]$FoliageDefault,
    [switch]$WalkingCollision,
    [switch]$WalkingCollisionDefault,
    [switch]$StructureExclusion,
    [switch]$SurfaceScatter,
    [switch]$TerrestrialVegetation,
    [ValidateRange(3,16)][int]$OasisTreeAttempts=3,
    [ValidateRange(0,2147483647)][int]$SurfaceSeed=424242,
    [switch]$GroundHold,
    [switch]$FlightResidency,
    [switch]$ResidencyOff,
    [ValidateRange(0,4)][int]$FlightBaseLodsPerFrame=1,
    [switch]$LodBoundary,
    [switch]$WaterFlight,
    [switch]$NativeLavaFlight,
    [switch]$LiquidReference,
    [switch]$WaterRelease,
    [switch]$WaterReleaseDefault,
    [switch]$WaterShoreTransmission,
    [switch]$ShoreWaterFactory,
    [ValidateRange(0.002,0.05)][double]$WaterNearHeightKm=0.05,
    [ValidateRange(0.05,1000)][double]$WaterCoastDepthM=20,
    [ValidateRange(30,89)][double]$LiquidViewPitchDeg=55,
    [ValidateCount(6,6)][double[]]$LiquidViewFrame,
    [switch]$WaterPayload,
    [switch]$WaterMaterial,
    [switch]$WaterSurfaceFilter,
    [switch]$WaterSurfacePass,
    [switch]$WaterSurfacePrecise,
    [switch]$WaterSurfaceAnchored,
    [switch]$WaterAnchorNoise,
    [ValidateSet('1','0.1')][string]$WaterWaveScaleFactor='1',
    [switch]$Performance,
    [switch]$CpuTrace,
    [ValidateSet(1,2,4)][int]$MeshUpdateTasks=1,
    [switch]$FastMeshCopy,
    [switch]$PreparedMesh,
    [switch]$CollisionHeightOnly,
    [ValidateRange(0,512)][int]$PreparedBudgetMiB=384,
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label
)
$ErrorActionPreference='Stop'
if($CanonicalNormalBandwidth -and ($SurfaceUnification -ne 'Candidate' -or $Isolation -ne 'Published' -or $Performance)){throw 'CanonicalNormalBandwidth requires original Published UnifiedDefault, not a timing comparison'}
if($Daylight -and (!$SurfaceUnification -or $Isolation -ne 'Published')){throw 'Daylight requires an explicit generic SurfaceUnification Published route'}
if($Family -in @('Greenhouse','Dwarf','Ocean','Ammonia','Metal','Carbon','Archipelago','Rogue','Basalt','Sulfur') -and !$SurfaceUnification){throw 'Expanded terrain families require explicit SurfaceUnification Control/Candidate'}
if($SurfaceUnification -and ($Isolation -ne 'Published' -or $CloudWeatherCandidate -or $CloudLayeredCandidate -or $WaterMaterial -or $WaterPayload -or $WaterShoreTransmission -or $PSBoundParameters.ContainsKey('TerrestrialPalette'))){throw 'Surface unification requires Published and no other material/payload candidate'}
if($SurfaceUnification -and ($WaterFlight -or $NativeLavaFlight -or $CloudFlight -or $FoliageFlight -or $FlightResidency)){throw 'Surface unification uses the generic terrain route; run named liquid/cloud/foliage/residency probes separately'}
if($CloudWeatherCandidate -and $CloudLayeredCandidate){throw 'Select one cloud candidate: refined V31 or layered V30'}
if($CloudWeatherCandidate -and (!$CloudWeather -or $CloudDefault)){throw 'CloudWeatherCandidate requires explicit CloudWeather; not a default-acceptance run'}
if($CloudLayeredCandidate -and (!$CloudWeather -or $CloudDefault)){throw 'CloudLayeredCandidate requires explicit CloudWeather; not a default-acceptance run'}
if($PSBoundParameters.ContainsKey('CloudFeatureScale') -and (!$CloudWeather -or $CloudDefault -or $CloudAerialOff -or $Performance)){throw 'CloudFeatureScale requires a visual weather fixture without default/aerial/timing experiments'}
if($CloudWeather -and (!$CloudFlight -or $CloudDefault)){throw 'Weather candidate needs an explicit cloud flight ON/OFF pair, never a default-acceptance run'}
if($ShoreWaterFactory -and (!$WaterRelease -or !$WaterReleaseDefault -or $WaterShoreTransmission)){throw 'Shore factory verification requires query-only WaterRelease WaterReleaseDefault, no replacement material'}
if($TerrestrialVegetation -and ($Family -ne 'Terrestrial' -or !$SurfaceScatter -or $FoliageDefault -or $FoliageOff)){throw 'Terrestrial vegetation trial requires explicit unpublished Terrestrial scatter flight'}
if($PSBoundParameters.ContainsKey('OasisTreeAttempts') -and ($Family -ne 'Oasis' -or !$SurfaceScatter -or $FoliageDefault)){throw 'Oasis density comparison requires explicit unpublished scatter flight'}
if($PSBoundParameters.ContainsKey('SurfaceSeed') -and !$FoliageFlight){throw 'Alternate foliage seed requires isolated foliage flight'}
if($WalkingCollisionDefault -and ($PSBoundParameters.ContainsKey('WalkingCollision') -or $PSBoundParameters.ContainsKey('StructureExclusion') -or !($FoliageDefault -or $TerrestrialVegetation) -or !$FoliageReentry -or $FoliageOff)){throw 'Query-only collision/exclusion default requires default foliage or explicit Terrestrial vegetation reentry'}
if($WaterShoreTransmission){
    if(!$WaterFlight -or $NativeLavaFlight -or $WaterRelease -or $WaterPayload -or $WaterMaterial -or $WaterSurfaceFilter -or $WaterSurfacePass -or $WaterSurfacePrecise -or $WaterSurfaceAnchored){throw 'ShoreTransmission requires only explicit WaterFlight, no other material candidate'}
    $WaterPayload=$true; $WaterMaterial=$true
}
if($NativeLavaFlight -and ($Family -ne 'Volcanic' -or $Isolation -ne 'Published' -or $WaterFlight -or $WaterRelease -or $WaterPayload -or $WaterMaterial -or $CloudFlight -or $FlightResidency -or $FoliageFlight -or $LodBoundary -or $GroundHold)){throw 'Native Lava flight needs isolated published Volcanic, no Water shader/payload'}
if($Family -eq 'Volcanic' -and !$NativeLavaFlight){throw 'Volcanic requires explicit native-lava flight'}
if($LiquidReference -and !(($WaterFlight -and $Family -eq 'Water') -or $NativeLavaFlight)){throw 'Radius/seed reference requires Water or native Volcanic liquid flight; not a save replay'}
if($NativeLavaFlight){$WaterFlight=$true} # Reuse motion/timing, never its material or UV1 opt-ins.
if($PSBoundParameters.ContainsKey('LiquidViewFrame')) {
    if(!$WaterFlight -or $LiquidViewFrame.Count -ne 6){throw 'Exact liquid view replay requires a liquid flight and six components'}
    foreach($v in $LiquidViewFrame){if([double]::IsNaN($v) -or [double]::IsInfinity($v)){throw 'Liquid view frame must be finite'}}
    $d2=0.; $t2=0.; $dt=0.
    for($i=0;$i -lt 3;$i++){$d2+=$LiquidViewFrame[$i]*$LiquidViewFrame[$i];$t2+=$LiquidViewFrame[$i+3]*$LiquidViewFrame[$i+3];$dt+=$LiquidViewFrame[$i]*$LiquidViewFrame[$i+3]}
    if([math]::Abs($d2-1.) -gt 0.000001 -or [math]::Abs($t2-1.) -gt 0.000001 -or [math]::Abs($dt) -gt 0.000001){throw 'Liquid view frame must be unit and orthogonal'}
}
if($CloudDefault -and (!$CloudFlight -or $Clouds -or $CloudAerialOff -or $Performance)){throw 'CloudDefault requires a visual cloud flight without ON/OFF overrides'}
if($CloudAerialOff -and !$Clouds){throw 'Cloud aerial A/B requires enabled clouds'}
if($CloudHorizon -and !$CloudFlight){throw 'CloudHorizon requires the dedicated cloud flight'}
if($CloudGround -and (!$CloudFlight -or !$CloudHorizon -or $Performance)){throw 'CloudGround requires a visual cloud horizon route; not a timing comparison'}
if($CloudFlight -and ($Isolation -ne 'Published' -or $Family -notin @('Oasis','Terrestrial','Water') -or $WaterFlight -or $FlightResidency -or $FoliageFlight -or $LodBoundary)){throw 'Cloud flight needs published Oasis/Terrestrial/Water route without other experiments'}
if($CloudFlight){$GroundHold=$true}
if($Clouds -and ($Isolation -ne 'Published' -or $Family -notin @('Terrestrial','Water','Oasis'))){throw 'Cloud candidate requires an eligible published wet family'}
if($WaterReleaseDefault -and !$WaterRelease){throw 'Default release verification requires WaterRelease'}
if($FoliageReentry -and !$FoliageFlight){throw 'FoliageReentry requires the dedicated foliage flight'}
if($FoliageDefault -and (!$FoliageFlight -or $FoliageOff)){throw 'Default foliage verification requires an ON foliage flight'}
if($PSBoundParameters.ContainsKey('WalkingCollision') -and (!$FoliageDefault -or !$FoliageReentry -or $FoliageOff)){throw 'WalkingCollision comparison requires default foliage and two reentry cycles, with foliage kept ON'}
if($PSBoundParameters.ContainsKey('StructureExclusion') -and (!$PSBoundParameters.ContainsKey('WalkingCollision') -or !$FoliageDefault -or !$FoliageReentry)){throw 'Structure exclusion comparison requires the grounded collision ON/OFF route'}
if($SurfaceScatter -and (!$FoliageFlight -or $FoliageDefault -or $FoliagePrototype -or $Clouds -or $CloudFlight)){throw 'SurfaceScatter requires an isolated explicit foliage flight, not default/prototype/cloud mode'}
if($WaterRelease -and (!$WaterFlight -or $WaterPayload -or $WaterMaterial -or $WaterSurfaceFilter)){throw 'Release flight must use only the ordinary factory and native payload activation'}
if($WaterAnchorNoise -and !$WaterSurfaceAnchored){throw 'WaterAnchorNoise requires WaterSurfaceAnchored'}
if($WaterSurfaceAnchored -and (!$WaterSurfaceFilter -or $WaterSurfacePass)){throw 'Anchored waves require filtered Water with ordinary shading'}
if($WaterSurfaceAnchored){$WaterSurfacePrecise=$true}
if($FoliageOff -and !$FoliageFlight){throw 'FoliageOff requires the dedicated foliage flight'}
if($FoliageFlight -and ($Isolation -ne 'Published' -or ($Family -notin @('Forest','Frozen') -and !(($SurfaceScatter -or $FoliageDefault) -and $Family -in @('Water','Oasis','Metallic','Crystal','Terrestrial'))) -or !$GroundHold -or $WaterFlight -or $FlightResidency -or $LodBoundary -or $TerrestrialPalette)){throw 'Foliage flight needs Forest/Frozen or reviewed default/scatter family Published GroundHold without other experiments'}
if($FoliageFlight){$FoliagePrototype = -not ($FoliageOff -or $FoliageDefault -or $SurfaceScatter)}
if($TerrestrialPalette -and ($Family -ne 'Terrestrial' -or $Isolation -ne 'Published' -or $WaterFlight -or $FlightResidency -or $FoliagePrototype -or $LodBoundary)){throw 'Palette route needs published Earth-like terrain without another experiment'}
if($FlightResidency -and ($Isolation -ne 'Published' -or $WaterFlight -or $GroundHold -or $LodBoundary -or $FoliagePrototype)){throw 'Residency route needs Published without other experiments'}
if(($WaterPayload -or $WaterMaterial) -and -not $WaterFlight){throw 'Water switches require WaterFlight'}
if($Performance -and -not ($WaterFlight -or $FlightResidency -or $FoliageFlight -or $CloudFlight)){throw 'Performance needs a measured flight route'}
if($ResidencyOff -and -not $FlightResidency){throw 'ResidencyOff requires FlightResidency route'}
if($WaterMaterial -and -not $WaterPayload){throw 'Water material requires native WaterPayload'}
if($WaterSurfaceFilter -and -not $WaterMaterial){throw 'WaterSurfaceFilter requires WaterMaterial'}
if($WaterSurfacePass -and -not $WaterSurfaceFilter){throw 'WaterSurfacePass requires WaterSurfaceFilter'}
if($WaterSurfacePrecise -and (-not $WaterSurfaceFilter -or $WaterSurfacePass)){throw 'WaterSurfacePrecise requires filtered Water with ordinary shading'}
if($WaterWaveScaleFactor -ne '1' -and -not $WaterSurfaceFilter){throw 'WaterWaveScaleFactor requires WaterSurfaceFilter'}
if($CpuTrace -and -not $Performance){throw 'CpuTrace requires Performance; timing pair normally excludes per-vertex tracing'}
if($WaterFlight -and ($Isolation -ne 'Published' -or ($Family -notin @('Water','Terrestrial','Oasis') -and !$NativeLavaFlight) -or $LodBoundary -or $GroundHold -or $FoliagePrototype)){throw 'Liquid flight needs explicit wet Published family without other experiments'}
if($LodBoundary -and ($Isolation -ne 'Published' -or $GroundHold -or $FoliagePrototype)){throw 'LOD boundary requires Published with no other experiment'}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
if($SurfaceUnification){
    . (Join-Path $PSScriptRoot 'SurfaceUnificationDiagnostic.ps1')
    $surfaceSelection=Get-APSSurfaceUnificationDiagnostic -ProjectRoot $projectRoot -Mode $SurfaceUnification
}
if($PSBoundParameters.ContainsKey('CollisionHeightOnly') -and
    (Get-Content -Raw -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.h')) -notmatch '#define APS_COLLISION_HEIGHT_HOOK 1') {
    throw 'Height-only native/APS candidate is not installed; cannot produce a valid ON/OFF comparison'
}
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/'+$Family.ToLowerInvariant()+'-'+$Label
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; no process touched'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
$candidate=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'+$(if($Isolation -eq 'Combined'){'ContinuityCombined20260929V1/MI_APS_NormalWarpTerra.uasset'}elseif($Isolation -eq 'NormalHex'){'NormalHex20260929V1/MI_APS_NormalWarpTerra.uasset'}else{'OrbitalFields20260929V3/MI_APS_LodPixelTerra.uasset'})
if($Isolation -eq 'Published'){$candidate=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.uasset';$DefaultAtmosphere=$true}
if(-not(Test-Path -LiteralPath $candidate)){throw 'Required diagnostic template not baked'}
if($WaterSurfaceFilter -and -not(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930/MI_APS_WaterSurface.uasset'))){throw 'Bake WaterSurface first'}
if($WaterSurfacePass -and -not(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePass20260930/MI_APS_WaterSurface.uasset'))){throw 'Bake WaterSurfacePass first'}
if($WaterSurfacePrecise -and -not(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePrecise20260930/MI_APS_WaterSurface.uasset'))){throw 'Bake WaterSurfacePrecise first'}
if($WaterSurfaceAnchored -and -not(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchored20260930/MI_APS_WaterAnalytic.uasset'))){throw 'Bake anchored water first'}
if($WaterAnchorNoise -and -not(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930/MI_APS_WaterAnalytic.uasset'))){throw 'Bake anchored noise first'}
if($Isolation -in @('NormalHex','Combined','Published') -and $FoliagePrototype -and !$FoliageFlight){throw 'Terrain flight must not activate unrelated foliage'}
$dll=Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
if($CanonicalNormalBandwidth -and (Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Tests/APSPublishedNormalBandwidthProbe.h')).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Canonical normal measurement newer than DLL; build first'}
foreach($p in @('Source/APS_ALPHA/Core/Planetary/APSShoreWaterMaterial.h','Source/APS_ALPHA/Core/Planetary/APSWaterLightingSubsystem.h','Source/APS_ALPHA/Core/Planetary/APSWaterLightingSubsystem.cpp','Source/APS_ALPHA/Tests/APSShoreWaterTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$p)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Shore factory/binder newer than DLL'}
}
if($FoliageFlight){
    foreach($source in @('Source/APS_ALPHA/Generation/AstroGenerator.cpp','Source/APS_ALPHA/Core/Planetary/APSSurfaceLandingRelief.h','Source/APS_ALPHA/Tests/APSSurfaceLandingReliefTests.cpp')) {
        if((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Landing recovery source newer than DLL; build first'}
    }
}
if($Clouds -or $CloudFlight) {
    foreach($cloudSource in @('Source/APS_ALPHA/Core/Planetary/APSPlanetCloudPolicy.h',
        'Source/APS_ALPHA/Core/Rendering/APSPlanetCloudComponent.cpp',
        'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h',
        'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudSettings.h',
        'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudLayers.h',
        'Source/APS_ALPHA/Tests/APSPlanetCloudWeatherTests.cpp',
        'Source/APS_ALPHA/Tests/APSPlanetCloudLayersTests.cpp',
        'Source/APS_ALPHA/Editor/APSPlanetCloudBuilder.h',
        'Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h',
        'Source/APS_ALPHA/Editor/APSPlanetCloudLayeredHlsl.h',
        'Source/APS_ALPHA/Tests/APSPlanetCloudTests.cpp')) {
        if((Get-Item -LiteralPath ($projectRoot+'/'+$cloudSource)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc) {
            throw ('Cloud source newer than DLL; build before visual verification: '+$cloudSource)
        }
    }
    $cloudHeader=if($CloudWeather){'APSPlanetCloudWeather.h'}else{'APSPlanetCloudPolicy.h'}
    $cloudPolicy=Get-Content -Raw -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Core/Planetary/'+$cloudHeader)
    $cloudPathMatch=[regex]::Match($cloudPolicy,'MaterialPath\s*=\s*TEXT\("(/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/Cloud[^"]+)"\)')
    if(!$cloudPathMatch.Success){throw 'Cannot resolve the exact cloud candidate asset from its policy'}
    $cloudObjectPath=$cloudPathMatch.Groups[1].Value
    $cloudAsset=$projectRoot+'/Content/'+$cloudObjectPath.Substring(6).Split('.')[0]+'.uasset'
    if($CloudWeather){
        . (Join-Path $PSScriptRoot 'CloudWeatherDiagnostic.ps1')
        $cloudSelection=Get-APSCloudWeatherDiagnostic -ProjectRoot $projectRoot -Candidate:$CloudWeatherCandidate -LayeredCandidate:$CloudLayeredCandidate
        $cloudObjectPath=$cloudSelection.ObjectPath
        $cloudAsset=$cloudSelection.Asset
    }
    if(!(Test-Path -LiteralPath $cloudAsset)){throw ('Cloud candidate not baked: '+$cloudAsset)}
}
if($CloudFlight -and (Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Tests/APSCloudFlightProbe.h')).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Cloud flight probe newer than DLL: build first'}
if($WaterRelease){
    if(!(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1/MI_APS_CoastalWater.uasset'))){throw 'Bake WaterV1 first'}
    foreach($p in @('Source/APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h','Source/APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h','Source/APS_ALPHA/Generation/AstroGenerator.cpp','Source/APS_ALPHA/Generation/PlanetarySurfaceGeneratorStreaming.cpp','Source/APS_ALPHA/Tests/APSSharedGeneratedLiquidSelectionTests.cpp')){
        if((Get-Item -LiteralPath ($projectRoot+'/'+$p)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Water release source newer than DLL'}
    }
}
if($FoliageFlight -and (Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Tests/APSFoliageFlightProbe.h')).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Foliage flight probe newer than DLL: build first'}
if($FoliageFlight){
    foreach($source in @('Source/APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.cpp','Source/APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.h','Source/APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.cpp','Source/APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.h','Source/APS_ALPHA/Tests/APSFoliageExclusionTests.cpp','Source/APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.cpp')) {
        if((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Foliage implementation newer than DLL: build first'}
    }
}
if($TerrestrialPalette) {
    foreach($paletteSource in @('Source/APS_ALPHA/Core/Planetary/APSTerrestrialMaterialPalette.h','Source/APS_ALPHA/Core/Planetary/APSNativeTerrainMaterial.h','Source/APS_ALPHA/Generation/PlanetarySurfaceGeneratorStreaming.cpp','Source/APS_ALPHA/Tests/APSTerrestrialMaterialPaletteTests.cpp')) {
        if((Get-Item -LiteralPath ($projectRoot+'/'+$paletteSource)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Palette source newer than DLL: build first'}
    }
}
foreach($source in @('Source/APS_ALPHA/Core/World/APSPlanetEnvironmentFlightResidency.cpp','Source/APS_ALPHA/Tests/APSFlightResidencyProbe.h','Source/APS_ALPHA/Tests/APSWaterFlightProbe.h','Source/APS_ALPHA/Tests/APSWaterNormalABProbe.h','Source/APS_ALPHA/Core/Planetary/APSWaterSurfaceLighting.h','Source/APS_ALPHA/Core/Planetary/APSWaterSurfaceFilter.h','Source/APS_ALPHA/Tests/APSWorldScapeLodBoundaryProbe.h','Source/APS_ALPHA/Tests/APSSharedTerrainLodABProbe.h','Source/APS_ALPHA/Tests/APSGeneratedGameplayHandoffSmokeTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Known newer flight diagnostic source: build first'}
}
if($FoliagePrototype) {
    $palette=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1/FC_APS_Proto_'+$Family+'.uasset'
    if(-not(Test-Path -LiteralPath $palette)){throw 'Required foliage prototype palette not baked; editor not started'}
}
if($SurfaceScatter -or $FoliageDefault){
    $palette=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/SurfaceScatter20260930V2/FC_APS_Scatter_'+$Family+'.uasset'
    if(!(Test-Path -LiteralPath $palette)){throw 'Required five-mesh palette not baked; editor not started'}
    foreach($p in @('Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h','Source/APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.cpp','Source/APS_ALPHA/Tests/APSPlanetSurfaceScatterTests.cpp')){
        if((Get-Item -LiteralPath ($projectRoot+'/'+$p)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Scatter source newer than DLL: build first'}
    }
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
foreach($source in @('Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.cpp','Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h','Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.cpp','Source/APS_ALPHA/Core/Planetary/APSTerrestrialVegetation.h','Source/APS_ALPHA/Tests/APSTerrestrialVegetationTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Newer Terrestrial vegetation source; build first'}
}
New-Item -ItemType Directory -Path $runDir | Out-Null
if($SurfaceUnification){Save-APSSurfaceUnificationDiagnostic -Selection $surfaceSelection -RunDir $runDir}
if($CanonicalNormalBandwidth){Get-FileHash -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Tests/APSPublishedNormalBandwidthProbe.h') | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/normal-bandwidth-source.json') -Encoding utf8}
if($CloudWeather){Save-APSCloudWeatherDiagnostic -ProjectRoot $projectRoot -RunDir $runDir -Selection $cloudSelection -FeatureScale $CloudFeatureScale}
Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -Recurse -File -Filter '*.uasset' |
    Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll') |
    Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
Get-FileHash -LiteralPath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4/Binaries/Win64/UnrealEditor-WorldScapeCore.dll' |
    Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/native-dll.json') -Encoding utf8
if($Clouds -or $CloudFlight) {
    # BOTH timing legs identify the same immutable baked material. A matched
    # project DLL alone is insufficient when a uasset changed between runs.
    $cloudIdentity=Get-FileHash -LiteralPath $cloudAsset
    [PSCustomObject]@{ObjectPath=$cloudObjectPath;Path=$cloudIdentity.Path;Hash=$cloudIdentity.Hash} |
        ConvertTo-Json | Out-File ($runDir+'/cloud-asset.json') -Encoding utf8
}
$tests='APS.Gameplay.Generation.EmissivePhotosphereShadowPolicy+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics'
if($SurfaceUnification){$tests+='+APS.Contracts.PlanetSurface.MaterialPolicy+APS.Gameplay.World.PlanetSurface.CryogenicGeometry+APS.Gameplay.World.PlanetSurface.OrbitalWaterAppearance+APS.Gameplay.World.PlanetSurface.SharedGeneratedLiquidSelection+APS.Gameplay.World.PlanetSurface.CoastalWater.ReleaseContract'}
if($Isolation -eq 'Published'){$tests+='+APS.Contracts.PlanetSurface.TerrainContinuity+APS.Gameplay.World.PlanetSurface.SurfaceFillContinuity'}
if($TerrestrialPalette){$tests+='+APS.Gameplay.World.PlanetSurface.TerrestrialMaterialPalette'}
if($FoliageFlight){$tests+='+APS.Gameplay.World.PlanetSurface.Foliage+APS.Gameplay.World.PlanetSurface.LandingReliefRecovery'}
if($WaterRelease){$tests+='+APS.Gameplay.World.PlanetSurface.CoastalWater.ReleaseContract+APS.Gameplay.World.PlanetSurface.SharedGeneratedLiquidSelection'}
if($ShoreWaterFactory){$tests+='+APS.Gameplay.World.PlanetSurface.ShoreWater'}
if($Clouds -or $CloudDefault){$tests+='+APS.Gameplay.World.PlanetSurface.Clouds'}
$publicationCVars=$(if($FlightResidency){'aps.Surface.FlightResidency '+[int](-not $ResidencyOff)+',aps.Surface.FlightBaseLodsPerFrame '+$FlightBaseLodsPerFrame+','}else{''})
if($PSBoundParameters.ContainsKey('CollisionHeightOnly')) {
    $publicationCVars+='worldscape.CollisionHeightOnly '+[int][bool]$CollisionHeightOnly+','
}
if($PSBoundParameters.ContainsKey('WalkingCollision')) {
    $publicationCVars+='aps.WorldScapeFoliage.WalkingCollision '+[int][bool]$WalkingCollision+','
}
if($PSBoundParameters.ContainsKey('StructureExclusion')) {$publicationCVars+='aps.WorldScapeFoliage.StructureExclusion '+[int][bool]$StructureExclusion+','}
if($WalkingCollisionDefault){$publicationCVars+='aps.WorldScapeFoliage.WalkingCollision,aps.WorldScapeFoliage.StructureExclusion,'}
if($FoliageDefault -and $Family -eq 'Oasis'){$publicationCVars+='aps.WorldScapeFoliage.OasisTreeAttempts,'}
if($FoliageDefault -and $Family -eq 'Terrestrial'){$publicationCVars+='aps.WorldScapeFoliage.TerrestrialVegetation,'}
if($TerrestrialVegetation){$publicationCVars+='aps.WorldScapeFoliage.TerrestrialVegetation 1,'}
if($PSBoundParameters.ContainsKey('OasisTreeAttempts')){$publicationCVars+=('aps.WorldScapeFoliage.OasisTreeAttempts '+$OasisTreeAttempts+',')}
if($WaterShoreTransmission){$publicationCVars+='r.Water.SingleLayer.VSMFiltering 1,'}
if($ShoreWaterFactory){$publicationCVars+='aps.Surface.ShoreWater 1,r.Water.SingleLayer.VSMFiltering 1,'}
if($FoliageOff){$publicationCVars+='aps.WorldScapeFoliage.Enable 0,aps.WorldScapeFoliage.Prototype 0,'}
if($SurfaceScatter){$publicationCVars+='aps.WorldScapeFoliage.Enable '+[int](-not $FoliageOff)+',aps.WorldScapeFoliage.Prototype 0,aps.WorldScapeFoliage.SurfaceScatter 1,'}
elseif($FoliageFlight -and !$FoliageDefault){$publicationCVars+='aps.WorldScapeFoliage.SurfaceScatter 0,'}
if($Clouds){$publicationCVars+='aps.Surface.Clouds 1,'}
elseif($CloudDefault){$publicationCVars+='aps.Surface.Clouds,'} # Query only; never enable through the test.
elseif($CloudFlight){$publicationCVars+='aps.Surface.Clouds 0,'}
if($CloudWeather){$publicationCVars+='aps.Surface.CloudWeather 1,'}
if($CloudAerialOff){$publicationCVars+='aps.Surface.CloudAerial 0,'}
if($PSBoundParameters.ContainsKey('TerrestrialPalette')) {
    $publicationCVars+='aps.Surface.TerrestrialPalette '+[int][bool]$TerrestrialPalette+','
}
if($PSBoundParameters.ContainsKey('MeshUpdateTasks') -or $PSBoundParameters.ContainsKey('FastMeshCopy')) {
    $publicationCVars+='worldscape.MeshUpdateTasks '+$MeshUpdateTasks+',worldscape.MeshFastCopy '+[int][bool]$FastMeshCopy+','
}
if($PSBoundParameters.ContainsKey('PreparedMesh')) {
    $publicationCVars+='worldscape.PreparedMesh '+[int][bool]$PreparedMesh+',worldscape.PreparedMeshBudgetMiB '+$PreparedBudgetMiB+','
}
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=1000',
    ('-APSDiagnosticPlanet='+$Family),'-APSDiagnosticOrbitOverview',$(if($LodBoundary){'-APSDiagnosticOrbitHeightKm=2.36'}else{'-APSDiagnosticOrbitHeightKm=100'}),
    '-APSProbeOrbitalFieldsFlight',
    ('-ExecCmds="'+$publicationCVars+$(if($FoliagePrototype){'aps.WorldScapeFoliage.Enable 1,aps.WorldScapeFoliage.Prototype 1,'}else{''})+'Automation RunTests '+$tests+'"'),
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/gameplay.log"')
)
switch($Isolation){
    'Atmosphere' {$arguments+='-APSProbeAtmosphereFlight'}
    'KeyShadow' {$arguments+='-APSProbeShadowFlight'}
    'StarShadow' {$arguments+='-APSProbeStarShadowFlight'}
}
if($Isolation -eq 'Published'){$arguments+='-APSProbePublishedTerrainFlight'}
elseif($Isolation -in @('NormalHex','Combined')){
    $arguments+=@('-APSProbeNormalHex','-APSProbeMacroAB');$DefaultAtmosphere=$true
}else{$arguments+='-APSProbeOrbitalFieldsAB'}
if($Isolation -ne 'Published'){$arguments+='-APSDiagnosticTerrainLodAB'}
if($Isolation -eq 'Combined'){$arguments+='-APSProbeContinuityCombined'}
if($DefaultAtmosphere){$arguments+='-APSProbeDefaultAtmosphere'}
if($Daylight){$arguments+='-APSProbeFlightDaylight'}
if($CanonicalNormalBandwidth){$arguments+='-APSProbeCanonicalNormalBandwidth'}
if(($Clouds -or $CloudDefault) -and !$Performance){$arguments+='-APSCloudDiagnostics'}
if($CloudFlight){$arguments+='-APSProbeCloudFlight'}
if($CloudWeatherCandidate){$arguments+='-APSCloudWeatherCandidate'}
if($CloudLayeredCandidate){$arguments+='-APSCloudLayeredCandidate'}
if($PSBoundParameters.ContainsKey('CloudFeatureScale')){$arguments+=('-APSCloudFeatureScale='+$CloudFeatureScale)}
if($CloudWeather){$arguments+='-ini:Engine:[SystemSettings]:aps.Surface.CloudWeather=1'}
if($CloudHorizon){$arguments+='-APSProbeCloudHorizon'}
if($CloudGround){$arguments+='-APSProbeCloudGround'}
if($CloudFlight -and $Performance){$arguments+='-APSProbeCloudFlightPerf'}
if($GroundHold){$arguments+='-APSProbeFlightGroundHold'}
if($FoliageFlight){$arguments+='-APSProbeFoliageFlight'}
if($FoliageOff){$arguments+='-APSProbeFoliageFlightOff'}
if($FoliageReentry){$arguments+='-APSProbeFoliageFlightReentry'}
if($FoliageDefault){$arguments+='-APSProbeFoliageFlightDefault'}
if($PSBoundParameters.ContainsKey('WalkingCollision')){$arguments+=('-APSProbeWalkingCollisionExpected='+[int][bool]$WalkingCollision)}
if($PSBoundParameters.ContainsKey('StructureExclusion')){$arguments+=('-APSProbeStructureExclusionExpected='+[int][bool]$StructureExclusion)}
if($WalkingCollisionDefault){$arguments+='-APSProbeWalkingCollisionExpected=1';$arguments+='-APSProbeStructureExclusionExpected=1'}
if($SurfaceScatter){$arguments+='-APSProbeSurfaceScatterFlight'}
if($PSBoundParameters.ContainsKey('SurfaceSeed')){$arguments+=('-APSProbeFoliageSeed='+$SurfaceSeed)}
if($FoliageFlight -and $Performance){$arguments+='-APSProbeFoliageFlightPerf'}
if($FlightResidency){$arguments+='-APSProbeFlightResidency'}
if($ResidencyOff){$arguments+='-APSProbeFlightResidencyOff'}
if($FlightResidency -and $Performance){$arguments+='-APSProbeFlightResidencyPerf'}
if($LodBoundary){$arguments+='-APSProbeFlightLodBoundary'}
if($NativeLavaFlight){$arguments+='-APSProbeNativeLavaFlight'}elseif($WaterFlight){$arguments+='-APSProbeWaterFlight'}
if($LiquidReference){$arguments+=$(if($NativeLavaFlight){'-APSCoastReferenceLava73875'}else{'-APSCoastReferenceWater487132'})}
if($WaterRelease){$arguments+='-APSProbeWaterRelease';if(!$WaterReleaseDefault){$arguments+='-ini:Engine:[SystemSettings]:aps.Surface.CoastalWater=1'}}
if($WaterFlight){$arguments+=('-APSWaterFlightNearKm='+$WaterNearHeightKm.ToString([Globalization.CultureInfo]::InvariantCulture))}
if($WaterFlight){$arguments+=('-APSWaterABDepthM='+$WaterCoastDepthM.ToString([Globalization.CultureInfo]::InvariantCulture))}
if($WaterFlight){$arguments+=('-APSLiquidFlightPitchDeg='+$LiquidViewPitchDeg.ToString([Globalization.CultureInfo]::InvariantCulture))}
if($PSBoundParameters.ContainsKey('LiquidViewFrame')) {
    # Existing C++ replay path: observer only, no field/geometry/light mutation.
    $keys=@('X','Y','Z','U','V','W')
    for($i=0;$i -lt 6;$i++){$arguments+=('-APSWaterABView'+$keys[$i]+'='+$LiquidViewFrame[$i].ToString('R',[Globalization.CultureInfo]::InvariantCulture))}
}
if($WaterPayload){$arguments+='-APSWaterDepthPayload'}
if($WaterMaterial){$arguments+='-APSProbeWaterFlightMaterial'}
if($WaterShoreTransmission){$arguments+=@('-APSProbeWaterShoreTransmission','-ini:Engine:[SystemSettings]:r.Water.SingleLayer.ShadersSupportVSMFiltering=1,r.Water.SingleLayer.VSMFiltering=1')}
if($ShoreWaterFactory){$arguments+=@('-APSProbeShoreWaterFactory','-ini:Engine:[SystemSettings]:aps.Surface.ShoreWater=1,r.Water.SingleLayer.ShadersSupportVSMFiltering=1,r.Water.SingleLayer.VSMFiltering=1')}
if($WaterSurfaceFilter){$arguments+=@('-APSProbeWaterSurfaceFilter',('-APSWaterSurfaceScaleMultiplier='+$WaterWaveScaleFactor))}
if($WaterSurfacePrecise){$arguments+='-APSProbeWaterSurfacePrecise'}
if($WaterSurfaceAnchored){$arguments+=@('-APSProbeWaterSurfaceRelative','-APSProbeWaterAnalytic','-APSWaterAnchorSplit')}
if($WaterAnchorNoise){$arguments+='-APSWaterAnchorNoise'}
if($WaterSurfacePass){$arguments+=@('-APSProbeWaterSurfacePass','-ini:Engine:[SystemSettings]:r.Water.SingleLayer.ShadersSupportVSMFiltering=1,r.Water.SingleLayer.VSMFiltering=1')}
if($Performance -and $WaterFlight){$arguments+='-APSProbeWaterFlightPerf'}
if($CpuTrace){$arguments+=@('-trace=cpu,frame,bookmark,region,loadtime','-statnamedevents',('-tracefile="'+$runDir+'/water-flight.utrace"'))}
if($SurfaceUnification){
    $arguments+='-APSProbeSurfaceUnificationFlight'
    if($surfaceSelection.Flag){$arguments+=$surfaceSelection.Flag}
}
$arguments -join ' ' | Out-File ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Isolation=$Isolation;DefaultAtmosphere=[bool]$DefaultAtmosphere;FoliagePrototype=[bool]$FoliagePrototype} | ConvertTo-Json
