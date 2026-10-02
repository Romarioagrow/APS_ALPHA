param(
    [ValidatePattern('^[A-Za-z]+$')][string]$Family='Frozen',
    [ValidatePattern('^[A-Za-z]+$')][string[]]$FamilyBatch=@(),
    [ValidatePattern('^[a-z0-9-]+$')][string]$Label='native',
    [ValidateSet(-1,0,1,2)][int]$MacroMode=-1,
    [switch]$WheelSweep,
    [switch]$WarpAB,
    [switch]$SlopeSideAB,
    [switch]$OrbitalFieldsAB,
    [switch]$Combined,
    [switch]$Published,
    [switch]$PublishedDefault,
    [switch]$WaterRelease,
    [switch]$ShoreWaterFactory,
    [switch]$Clouds,
    [switch]$CloudWeather,
    [switch]$CloudWeatherCandidate,
    [switch]$CloudLayeredCandidate,
    [ValidateSet('0.5','1','2')][string]$CloudFeatureScale,
    [switch]$CloudOff,
    [switch]$CloudNoFog,
    [ValidateRange(0,8)][int]$CloudDebug=0,
    [ValidateSet(-1,0,1)][int]$TerrestrialPaletteMode=-1,
    [switch]$LegacyAtmosphereShell,
    [switch]$FamilyScaleAB,
    [switch]$FoliageRegression,
    [switch]$Buffers,
    [switch]$Patterns,
    [switch]$Volumes,
    [switch]$AllFields,
    [switch]$FixedHeight,
    [ValidateRange(640,3840)][int]$Width=1920,
    [ValidateRange(480,2160)][int]$Height=1080,
    [switch]$RouteRegression,
    [ValidateRange(70,20000)][double]$HeightKm=2000
)
$ErrorActionPreference='Stop'
if($CloudWeatherCandidate -and $CloudLayeredCandidate){throw 'Select one cloud candidate: weather V28 or layered V30'}
if($CloudWeatherCandidate -and (!$CloudWeather -or $PublishedDefault)){throw 'CloudWeatherCandidate requires explicit CloudWeather; not a default-acceptance run'}
if($CloudLayeredCandidate -and (!$CloudWeather -or $PublishedDefault)){throw 'CloudLayeredCandidate requires explicit CloudWeather; not a default-acceptance run'}
if($CloudLayeredCandidate -and $CloudDebug -eq 4){throw 'Layered V30 does not implement numeric CloudDebug 4; magenta is unsupported, not probe evidence'}
if($PSBoundParameters.ContainsKey('CloudFeatureScale') -and (!$CloudWeather -or $PublishedDefault -or $CloudNoFog -or $CloudDebug -ne 0 -or $FamilyBatch.Count -or $RouteRegression)){throw 'CloudFeatureScale requires one explicit weather fixture without default/debug/route experiments'}
if($CloudWeather -and (!$Published -or !($Clouds -xor $CloudOff))){throw 'Weather requires published terrain with explicit clouds ON or OFF'}
if($ShoreWaterFactory -and (!$WaterRelease -or !$Published -or $Family -notin @('Terrestrial','Oasis','Water') -or $FamilyBatch.Count)){throw 'Shore factory preview requires one coastal WaterRelease family and Published terrain'}
if($CloudOff -and ($Clouds -or $CloudNoFog -or $CloudDebug -ne 0)){throw 'Explicit cloud OFF cannot mix with cloud ON/debug experiments'}
if($CloudNoFog -and !$Clouds){throw 'Fog isolation requires the cloud diagnostic'}
if($WaterRelease -and !$Published){throw 'Water release needs published terrain without material replacements'}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
if($CloudWeather){
    . (Join-Path $PSScriptRoot 'CloudWeatherDiagnostic.ps1')
    $cloudSelection=Get-APSCloudWeatherDiagnostic -ProjectRoot $projectRoot -Candidate:$CloudWeatherCandidate -LayeredCandidate:$CloudLayeredCandidate
    foreach($p in @('Core/Planetary/APSPlanetCloudWeather.h','Core/Planetary/APSPlanetCloudSettings.h','Core/Planetary/APSPlanetCloudLayers.h','Core/Rendering/APSPlanetCloudComponent.cpp','Tests/APSPlanetCloudWeatherTests.cpp','Tests/APSPlanetCloudLayersTests.cpp','Editor/APSPlanetCloudHlsl.h','Editor/APSPlanetCloudLayeredHlsl.h')){
        if((Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/'+$p)).LastWriteTimeUtc -gt (Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).LastWriteTimeUtc){throw 'Cloud weather source newer than DLL: build first'}
    }
    if(!(Test-Path -LiteralPath $cloudSelection.Asset)){throw 'Bake the selected separate cloud-weather graph first'}
}
if($WaterRelease){
    foreach($p in @('Source/APS_ALPHA/Core/Planetary/APSShoreWaterMaterial.h','Source/APS_ALPHA/Core/Planetary/APSWaterLightingSubsystem.h','Source/APS_ALPHA/Core/Planetary/APSWaterLightingSubsystem.cpp','Source/APS_ALPHA/Core/Planetary/APSWaterSurfaceLighting.h','Source/APS_ALPHA/Tests/APSShoreWaterTests.cpp')) {
        if((Get-Item -LiteralPath ($projectRoot+'/'+$p)).LastWriteTimeUtc -gt (Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).LastWriteTimeUtc){throw 'Newer shore factory/binder; build first'}
    }
    if(!(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1/MI_APS_CoastalWater.uasset'))){throw 'Bake WaterV1 first'}
    foreach($p in @('Source/APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h','Source/APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h','Source/APS_ALPHA/Generation/AstroGenerator.cpp','Source/APS_ALPHA/Generation/PlanetarySurfaceGeneratorStreaming.cpp')){
        if((Get-Item -LiteralPath ($projectRoot+'/'+$p)).LastWriteTimeUtc -gt (Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).LastWriteTimeUtc){throw 'Water release source newer than DLL'}
    }
}
if ($Combined) { $OrbitalFieldsAB=$true }
if ($PublishedDefault -and -not $Published) { throw 'PublishedDefault requires Published; no override will be applied' }
if ($TerrestrialPaletteMode -ge 0) {
    if (-not $Published -or $PublishedDefault -or $Family -ne 'Terrestrial' -or $FamilyBatch.Count -or $LegacyAtmosphereShell) {
        throw 'Palette pair requires a single published Terrestrial and ordinary atmosphere, without PublishedDefault'
    }
    foreach ($paletteSource in @('Source/APS_ALPHA/Core/Planetary/APSTerrestrialMaterialPalette.h','Source/APS_ALPHA/Core/Planetary/APSNativeTerrainMaterial.h','Source/APS_ALPHA/Generation/PlanetarySurfaceGeneratorStreaming.cpp','Source/APS_ALPHA/Tests/APSTerrestrialMaterialPaletteTests.cpp')) {
        if ((Get-Item -LiteralPath ($projectRoot+'/'+$paletteSource)).LastWriteTimeUtc -gt (Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).LastWriteTimeUtc) { throw 'Palette source newer than DLL: build first' }
    }
}
if ($Published -and ($Combined -or $WarpAB -or $SlopeSideAB -or $OrbitalFieldsAB -or $Buffers -or $Patterns -or $Volumes -or $AllFields -or $MacroMode -ne -1)) { throw 'Published route cannot mix with material diagnostics' }
foreach ($probePath in @('Source/APS_ALPHA/Tests/APSPlanetTerrainLodABProbe.h','Source/APS_ALPHA/Tests/APSPlanetRefinementRenderedTests.cpp')) {
    if ((Get-Item -LiteralPath ($projectRoot+'/'+$probePath)).LastWriteTimeUtc -gt (Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).LastWriteTimeUtc) {
        throw 'Known newer menu probe source: build before launch'
    }
}
if ($FamilyBatch.Count) {
    if ($PSBoundParameters.ContainsKey('Family') -or (-not $OrbitalFieldsAB -and -not $Published) -or -not $FamilyScaleAB) {
        throw 'Batch requires OrbitalFieldsAB + FamilyScaleAB, and no single Family'
    }
    if (@($FamilyBatch | Sort-Object -Unique).Count -ne $FamilyBatch.Count) { throw 'Duplicate batch type' }
    $probeSource=Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Tests/APSPlanetRefinementRenderedTests.cpp')
    if ($probeSource.LastWriteTimeUtc -gt (Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).LastWriteTimeUtc) {
        throw 'Known newer batch probe source: build before launch'
    }
    $Family='matrix'
}
if ($FamilyScaleAB) {
    if (-not $OrbitalFieldsAB -and -not $Published) { throw 'Family scale views require OrbitalFieldsAB or Published' }
    $HeightKm=20000
}
if (([int][bool]$SlopeSideAB + [int][bool]$WarpAB + [int][bool]$OrbitalFieldsAB) -gt 1) { throw 'Select one candidate' }
if ($OrbitalFieldsAB) { $WarpAB=$true }
if ($SlopeSideAB) { $WarpAB=$true }
if ($AllFields) { $Volumes=$true }
if ($Volumes) { $Patterns=$true }
if ($WarpAB -and ($WheelSweep -or $MacroMode -ne -1)) { throw 'Warp A/B cannot mix with another candidate or wheel sweep' }
if ($Buffers -and ($WarpAB -or $WheelSweep -or $MacroMode -ne -1)) { throw 'Buffer isolation needs only fixed native views' }
if ($Patterns -and ($Buffers -or $WarpAB -or $WheelSweep -or $MacroMode -ne -1)) { throw 'Pattern isolation needs only fixed native views' }
if ($FixedHeight -and $WheelSweep) { throw 'Fixed height cannot mix with wheel sweep' }
if ($WarpAB) {
    $candidateTypes=if($FamilyBatch.Count){$FamilyBatch}else{@($Family)}
    foreach($candidateType in $candidateTypes) {
    if ($Combined -and $candidateType -in @('Volcanic','Melted','Lava')) { throw 'Combined candidate has no audited Magma permutation' }
    $candidateFolder=if ($OrbitalFieldsAB) { 'OrbitalFields20260929V3' } elseif ($SlopeSideAB) { 'LodSlopeSide20260929' } else { 'LodWarpPixel20260929' }
    $candidateInstance='MI_APS_LodPixelTerra'
    # Preflight matches ResolveArchetype. The runtime probe independently selects
    # the actual bound parent and still verifies every static switch/volume source.
    if ($OrbitalFieldsAB -and $candidateType -in @('Volcanic','Melted','Lava')) {
        $candidateFolder='OrbitalFields20260929MagmaV3'
        $candidateInstance='MI_APS_LodPixelMagma'
    }
    $candidateMaster='M_APS_LodPixelTerrain'
    if ($Combined) {
        $candidateFolder='ContinuityCombined20260929V1'
        $candidateInstance='MI_APS_NormalWarpTerra'
        $candidateMaster='M_APS_NormalWarpTerrain'
    }
    foreach ($assetName in @($candidateMaster,$candidateInstance)) {
        $assetFile=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'+$candidateFolder+'/'+$assetName+'.uasset'
        if (-not (Test-Path -LiteralPath $assetFile -PathType Leaf)) { throw ('Candidate is not baked; editor not started: '+$assetFile) }
    }
    }
}
if (Get-Process UnrealEditor,UnrealEditor-Cmd,ShaderCompileWorker,cl,link -ErrorAction SilentlyContinue) {
    throw 'Editor/compiler active; no session touched'
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/'+$Family.ToLowerInvariant()+'-'+$Label
if (Test-Path -LiteralPath $runDir) { throw 'Evidence exists; use a fresh label' }
New-Item -ItemType Directory -Path $runDir | Out-Null
if($CloudWeather){Save-APSCloudWeatherDiagnostic -ProjectRoot $projectRoot -RunDir $runDir -Selection $cloudSelection -FeatureScale $CloudFeatureScale}
$protected=Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -File -Recurse -Filter '*.uasset' |
    Get-FileHash -Algorithm SHA256 | Select-Object Path,Hash
$protected | ConvertTo-Json | Out-File -LiteralPath ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll') |
    Select-Object Path,Hash | ConvertTo-Json | Out-File -LiteralPath ($runDir+'/dll.json') -Encoding utf8
$tests='APS.Contracts.PlanetSurface.RenderedProbeFamily+APS.Preview.Editor.PlanetWheelContinuity+APS.Preview.Editor.PlanetMoonZoomBounds+APS.Rendered.PlanetRefinement.CausalLayers'
$tests+='+APS.Preview.Atmosphere.InsideOutsideShell+APS.Materials.SharedTerrain.LivingBiomeTransfer'
if ($Published) { $tests+='+APS.Contracts.PlanetSurface.TerrainContinuity' }
if ($CloudWeather) { $tests+='+APS.Gameplay.World.PlanetSurface.Clouds.Weather' }
if ($CloudLayeredCandidate) { $tests+='+APS.Gameplay.World.PlanetSurface.Clouds.Layers' }
if($ShoreWaterFactory){$tests+='+APS.Gameplay.World.PlanetSurface.ShoreWater'}
if ($TerrestrialPaletteMode -ge 0) { $tests+='+APS.Gameplay.World.PlanetSurface.TerrestrialMaterialPalette' }
if ($FamilyBatch.Count) {
    $tests=$tests.Replace('APS.Rendered.PlanetRefinement.CausalLayers','APS.Rendered.PlanetRefinement.OrbitalFieldsFamily')
    [pscustomobject]@{Families=@($FamilyBatch);ViewsPerFamily=$(if($Published){6}else{12})} | ConvertTo-Json | Out-File ($runDir+'/family-batch.json') -Encoding utf8
}
if ($FoliageRegression) { $tests+='+APS.Gameplay.World.PlanetSurface.Foliage' }
if ($RouteRegression) { $tests+='+APS.Rendered.MainMenu.ContinuousPhysicalRoute' }
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'), '/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=1000',
    ('-APSPlanetProbeFamily='+$Family),
    ('-APSProbeViewportWidth='+$Width), ('-APSProbeViewportHeight='+$Height),
    ('-UserDir="'+$runDir+'"'),
    ('-ExecCmds="'+$(if($TerrestrialPaletteMode -ge 0){'aps.Surface.TerrestrialPalette '+$TerrestrialPaletteMode+','}else{''})+$(if($LegacyAtmosphereShell){'aps.Preview.AtmosphereInterior 0,'}else{''})+$(if($PublishedDefault){''}else{'aps.Surface.TerrainContinuity '+$(if($Published){'1'}else{'0'})+','})+'Automation RunTests '+$tests+'"'),
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'), ('-abslog="'+$runDir+'/menu.log"')
)
if ($WarpAB) {
    $arguments+=@('-APSProbeTerrainLodAB',$(if ($OrbitalFieldsAB) { '-APSProbeTerrainOrbitalFields' } elseif ($SlopeSideAB) { '-APSProbeTerrainSlopeSide' } else { '-APSProbeTerrainWarpOnly' }),('-APSPlanetProbeHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture)))
} else {
    $arguments+='-APSProbeTerrainNativeViews'
    if ($Buffers -or $Patterns -or $FixedHeight -or $FamilyScaleAB) {
        $arguments+=('-APSPlanetProbeHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture))
        if ($Buffers) { $arguments+='-APSProbeTerrainBuffers' }
        if ($Patterns) { $arguments+= $(if ($AllFields) { '-APSProbeTerrainAllFields' } elseif ($Volumes) { '-APSProbeTerrainVolumes' } else { '-APSProbeTerrainPatterns' }) }
    } elseif ($WheelSweep) { $arguments+='-APSProbeTerrainWheelSweep' } else { $arguments+='-APSProbeTerrainZoomSweep' }
}
if ($MacroMode -ge 0) { $arguments+=('-APSProbeTerrainMacroMode='+$MacroMode) }
if ($FamilyScaleAB) { $arguments+='-APSProbeTerrainFamilyScaleAB' }
if ($Combined) { $arguments+='-APSProbeTerrainCombined' }
if ($Published) { $arguments+='-APSProbeTerrainPublishedViews' }
if ($FamilyBatch.Count) {
    $arguments+=@('-APSProbeTerrainFamilyBatch',('-APSProbeTerrainBatchFamilies='+($FamilyBatch -join ',')))
}
if($WaterRelease){$arguments+='-ini:Engine:[SystemSettings]:aps.Surface.CoastalWater=1'}
if($ShoreWaterFactory){
    $arguments+='-APSProbeShoreWaterFactory'
    $arguments+='-ini:Engine:[SystemSettings]:aps.Surface.ShoreWater=1,r.Water.SingleLayer.ShadersSupportVSMFiltering=1,r.Water.SingleLayer.VSMFiltering=1'
    $arguments=@($arguments|ForEach-Object{if($_.StartsWith('-ExecCmds="')){$_.Replace('-ExecCmds="','-ExecCmds="aps.Surface.ShoreWater 1,r.Water.SingleLayer.VSMFiltering 1,')}else{$_}})
}
if($CloudOff){$arguments+='-ini:Engine:[SystemSettings]:aps.Surface.Clouds=0'}
if($CloudWeatherCandidate){$arguments+='-APSCloudWeatherCandidate'}
if($CloudLayeredCandidate){$arguments+='-APSCloudLayeredCandidate'}
if($PSBoundParameters.ContainsKey('CloudFeatureScale')){$arguments+=('-APSCloudFeatureScale='+$CloudFeatureScale)}
if($CloudWeather){
    $arguments+='-ini:Engine:[SystemSettings]:aps.Surface.CloudWeather=1'
    $arguments=@($arguments|ForEach-Object{if($_.StartsWith('-ExecCmds="')){$_.Replace('-ExecCmds="','-ExecCmds="aps.Surface.CloudWeather 1,')}else{$_}})
}
if($Clouds){
    $arguments+=@('-ini:Engine:[SystemSettings]:aps.Surface.Clouds=1','-APSCloudDiagnostics')
    $arguments=@($arguments | ForEach-Object { if($_.StartsWith('-ExecCmds="')){$_.Replace('-ExecCmds="',('-ExecCmds="aps.Surface.CloudDebug '+$CloudDebug+','))}else{$_} })
}
if($CloudNoFog){$arguments=@($arguments|ForEach-Object{if($_.StartsWith('-ExecCmds="')){$_.Replace('-ExecCmds="','-ExecCmds="ShowFlag.Fog 0,')}else{$_}})}
$arguments -join ' ' | Out-File -LiteralPath ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;MacroMode=$MacroMode} | ConvertTo-Json
