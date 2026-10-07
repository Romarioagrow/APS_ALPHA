param(
    [Parameter(Mandatory)][ValidateSet('Lidim','Jaim')][string]$Reference,
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$CanonicalChartAB,
    [switch]$CanonicalCoverage,
    [switch]$CoverageFar,
    [switch]$Daylight,
    [switch]$FlatNormals,
    [ValidateSet(2,4)][int]$DenseHeight,
    [switch]$CanonicalSlopeAB,
    [ValidatePattern('^[0-9A-Fa-f]{40}$')][string]$SlopeFunctionSHA1,
    [ValidatePattern('^[0-9A-Fa-f]{40}$')][string]$CoverageMasterSHA1,
    [switch]$WarpPixel,
    [switch]$AtmosphereBoundary,
    [switch]$AtmosphereGround,
    [ValidateSet('Control','Candidate')][string]$AtmosphereTail,
    [switch]$CheckOnly
)
$ErrorActionPreference='Stop'
if($Daylight -and !$CoverageFar){throw 'Frozen Daylight requires the explicit CoverageFar route'}
if($FlatNormals -and (!$CanonicalCoverage -or !$CoverageFar -or !$Daylight -or $DenseHeight -or $CanonicalSlopeAB)){throw 'FlatNormals requires standalone canonical CoverageFar + Daylight; no DenseHeight or slope comparison'}
if($DenseHeight -and (!$CanonicalCoverage -or !$CoverageFar -or !$Daylight -or $CanonicalSlopeAB)){throw 'DenseHeight requires standalone CanonicalCoverage + CoverageFar + Daylight; no slope comparison'}
if($CanonicalSlopeAB -and (!$CanonicalCoverage -or $Reference -ne 'Lidim' -or $CoverageFar -or $CanonicalChartAB -or $WarpPixel -or $AtmosphereBoundary -or $AtmosphereGround -or $AtmosphereTail)){throw 'CanonicalSlopeAB requires standalone Lidim canonical coverage static comparison'}
if($CoverageFar -and ($Reference -ne 'Lidim' -or $CanonicalChartAB -or $WarpPixel -or $AtmosphereBoundary -or $AtmosphereGround -or $AtmosphereTail)){throw 'CoverageFar requires standalone Lidim original/canonical far-orbit comparison'}
if($CanonicalCoverage -and ($Reference -ne 'Lidim' -or $CanonicalChartAB -or $WarpPixel -or $AtmosphereBoundary -or $AtmosphereGround -or $AtmosphereTail)){throw 'CanonicalCoverage requires standalone Lidim motion lighting comparison'}
if($CanonicalChartAB -and ($Reference -ne 'Lidim' -or $WarpPixel -or $AtmosphereBoundary -or $AtmosphereGround -or $AtmosphereTail)){throw 'CanonicalChartAB requires standalone Lidim static lighting comparison'}
if($WarpPixel -and $AtmosphereBoundary){throw 'Atmosphere boundary and warp-pixel are separate diagnostic comparisons'}
if($AtmosphereTail -and (!$AtmosphereBoundary -or $WarpPixel)){throw 'AtmosphereTail requires the separate AtmosphereBoundary route'}
if($AtmosphereGround -and (!$AtmosphereBoundary -or $WarpPixel)){throw 'AtmosphereGround requires AtmosphereBoundary without WarpPixel; omit AtmosphereTail to inspect the ordinary installed material'}
$projectRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if($CanonicalSlopeAB){
    if(!$SlopeFunctionSHA1 -or $SlopeFunctionSHA1 -eq '3FF327494FFB83C3521A5BA1318FA913C3303AC2'){throw 'Slope requires explicitly cold-verified candidate function SHA1'}
    $slopeFunction=Join-Path $projectRoot 'Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407.uasset'
    if((Get-FileHash -LiteralPath $slopeFunction -Algorithm SHA1).Hash -ne $SlopeFunctionSHA1){throw 'Slope function differs from verified candidate; no Unreal launch'}
}elseif($SlopeFunctionSHA1){throw 'SlopeFunctionSHA1 requires CanonicalSlopeAB'}
if($CanonicalCoverage){
    if(!$CoverageMasterSHA1 -or $CoverageMasterSHA1 -eq '9541C8FB506D3F95E277D97C2FB1E7D626A420B0'){throw 'Coverage requires an explicitly verified successfully baked candidate SHA1'}
    $coverageMaster=Join-Path $projectRoot 'Content/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.uasset'
    if((Get-FileHash -LiteralPath $coverageMaster -Algorithm SHA1).Hash -ne $CoverageMasterSHA1){throw 'Coverage candidate asset does not match verified bake; no Unreal launch'}
}elseif($CoverageMasterSHA1){throw 'CoverageMasterSHA1 is only valid with CanonicalCoverage'}
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/frozen-descent-'+$Label
$dll=Get-Item -LiteralPath (Join-Path $projectRoot 'Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
$probeSources=@(
    'Tests/APSFrozenDescentProbe.h',
    'Tests/APSCanonicalCoverageLayout.h',
    'Tests/APSCanonicalFilteredHeight.h',
    'Tests/APSCanonicalCoverageTexture.h',
    'Generation/APSCanonicalCoverageLayout.h',
    'Generation/APSCanonicalFilteredHeight.h',
    'Core/Planetary/APSCanonicalCoverageTexture.h',
    'Core/Planetary/APSPlanetReliefRuntime.h',
    'Core/Planetary/APSPlanetReliefRuntime.cpp',
    'Tests/APSCanonicalCoverageFlight.h',
    'Tests/APSCanonicalCoverageFar.h',
    'Tests/APSCanonicalSlopeAB.h',
    'Editor/APSCanonicalPixelSlopeUpdate.h',
    'Editor/APSCanonicalPixelSlopePublisher.h',
    'Editor/APSCanonicalCoverageUpdate.h',
    'Tests/APSCanonicalReliefChart.h',
    'Tests/APSCanonicalReliefChartAB.h',
    'Editor/APSCanonicalReliefChartUpdate.h',
    'Editor/APSOrbitalReliefLightingPublisher.h',
    'Core/Rendering/APSAtmosphereTailMaterial.h',
    'Generation/PlanetarySurfaceGenerator.cpp',
    'Generation/PlanetarySurfaceGenerator.h',
    'Tests/APSFrozenDescentProbeTests.cpp',
    'Tests/APSPublishedTerrainDescentAudit.h',
    'Tests/APSGeneratedGameplayHandoffSmokeTests.cpp',
    'Core/World/APSWorldShiftEvents.h',
    'Core/World/APSWorldShiftEvents.cpp',
    'Core/World/APSWorldOriginSubsystem.cpp',
    'Core/Planetary/APSSharedTerrainMaterial.h',
    'Core/Rendering/APSPlanetCloudComponent.cpp',
    'Tests/APSWorldShiftMaterialFrameTests.cpp',
    'Tests/APSContinuousWarpPixelProbe.h',
    'Tests/APSContinuousWarpPixelAssets.h',
    'Editor/APSContinuousWarpPixelABBuilder.h',
    'Tests/APSAtmosphereTailProbe.h',
    'Tests/APSAtmosphereTailProbeTests.cpp',
    'Tests/APSAtmosphereTailAssets.h',
    'Editor/APSAtmosphereTailABBuilder.h'
)
foreach($relative in $probeSources){
    $file=Get-Item -LiteralPath (Join-Path $projectRoot ('Source/APS_ALPHA/'+$relative))
    if($file.LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw "Frozen descent probe newer than DLL: $relative. Build in a coordinated free window first."}
}
. (Join-Path $PSScriptRoot 'SurfaceUnificationDiagnostic.ps1')
# Candidate here means observing the ordinary unified default, not enabling an
# experimental material. No terrain/cloud/normal/streaming cvar is overridden.
$selection=Get-APSSurfaceUnificationDiagnostic -ProjectRoot $projectRoot -Mode Candidate
if($CheckOnly){'PASS: source/DLL preflight only; no Unreal started, no rendered acceptance.'; return}
if(Get-Process UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; no process touched'}
$builds=Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool|AutomationTool'
}
if($builds){throw 'Unreal build automation active; no process touched'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; choose a new Label'}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')

$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=1000',
    '-APSDiagnosticPlanet=Frozen','-APSDiagnosticOrbitOverview','-APSDiagnosticOrbitHeightKm=30.5',
    '-APSProbeOrbitalFieldsFlight','-APSProbePublishedTerrainFlight','-APSProbeDefaultAtmosphere',
    ('-APSProbeFrozenDescent='+$Reference),
    '-ExecCmds="Automation RunTests APS.Contracts.FrozenDescent+APS.World.Origin.MaterialFrames+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/gameplay.log"')
)
New-Item -ItemType Directory -Path $runDir | Out-Null
if($CanonicalChartAB){$arguments += '-APSProbeCanonicalChartAB'}
if($CanonicalCoverage){$arguments += '-APSProbeCanonicalCoverageFlight'}
if($CoverageFar){$arguments += '-APSProbeCanonicalCoverageFar'}
if($Daylight){$arguments += '-APSProbeFlightDaylight'}
if($FlatNormals){$arguments += '-APSProbeCanonicalFlatNormals'}
if($DenseHeight){$arguments += ('-APSProbeCanonicalDenseHeight='+$DenseHeight)}
if($CanonicalSlopeAB){$arguments += '-APSProbeCanonicalSlopeAB'}
if($WarpPixel){$arguments += '-APSProbeContinuousWarpPixel'}
if($AtmosphereBoundary){$arguments += '-APSProbeFrozenAtmosphereBoundary'}
if($AtmosphereGround){$arguments += '-APSProbeFrozenAtmosphereGround'}
if($AtmosphereTail){$arguments += ('-APSProbeAtmosphereTail='+$(if($AtmosphereTail -eq 'Candidate'){'1'}else{'0'}))}
Save-APSSurfaceUnificationDiagnostic -Selection $selection -RunDir $runDir
$nativeRoot='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$nativeFiles=@('Binaries/Win64/UnrealEditor-WorldScapeCore.dll',
    'Source/WorldScapeCore/Private/WorldScapeRoot_Main.cpp',
    'Source/WorldScapeCore/Private/WorldScapeRoot_Thread.cpp',
    'Source/WorldScapeCore/Private/WorldScapeLod.cpp') | ForEach-Object {Join-Path $nativeRoot $_}
[pscustomobject]@{
    Schema=2; Reference=$Reference; RunnerChangesProductionSettings=$false; DiagnosticWarpPixelCandidate=[bool]$WarpPixel
    CanonicalChartStaticAB=[bool]$CanonicalChartAB
    CanonicalCoverageFlight=[bool]$CanonicalCoverage
    CanonicalCoverageFar=[bool]$CoverageFar
    DaylightObserverOnly=[bool]$Daylight
    AuthoredNormalTexturesFlatOnly=[bool]$FlatNormals
    DenseHeightOversample=$DenseHeight
    CanonicalHeightFilter=$(if(!$CanonicalCoverage){'NotRequested'}elseif($DenseHeight){'SharedLattice'}else{'Quadrature'})
    CanonicalComparison=$(if(!$CanonicalCoverage){'NotRequested'}elseif($DenseHeight){'coarsest S2vsS4 and Q3vsS4; evidence only'}else{'legacy level1 Q3vsQ5'})
    CanonicalSlopeStaticAB=[bool]$CanonicalSlopeAB
    SlopeFunctionSHA1=$SlopeFunctionSHA1
    CoverageMasterSHA1=$CoverageMasterSHA1
    ProductionHeightAtmosphereBoundary=[bool]$AtmosphereBoundary
    ProductionHeightAtmosphereGround=[bool]$AtmosphereGround
    AtmosphereTailDiagnostic=$AtmosphereTail
    ProbeSourceHashes=@($probeSources | ForEach-Object {Join-Path $projectRoot ('Source/APS_ALPHA/'+$_)} | Get-FileHash -Algorithm SHA256 | Select-Object Path,Hash)
    NativeHashes=@(Get-FileHash -LiteralPath $nativeFiles -Algorithm SHA256 | Select-Object Path,Hash)
    Description='Seed/radius/type body fixture with resolved noise verification; not exact saved moon, atmosphere, camera, ship dynamics or full-frame video replay. Ordinary materials and live LOD; rendered captures and readback must be inspected.'
    VisualAcceptance=$false
} | ConvertTo-Json -Depth 5 | Out-File -LiteralPath (Join-Path $runDir 'descent.json') -Encoding utf8
$arguments -join ' ' | Out-File -LiteralPath (Join-Path $runDir 'command.txt') -Encoding utf8
# Recheck immediately before launch; coordination ownership must also have been
# confirmed by the caller. The script never closes or kills another process.
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler appeared during preflight; evidence retained, no Unreal started'}
if($CanonicalCoverage -and (Get-FileHash -LiteralPath $coverageMaster -Algorithm SHA1).Hash -ne $CoverageMasterSHA1){throw 'Coverage candidate changed during preflight; no Unreal launch'}
if($CanonicalSlopeAB -and (Get-FileHash -LiteralPath $slopeFunction -Algorithm SHA1).Hash -ne $SlopeFunctionSHA1){throw 'Slope function changed during preflight; no Unreal launch'}
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $runDir 'stdout.txt') -RedirectStandardError (Join-Path $runDir 'stderr.txt')
[pscustomobject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Reference=$Reference;VisualAcceptance=$false} | ConvertTo-Json
