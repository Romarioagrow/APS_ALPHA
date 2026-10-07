param(
    [Parameter(Mandatory)][ValidateSet('GasGiant','ScatterMaterial','SurfaceScatter','Clouds','MagmaFields','Foliage','FoliageLeaf','MacroApproach','NormalMacroWarp','NormalHex','ContinuityCombined','ContinuityRelease','WaterDepthFiltered','WaterSurface','WaterSurfacePass','WaterSurfacePrecise','WaterSurfaceRelative','WaterAnalytic','WaterDomainAudit','UnifiedLava','UnifiedLavaDetail')][string]$Candidate,
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$FinalNormalAudit,
    [switch]$SecondaryDomainAudit,
    [switch]$AnchorSplit,
    [switch]$AnchorNoise,
    [switch]$WaterRelease,
    [switch]$WaterShoreTransmission,
    [switch]$CloudWeatherCandidate,
    [switch]$CloudLayeredCandidate,
    [switch]$CloudTwoCrossingCandidate
)
$ErrorActionPreference='Stop'
if($CloudWeatherCandidate -and $CloudLayeredCandidate){throw 'Select one cloud candidate: refined V31 or layered V30'}
if($CloudWeatherCandidate -and $Candidate -ne 'Clouds'){throw 'CloudWeatherCandidate requires Candidate Clouds'}
if($CloudLayeredCandidate -and $Candidate -ne 'Clouds'){throw 'CloudLayeredCandidate requires Candidate Clouds'}
# Rio 06.10 (clouds vanish at an altitude): V33 two-crossing cloud graph, exclusive with V30/V31.
if($CloudTwoCrossingCandidate -and ($CloudWeatherCandidate -or $CloudLayeredCandidate)){throw 'Select one cloud candidate: two-crossing V33, refined V31 or layered V30'}
if($CloudTwoCrossingCandidate -and $Candidate -ne 'Clouds'){throw 'CloudTwoCrossingCandidate requires Candidate Clouds'}
if($WaterShoreTransmission -and ($Candidate -ne 'WaterAnalytic' -or $WaterRelease -or $AnchorSplit -or $AnchorNoise)){throw 'ShoreTransmission uses only published WaterV1'}
if($WaterRelease -and ($Candidate -ne 'WaterAnalytic' -or $AnchorSplit -or $AnchorNoise)){throw 'Release uses the saved anchored-noise source, no diagnostic builder switches'}
if($AnchorNoise -and !$AnchorSplit){throw 'AnchorNoise requires AnchorSplit'}
if($AnchorSplit -and $Candidate -ne 'WaterAnalytic'){throw 'AnchorSplit requires WaterAnalytic'}
if($FinalNormalAudit -and $Candidate -ne 'WaterDomainAudit'){throw 'FinalNormalAudit requires WaterDomainAudit'}
if($SecondaryDomainAudit -and ($Candidate -ne 'WaterDomainAudit' -or $FinalNormalAudit)){throw 'SecondaryDomainAudit requires exclusive WaterDomainAudit'}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/bake-'+$Candidate.ToLowerInvariant()+'-'+$Label
$folder=if($Candidate -eq 'ContinuityCombined'){'ContinuityCombined20260929V1'}elseif($Candidate -eq 'NormalHex'){'NormalHex20260929V1'}elseif($Candidate -eq 'MagmaFields'){'OrbitalFields20260929MagmaV3'}elseif($Candidate -eq 'MacroApproach'){'MacroApproach20260929V1'}elseif($Candidate -eq 'NormalMacroWarp'){'NormalMacroWarp20260929V1'}else{'FoliagePrototype20260929V1'}
$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'+$folder
if($Candidate -eq 'ContinuityRelease') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1' }
if($Candidate -eq 'UnifiedLava') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava' }
if($Candidate -eq 'UnifiedLavaDetail') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/UnifiedLavaDetail20261002V1' }
if($Candidate -eq 'WaterDepthFiltered') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928' }
if($Candidate -eq 'WaterSurface') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930' }
if($Candidate -eq 'WaterSurfacePass') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePass20260930' }
if($Candidate -eq 'WaterSurfacePrecise') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePrecise20260930' }
if($Candidate -eq 'WaterSurfaceRelative') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfaceRelative20260930' }
if($Candidate -eq 'WaterAnalytic') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930' }
if($AnchorSplit){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchored20260930'}
if($AnchorNoise){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930'}
if($WaterRelease){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1'}
if($WaterShoreTransmission){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterShoreTransmission20261001'}
if($Candidate -eq 'FoliageLeaf') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliageLeaf20260930' }
if($Candidate -eq 'SurfaceScatter') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/SurfaceScatter20260930V2' }
if($Candidate -eq 'Clouds'){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261002V27'}
if($CloudWeatherCandidate){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261003V31'}
if($CloudLayeredCandidate){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261002V30'}
if($CloudTwoCrossingCandidate){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261006V33'}
if($Candidate -eq 'GasGiant'){$destination=$projectRoot+'/Content/APS/APS_ALPHA/Diagnostics/GasCloudBelts20261002V2'}
if($Candidate -eq 'ScatterMaterial'){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ScatterMaterial20260930V1'}
if($Candidate -eq 'WaterDomainAudit') { $destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDomainAudit20260930' }
if($FinalNormalAudit){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterFinalNormalAudit20260930'}
if($SecondaryDomainAudit){$destination=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSecondaryAudit20260930'}
if(Test-Path -LiteralPath $destination) { throw 'Candidate destination exists; never overwrite prior evidence or assets' }
if(Test-Path -LiteralPath $runDir) { throw 'Evidence exists; use a fresh label' }
$bakeBusy=@(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue)
if($bakeBusy.Count -gt 0) {
    throw ('Editor/compiler active; no process touched: '+(($bakeBusy | ForEach-Object {$_.ProcessName+' PID='+$_.Id}) -join ', '))
}
$dll=Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
$sources=@('Source/APS_ALPHA/Editor/APSPlanetSurfaceAssetCommandlet.cpp',
    'Source/APS_ALPHA/Editor/APSSharedTerrainLodABBuilder.h',
    'Source/APS_ALPHA/Editor/APSPlanetFoliagePrototypeBuilder.h',
    'Source/APS_ALPHA/Editor/APSPlanetSurfaceScatterBuilder.h',
    'Source/APS_ALPHA/Editor/APSPlanetScatterMaterialBuilder.h',
    'Source/APS_ALPHA/Core/Planetary/APSPlanetScatterMaterial.h',
    'Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h',
    'Source/APS_ALPHA/Editor/APSFoliageLeafMaterialBuilder.h',
    'Source/APS_ALPHA/Editor/APSSharedTerrainMacroABBuilder.cpp',
    'Source/APS_ALPHA/Editor/APSNormalMacroWarpABBuilder.h',
    'Source/APS_ALPHA/Editor/APSTerrainContinuityPublisher.h',
    'Source/APS_ALPHA/Editor/APSNormalHexSampling.h',
    'Source/APS_ALPHA/Editor/APSOrbitalColorFieldsAB.h',
    'Source/APS_ALPHA/Core/Planetary/APSPlanetFoliagePrototype.h',
    'Source/APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.cpp',
    'Source/APS_ALPHA/Editor/APSWaterDepthMaterialBuilder.h',
    'Source/APS_ALPHA/Editor/APSWaterSurfaceFilterBuilder.h',
    'Source/APS_ALPHA/Editor/APSWaterSurfacePassBuilder.h',
    'Source/APS_ALPHA/Editor/APSWaterWavePrecisionBuilder.h',
    'Source/APS_ALPHA/Editor/APSWaterAnalyticWaveBuilder.h',
    'Source/APS_ALPHA/Editor/APSCoastalWaterPublisher.h',
    'Source/APS_ALPHA/Editor/APSWaterShoreTransmissionBuilder.h',
    'Source/APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h',
    'Source/APS_ALPHA/Editor/APSWaterDomainAuditBuilder.h',
    'Source/APS_ALPHA/Core/Planetary/APSWaterAnalyticWaves.h',
    'Source/APS_ALPHA/Core/Planetary/APSWaterSurfaceFilter.h',
    'Source/APS_ALPHA/Editor/APSPlanetCloudBuilder.h',
    'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h',
    'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudSettings.h',
    'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudLayers.h',
    'Source/APS_ALPHA/Editor/APSPlanetCloudHlsl.h',
    'Source/APS_ALPHA/Editor/APSPlanetCloudLayeredHlsl.h')
if($Candidate -eq 'UnifiedLava'){
    $sources+=@('Source/APS_ALPHA/Editor/APSUnifiedLavaSurfaceBuilder.h',
        'Source/APS_ALPHA/Editor/APSSharedTerrainMaterialBuilder.h',
        'Source/APS_ALPHA/Editor/APSSharedTerrainNormalContinuity.h',
        'Source/APS_ALPHA/Editor/APSSharedTerrainColorBounds.h',
        'Source/APS_ALPHA/Core/Planetary/APSUnifiedLavaSurface.h',
        'Source/APS_ALPHA/Core/Planetary/APSSharedLavaMaterial.h',
        'Source/APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h',
        'Source/APS_ALPHA/Core/Planetary/APSNativeTerrainMaterial.h')
}
if($Candidate -eq 'GasGiant'){
    $sources=@('Source/APS_ALPHA/Editor/APSGasGiantAssetCommandlet.cpp',
        'Source/APS_ALPHA/Core/Planetary/APSGasGiantMaterial.h')
    # Include any split HLSL source owned by this commandlet, not only its cpp.
    $sources+=@(Get-ChildItem -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Editor') -Filter 'APSGasGiant*Hlsl.h' -File | ForEach-Object {'Source/APS_ALPHA/Editor/'+$_.Name})
}
if($Candidate -eq 'UnifiedLavaDetail'){
    $sources+=@('Source/APS_ALPHA/Editor/APSUnifiedLavaDetailBuilder.h',
        'Source/APS_ALPHA/Editor/APSUnifiedLavaSurfaceBuilder.h',
        'Source/APS_ALPHA/Editor/APSSharedTerrainMaterialBuilder.h',
        'Source/APS_ALPHA/Core/Planetary/APSUnifiedLavaAssets.h')
}
foreach($source in $sources) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc) {
        throw ('Known newer bake source: build before running commandlet. '+$source)
    }
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
if($Candidate -eq 'Clouds'){
    . (Join-Path $PSScriptRoot 'CloudWeatherDiagnostic.ps1')
    $cloudSelection=Get-APSCloudWeatherDiagnostic -ProjectRoot $projectRoot -Candidate:$CloudWeatherCandidate -LayeredCandidate:$CloudLayeredCandidate
    if($CloudTwoCrossingCandidate){
        # CloudWeatherDiagnostic.ps1 knows V27/V30/V31 only: keep its V27 constant, source
        # list and DLL-age checks above, then validate the V33 constant against the header.
        $twoCrossingPath='/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261006V33/M_APS_PlanetCloud.M_APS_PlanetCloud'
        $twoCrossingMatch=[regex]::Match((Get-Content -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h') -Raw),
            '(?m)^inline constexpr const TCHAR\* TwoCrossingMaterialPath=TEXT\("([^"]+)"\);')
        if(!$twoCrossingMatch.Success -or $twoCrossingMatch.Groups[1].Value -cne $twoCrossingPath){throw 'Two-crossing cloud path differs from its policy constant'}
        $cloudSelection=[pscustomobject]@{ObjectPath=$twoCrossingPath;Asset=(Join-Path $projectRoot ('Content/'+$twoCrossingPath.Substring(6).Split('.')[0]+'.uasset'));Sources=$cloudSelection.Sources;Candidate=$false;LayeredCandidate=$false}
    }
    if([IO.Path]::GetFullPath((Split-Path -Parent $cloudSelection.Asset)) -ne [IO.Path]::GetFullPath($destination)){throw 'Cloud builder and destination disagree'}
}
New-Item -ItemType Directory -Path $runDir | Out-Null
if($Candidate -eq 'Clouds'){Save-APSCloudWeatherDiagnostic -ProjectRoot $projectRoot -RunDir $runDir -Selection $cloudSelection}
Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -Recurse -File -Filter '*.uasset' |
    Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.uasset') |
    Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/catalog-before.json') -Encoding utf8
Get-FileHash -LiteralPath $dll.FullName | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
if($Candidate -in @('Clouds','GasGiant','UnifiedLava')){
    @('Content/APS/APS_ALPHA/Assets/Materials/M_APS_GasGiantAtmosphere.uasset',
      'Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudVolume20261001V24/M_APS_PlanetCloud.uasset',
      'Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261001V26/M_APS_PlanetCloud.uasset') |
        ForEach-Object {Get-FileHash -LiteralPath ($projectRoot+'/'+$_)} | Select-Object Path,Hash |
        ConvertTo-Json | Out-File -LiteralPath ($runDir+'/previous-visuals-before.json') -Encoding utf8
}
if($Candidate -eq 'FoliageLeaf') {
    $vendor='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4/Content/Ressources'
    @('Mesh/Tree/MI_Grass_Leaf.uasset',
      'Materials/WorldScapeMaterials/Foliage/ModifiedWorldScapeFoilage/MI_FoliageInstance_Mod.uasset',
      'Materials/WorldScapeMaterials/Foliage/ModifiedWorldScapeFoilage/M_Master_Foliage_Mod.uasset') |
        ForEach-Object {Get-FileHash -LiteralPath (Join-Path $vendor $_)} | Select-Object Path,Hash |
        ConvertTo-Json | Out-File ($runDir+'/vendor-leaf-before.json') -Encoding utf8
}
if($Candidate -in @('WaterDepthFiltered','WaterSurface','WaterSurfacePass','WaterSurfacePrecise','WaterSurfaceRelative','WaterAnalytic','WaterDomainAudit')) {
    Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid') -Recurse -File -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/liquids-before.json') -Encoding utf8
}
if($Candidate -in @('UnifiedLava','UnifiedLavaDetail')) {
    # The candidate may only create its new directory. Record the lava input
    # and both published terrain/water releases without editing their packages.
    foreach($protectedFolder in @('SharedLiquid','ContinuityV1','WaterV1')) {
        Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/'+$protectedFolder) -Recurse -File -Filter '*.uasset' |
            Get-FileHash | Select-Object Path,Hash | ConvertTo-Json |
            Out-File -LiteralPath ($runDir+'/'+$protectedFolder.ToLowerInvariant()+'-before.json') -Encoding utf8
    }
}
if($Candidate -eq 'UnifiedLavaDetail') {
    Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava') -Recurse -File -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json |
        Out-File -LiteralPath ($runDir+'/unified-before.json') -Encoding utf8
}
if($Candidate -eq 'Foliage') {
    $meshRoot='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4/Content/Ressources/Mesh'
    @('Rock/Small_Rock/SM_Small_rock.uasset','Grass/SM_Grass_Temp.uasset',
      'Grass/SM_Grass_Cold.uasset','Grass/SM_Grass_Hot.uasset','Tree/SM_Tree_1.uasset') |
        ForEach-Object { Get-FileHash -LiteralPath (Join-Path $meshRoot $_) } |
        Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/vendor-meshes-before.json') -Encoding utf8
}
if($Candidate -eq 'SurfaceScatter') {
    $meshRoot='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4/Content/Ressources/Mesh'
    Get-ChildItem -LiteralPath $meshRoot -Recurse -File -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json |
        Out-File ($runDir+'/vendor-meshes-before.json') -Encoding utf8
    Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1') -File -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json |
        Out-File ($runDir+'/previous-foliage-before.json') -Encoding utf8
}
if($Candidate -eq 'ScatterMaterial') {
    Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/SurfaceScatter20260930V2') -File -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/scatter-before.json') -Encoding utf8
    Get-ChildItem -LiteralPath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4/Content/Ressources/Mesh/Rock' -File -Recurse -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/vendor-rock-before.json') -Encoding utf8
}
$commandlet=if($Candidate -eq 'GasGiant'){'APSGasGiantAsset'}else{'APSPlanetSurfaceAsset'}
$arguments=@(('"'+$projectRoot+'/APS_ALPHA.uproject"'),('-run='+$commandlet),
    '-AllowCommandletRendering','-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen',('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/bake.log"'))
if($Candidate -eq 'GasGiant'){
    $arguments+='-Candidate'
} elseif($Candidate -eq 'UnifiedLava'){
    $arguments+='-OnlyUnifiedLavaSurface'
} elseif($Candidate -eq 'UnifiedLavaDetail'){
    $arguments+='-OnlyUnifiedLavaDetail'
} elseif($WaterShoreTransmission){
    $arguments+='-OnlyWaterShoreTransmission'
} elseif($Candidate -eq 'MagmaFields') {
    $arguments+=@('-OnlySharedTerrainLodAB','-APSBuildTerrainOrbitalFields','-APSBuildTerrainOrbitalMagma')
} elseif($Candidate -eq 'MacroApproach') {
    $arguments+=@('-OnlySharedTerrainMacroAB','-APSBuildMacroApproachRange')
} elseif($Candidate -eq 'NormalMacroWarp') {
    $arguments+=@('-OnlySharedTerrainMacroAB','-APSBuildNormalMacroWarp')
} elseif($Candidate -eq 'ContinuityCombined') {
    $arguments+=@('-OnlySharedTerrainMacroAB','-APSBuildContinuityCombined')
} elseif($Candidate -eq 'ContinuityRelease') {
    $arguments+=@('-OnlySharedTerrainMacroAB','-APSPublishTerrainContinuity')
} elseif($Candidate -eq 'Clouds') {
    $arguments+='-OnlyPlanetCloudCandidate'
    if($CloudWeatherCandidate){$arguments+='-APSCloudWeatherCandidate'}
    if($CloudLayeredCandidate){$arguments+='-APSCloudLayeredCandidate'}
    if($CloudTwoCrossingCandidate){$arguments+='-APSCloudTwoCrossingCandidate'}
} elseif($Candidate -eq 'SurfaceScatter') {
    $arguments+='-OnlySurfaceScatter'
} elseif($Candidate -eq 'ScatterMaterial') {
    $arguments+='-OnlyScatterMaterial'
} elseif($Candidate -eq 'NormalHex') {
    $arguments+=@('-OnlySharedTerrainMacroAB','-APSBuildNormalHex')
} elseif($Candidate -eq 'WaterDepthFiltered') {
    $arguments+='-OnlyWaterDepthFilteredCandidate'
} elseif($Candidate -eq 'WaterSurface') {
    $arguments+='-OnlyWaterSurfaceCandidate'
} elseif($Candidate -eq 'WaterSurfacePass') {
    $arguments+=@('-OnlyWaterSurfacePassCandidate','-ini:Engine:[SystemSettings]:r.Water.SingleLayer.ShadersSupportVSMFiltering=1,r.Water.SingleLayer.VSMFiltering=1')
} elseif($Candidate -eq 'WaterSurfacePrecise') {
    $arguments+='-OnlyWaterSurfacePreciseCandidate'
} elseif($Candidate -eq 'WaterSurfaceRelative') {
    $arguments+='-OnlyWaterSurfaceRelativeCandidate'
} elseif($Candidate -eq 'WaterAnalytic') {
    $arguments+=$(if($WaterRelease){'-OnlyCoastalWaterRelease'}else{'-OnlyWaterAnalyticCandidate'})
} elseif($Candidate -eq 'WaterDomainAudit') {
    $arguments+='-OnlyWaterDomainAudit'
} elseif($Candidate -eq 'FoliageLeaf') {
    $arguments+='-OnlyFoliageLeafCandidate'
} else { $arguments+='-OnlyFoliagePrototype' }
if($FinalNormalAudit){$arguments += '-APSWaterFinalNormalAudit'}
if($AnchorSplit){$arguments += '-APSWaterAnchorSplit'}
if($AnchorNoise){$arguments += '-APSWaterAnchorNoise'}
if($SecondaryDomainAudit){$arguments += '-APSWaterSecondaryDomainAudit'}
$arguments -join ' ' | Out-File ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{Id=$process.Id;StartTime=$process.StartTime;Candidate=$Candidate;Evidence=$runDir;Destination=$destination} | ConvertTo-Json
# Completion, protected hashes, saved assets and actual renders must be checked
# separately. Returning a PID is not a successful bake or visual acceptance.
