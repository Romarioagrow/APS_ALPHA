param(
    [ValidatePattern('^[a-z0-9-]+$')][string]$Label='cloud-gas-contracts',
    [ValidateSet('All','GasAtmosphere','Batch','UnifiedLavaRHI','UnifiedLavaPreparationRHI','UnifiedLavaLifecycleRHI')][string]$Group='All',
    [switch]$LavaCompileDiagnostic
)
# Real Unreal automation. UnifiedLavaRHI uses the real shader backend, but does
# not capture a scene; no group in this helper establishes visual quality.
$ErrorActionPreference='Stop'
if($LavaCompileDiagnostic -and $Group -ne 'UnifiedLavaRHI'){
    throw 'LavaCompileDiagnostic requires the isolated UnifiedLavaRHI group'
}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){
    throw 'Editor/compiler active; no session touched'
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
$dll=Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
$sources=@(
    'UI/MainMenu/APSAtmosphereControlBounds.h','UI/MainMenu/WorldGenerationViewModel.cpp',
    'UI/MainMenu/SWorldGenerationPanel.cpp','Generation/AstroGeneratorBodyOverrides.cpp',
    'Tests/APSGasAtmosphereControlBoundsTests.cpp','Tests/APSGasMaterialSeedTests.cpp',
    'Generation/AstroGenerator.cpp','Core/Planetary/APSGasGiantMaterial.h',
    'Core/Planetary/APSPlanetCloudWeather.h',
    'Core/Rendering/APSPlanetCloudComponent.cpp','Tests/APSPlanetCloudWeatherTests.cpp',
    'Core/Planetary/APSWaterLightingSubsystem.cpp','Tests/APSShoreWaterTests.cpp',
    'Tests/APSPlanetSurfaceProfileTests.cpp')
if($Group -eq 'UnifiedLavaRHI'){
    $sources+=@('Core/Planetary/APSUnifiedLavaSurface.h',
        'Core/Planetary/APSSharedTerrainMaterial.h','Tests/APSUnifiedLavaFactoryTests.cpp')
}
if($Group -in @('Batch','UnifiedLavaPreparationRHI','UnifiedLavaLifecycleRHI')){
    $sources+=@('Core/Planetary/APSUnifiedLavaMaterialPreparation.h',
        'Core/Planetary/APSUnifiedLavaSurface.h','Tests/APSUnifiedLavaPreparationTests.cpp',
        'Generation/PlanetarySurfaceGenerator.h','Generation/PlanetarySurfaceGenerator.cpp',
        'Generation/PlanetarySurfaceGeneratorStreaming.cpp')
}
if($Group -eq 'UnifiedLavaLifecycleRHI'){
    $sources+=@('Tests/APSUnifiedLavaGeneratorLifecycleTests.cpp',
        'Generation/AstroGenerator.cpp','Actors/Astro/PlanetaryBodyStreaming.cpp')
}
if($Group -eq 'Batch'){
    $sources+=@(
        'Tests/APSFoliageCollisionTests.cpp','Tests/APSFoliageExclusionTests.cpp',
        'Tests/APSFoliageLeafMaterialTests.cpp','Tests/APSPlanetSurfaceScatterTests.cpp',
        'Tests/APSTerrestrialVegetationTests.cpp','Tests/APSWorldScapeFoliagePolicyTests.cpp',
        'Core/Planetary/APSFoliageCollisionComponent.h','Core/Planetary/APSFoliageCollisionComponent.cpp',
        'Core/Planetary/APSFoliageExclusionComponent.h','Core/Planetary/APSFoliageExclusionComponent.cpp',
        'Core/Planetary/APSFoliageLeafMaterial.h','Core/Planetary/APSPlanetSurfaceScatter.h',
        'Core/Planetary/APSPlanetScatterMaterial.h','Core/Planetary/APSPlanetFoliagePrototype.h',
        'Core/Planetary/APSTerrestrialVegetation.h','Core/Planetary/APSWorldScapeFoliagePolicy.h',
        'Core/Planetary/APSWorldScapeFoliagePolicy.cpp',
        'Tests/APSCoastalReliefTests.cpp','Tests/APSWorldScapeLiquidLatticeTests.cpp',
        'Tests/APSWaterDepthPaletteTests.cpp','Tests/APSWaterSurfaceFilterTests.cpp',
        'Tests/APSSharedGeneratedLiquidSelectionTests.cpp','Tests/APSWorldScapeSurfaceEnvelopeTests.cpp',
        'Tests/APSUnifiedLavaFactoryTests.cpp',
        'Core/Planetary/APSCoastalRelief.h','Core/Planetary/APSWorldScapeLiquidLattice.h',
        'Core/Planetary/APSWaterDepthPalette.h','Core/Planetary/APSWaterSurfaceFilter.h',
        'Core/Planetary/APSWaterAnalyticWaves.h','Core/Planetary/APSSharedGeneratedLiquidMaterial.h',
        'Core/Planetary/APSCoastalWaterMaterial.h','Core/Planetary/APSShoreWaterMaterial.h',
        'Core/Planetary/APSUnifiedLavaSurface.h','Core/Planetary/APSWorldScapeSurfaceEnvelope.h',
        'Core/Planetary/APSPlanetSurfaceProfile.cpp',
        'Generation/APSWorldScapePlanetNoise.h','Generation/APSWorldScapePlanetNoise.cpp')
}
foreach($source in $sources){
    if((Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){
        throw ('Source newer than DLL; build before testing: '+$source)
    }
}
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/contracts-'+$Label
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-FileHash -LiteralPath $dll.FullName | Select-Object Path,Hash | ConvertTo-Json |
    Out-File -LiteralPath ($runDir+'/dll.json') -Encoding utf8
$tests='APS.UI.Generation.GasAtmosphere+APS.UI.Generation.GasMaterialSeed'
if($Group -eq 'UnifiedLavaRHI'){
    $tests='APS.Gameplay.World.PlanetSurface.UnifiedLava.Factory.FreshLoadRHI'
}
if($Group -eq 'UnifiedLavaPreparationRHI'){
    # A separate fresh process: never warm the template via the historical
    # strict-factory diagnostic before testing production demand preparation.
    $tests='APS.Gameplay.World.PlanetSurface.UnifiedLava.Preparation.FreshLoadRHI'
}
if($Group -eq 'UnifiedLavaLifecycleRHI'){
    $tests='APS.Gameplay.World.PlanetSurface.UnifiedLava.Generator.FreshLifecycleRHI'
}
if($Group -in @('All','Batch')){
    $tests+='+APS.Gameplay.World.PlanetSurface.Clouds.Weather+APS.Gameplay.World.PlanetSurface.ShoreWater'
    $tests+='+APS.Gameplay.World.PlanetSurface.AtmosphereDeterministicVariation'
}
if($Group -eq 'Batch'){
    # Existing non-rendering contracts only. Foliage includes ScatterAssetCoverage;
    # do not pull flight/render probes into this NullRHI queue.
    $tests+='+APS.Gameplay.World.PlanetSurface.Foliage'
    $tests+='+APS.Gameplay.World.PlanetSurface.CoastalRelief'
    $tests+='+APS.Gameplay.World.PlanetSurface.LiquidLattice'
    $tests+='+APS.Gameplay.World.PlanetSurface.WaterDepth.PaletteBounds'
    $tests+='+APS.Gameplay.World.PlanetSurface.WaterSurface.FootprintContract'
    $tests+='+APS.Gameplay.World.PlanetSurface.WaterSurface.AnalyticKernelContract'
    $tests+='+APS.Gameplay.World.PlanetSurface.SharedGeneratedLiquidSelection'
    $tests+='+APS.Gameplay.World.PlanetSurface.CoastalWater.ReleaseContract'
    $tests+='+APS.Gameplay.World.PlanetSurface.UnifiedLavaFamilyMatrix'
    # Only CPU rejection guards belong in NullRHI. FreshLoadRHI must run alone
    # in a new RHI process before anything loads the saved candidate material.
    $tests+='+APS.Gameplay.World.PlanetSurface.UnifiedLava.Factory.InvalidInputs'
    $tests+='+APS.Gameplay.World.PlanetSurface.UnifiedLava.Preparation.InvalidInputs'
}
$rhiArguments=if($Group -in @('UnifiedLavaRHI','UnifiedLavaPreparationRHI','UnifiedLavaLifecycleRHI')){@('-d3d12','-sm6','-RenderOffscreen')}else{@('-NullRHI')}
if($LavaCompileDiagnostic){$rhiArguments+='-APSUnifiedLavaCompileDiagnostic'}
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Engine/Maps/Entry',
    '-unattended','-nop4','-nosound','-nosplash','-NoLiveCoding',
    '-ddc=InstalledNoZenLocalFallback',('-UserDir="'+$runDir+'"'),
    ('-ExecCmds="Automation RunTests '+$tests+'"'),'-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'),('-abslog="'+$runDir+'/tests.log"')
)+$rhiArguments
$arguments -join ' ' | Out-File -LiteralPath ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
    -ArgumentList $arguments -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
$process.PriorityClass='BelowNormal'
[pscustomobject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Group=$Group}|ConvertTo-Json
