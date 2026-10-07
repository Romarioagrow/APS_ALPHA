param(
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9][a-z0-9-]{0,63}$')][string]$Label,
    [switch]$CheckOnly,
    [switch]$CharacterRoundTrip,
    [switch]$LidimApproach,
    [switch]$NativeGlobeRoundTrip,
    [switch]$GlobeWireframe,
    [switch]$GlobeBaseColor,
    [switch]$GlobeWorldNormal,
    [switch]$GlobeOpaqueOnly,
    [switch]$GlobeNoShadows,
    [switch]$GlobeFreshShadowCache,
    [switch]$OriginalWarpPixel,
    [ValidateSet('None','Control','Native')][string]$SlopeIsolation='None',
    [switch]$CoordinatedWindowConfirmed
)
$ErrorActionPreference='Stop'
if($OriginalWarpPixel){throw 'Retired material-substitution experiment. Replay must use the original saved terrain.'}
if(($GlobeNoShadows -or $GlobeFreshShadowCache) -and (!$NativeGlobeRoundTrip -or $GlobeWireframe -or $GlobeBaseColor -or $GlobeWorldNormal -or $GlobeOpaqueOnly -or $OriginalWarpPixel -or ($GlobeNoShadows -and $GlobeFreshShadowCache))){throw 'Shadow isolation requires a separate normal-material NativeGlobeRoundTrip and one shadow diagnostic only'}
if($CharacterRoundTrip -and $LidimApproach){throw 'CharacterRoundTrip and LidimApproach are separate diagnostic routes'}
if($NativeGlobeRoundTrip -and ($CharacterRoundTrip -or $LidimApproach -or $SlopeIsolation -ne 'None')){throw 'NativeGlobeRoundTrip is a separate lifecycle route'}
if($GlobeWireframe -and !$NativeGlobeRoundTrip){throw 'GlobeWireframe requires NativeGlobeRoundTrip'}
if($GlobeBaseColor -and (!$NativeGlobeRoundTrip -or $GlobeWireframe -or $GlobeWorldNormal -or $OriginalWarpPixel)){throw 'GlobeBaseColor requires a separate NativeGlobeRoundTrip without other material/view diagnostics'}
if($GlobeWorldNormal -and (!$NativeGlobeRoundTrip -or $GlobeWireframe -or $GlobeBaseColor -or $OriginalWarpPixel)){throw 'GlobeWorldNormal requires a separate NativeGlobeRoundTrip without other material/view diagnostics'}
if($OriginalWarpPixel -and (!$NativeGlobeRoundTrip -or $GlobeWireframe -or $GlobeBaseColor -or $GlobeWorldNormal)){throw 'OriginalWarpPixel requires a separate lit NativeGlobeRoundTrip'}
$bufferTarget=if($GlobeWorldNormal){'WorldNormal'}elseif($GlobeBaseColor){'BaseColor'}else{''}
if($GlobeOpaqueOnly -and (!$NativeGlobeRoundTrip -or $GlobeWireframe -or $bufferTarget -or $OriginalWarpPixel)){throw 'GlobeOpaqueOnly requires a separate lit NativeGlobeRoundTrip'}
if($SlopeIsolation -ne 'None' -and (!$LidimApproach -or $CharacterRoundTrip)){throw 'SlopeIsolation requires LidimApproach only'}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$workRoot='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929'
$checkpoint=Join-Path $workRoot 'user-world-khoax-1236'
$slot='Jeqiwoga Cluster 1212'
$runDir=Join-Path $workRoot ('saved-world-replay-'+$Label)
$saveFile=Join-Path $checkpoint ($slot+'.sav')
$metaFile=Join-Path $checkpoint ($slot+'.apsmeta')
$expectedSave='B0D5856DDE70D83ABB2FE4A39067C27925556E61A203A107ABA600202131986F'
$expectedMeta='478EAB11030C6D3E4375AD859EEABEF0CEDB553E816751EFAC23F734552AECE0'
if((Get-FileHash -LiteralPath $saveFile -Algorithm SHA256).Hash -ne $expectedSave){throw 'Protected checkpoint save changed'}
if((Get-FileHash -LiteralPath $metaFile -Algorithm SHA256).Hash -ne $expectedMeta){throw 'Protected checkpoint metadata changed'}
$saveSha1=(Get-FileHash -LiteralPath $saveFile -Algorithm SHA1).Hash
$dll=Get-Item -LiteralPath (Join-Path $projectRoot 'Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
$replaySources=@('Tests/APSExistingWorldPlanetReplayTests.cpp','Tests/APSPublishedTerrainDescentAudit.h',
    'Core/Rendering/APSStellarVisualSubsystem.cpp','Core/Rendering/APSStellarVisualSubsystem.h','Core/Rendering/APSPlanetSurfaceFill.h',
    'Tests/APSStellarTargetFrameTests.cpp','Actors/Astro/Planet.cpp','Tests/APSGasMaterialSeedTests.cpp',
    'Tests/APSOriginalWarpPixelAssets.h',
    'Tests/APSSavedNativeGlobeRoundTrip.h','Core/World/APSPlaceholderGlobe.cpp','Generation/APSNativeGlobeSnapshot.h','Generation/APSClosedGlobeMesh.cpp',
    'Tests/APSSavedCharacterRoundTrip.h','Tests/APSSavedLidimApproach.h','Tests/APSSavedSlopeIsolation.h','Pawns/Characters/CustomGravityCharacter.h','Pawns/Characters/CustomGravityCharacter.cpp',
    'Pawns/Characters/APSSpeedModeCharacter.cpp',
    'Core/Controllers/MainMenuController.cpp','Core/Controllers/GravityPlayerController.cpp',
    'Core/GameModes/GravityGameModeBase.cpp','Core/Saves/APSWorldSaveSnapshot.cpp',
    'Core/Saves/APSWorldSaveSnapshot.h','Core/Saves/GameSave.h','Core/Saves/GameSave.cpp',
    'Core/Instances/MainGameplayInstance.h','Core/Instances/MainGameplayInstance.cpp',
    'Gameplay/Fleet/APSFleetCommand.cpp','Core/World/APSWorldOriginSubsystem.cpp')
$sourcePaths=foreach($relative in $replaySources){
    $source=Get-Item -LiteralPath (Join-Path $projectRoot ('Source/APS_ALPHA/'+$relative))
    if($source.LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw ('Replay source newer than DLL: '+$relative)}
    $source.FullName
}
. (Join-Path $projectRoot 'Tools/Diagnostics/SurfaceUnificationDiagnostic.ps1')
$surface=Get-APSSurfaceUnificationDiagnostic -ProjectRoot $projectRoot -Mode Candidate
$warpAssets=@()
if($OriginalWarpPixel){
    $warpDirectory=Join-Path $projectRoot 'Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003'
    $warpFiles=@('M_APS_ContinuousWarpPixel','MI_APS_ContinuousWarpPixel','MF_APS_ContinuousWarpPixelWAT','MF_APS_ContinuousWarpPixelPlanetMap','MF_APS_ContinuousWarpPixelSlope','MF_APS_ContinuousWarpPixelOrbital','MF_APS_ContinuousWarpPixelNormalCoordinates') | ForEach-Object {Join-Path $warpDirectory ($_+'.uasset')}
    foreach($file in $warpFiles){if(!(Test-Path -LiteralPath $file -PathType Leaf)){throw "Original-colour warp diagnostic missing: $file"}}
    $master=Join-Path $projectRoot 'Content/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.uasset'
    if((Get-FileHash -LiteralPath $master -Algorithm SHA1).Hash -ne '9541C8FB506D3F95E277D97C2FB1E7D626A420B0'){throw 'Original-colour production master changed; experiment refused'}
    $warpAssets=@(Get-FileHash -LiteralPath $warpFiles -Algorithm SHA256 | Select-Object Path,Hash)
}
if($CheckOnly){'PASS: exact checkpoint/source/DLL preflight; no files copied, no process launched, no rendered acceptance.';return}
if(!$CoordinatedWindowConfirmed){throw 'Caller must inspect shared coordination file and explicitly confirm a free window'}
function Assert-NoSharedUnrealProcess {
    $live=Get-CimInstance Win32_Process | Where-Object {
        $_.Name -match 'UnrealEditor|UnrealBuildTool|ShaderCompileWorker|^(cl|link)\.exe$' -or
        ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool|AutomationTool')
    }
    if($live){$live | Select-Object ProcessId,Name,CreationDate,CommandLine | Format-List;throw 'Shared UE/build process active; no process touched'}
}
Assert-NoSharedUnrealProcess
if(Test-Path -LiteralPath $runDir){throw 'Evidence directory exists; choose a fresh label'}
& (Join-Path $projectRoot 'Tools/Diagnostics/AssertPlanetGpuHeadroom.ps1')
$savedDir=Join-Path $runDir 'Saved'
$privateSaves=Join-Path $savedDir 'SaveGames'
New-Item -ItemType Directory -Path $privateSaves | Out-Null
Copy-Item -LiteralPath $saveFile,$metaFile -Destination $privateSaves
if((Get-FileHash -LiteralPath (Join-Path $privateSaves ($slot+'.sav')) -Algorithm SHA256).Hash -ne $expectedSave){throw 'Private save copy mismatch; no launch'}
if((Get-FileHash -LiteralPath (Join-Path $privateSaves ($slot+'.apsmeta')) -Algorithm SHA256).Hash -ne $expectedMeta){throw 'Private metadata copy mismatch; no launch'}
Save-APSSurfaceUnificationDiagnostic -Selection $surface -RunDir $runDir
$userSaves=@(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Saved/SaveGames') -File |
    Where-Object { $_.Name -in @(($slot+'.sav'),($slot+'.apsmeta')) } |
    Get-FileHash -Algorithm SHA256 | Select-Object Path,Hash)
if($userSaves.Count -ne 2){throw 'Both original user save hashes must be recorded; no launch'}
$nativeRoot='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$nativeFiles=@('Binaries/Win64/UnrealEditor-WorldScapeCore.dll',
    'Source/WorldScapeCore/Private/WorldScapeRoot_Main.cpp',
    'Source/WorldScapeCore/Private/WorldScapeRoot_Thread.cpp',
    'Source/WorldScapeCore/Private/WorldScapeLod.cpp') | ForEach-Object {Join-Path $nativeRoot $_}
$starDll=Join-Path $projectRoot 'Plugins/APSStarRenderer/Binaries/Win64/UnrealEditor-APSStarRenderer.dll'
[pscustomobject]@{
    Schema=1;Created=(Get-Date -Format o);Slot=$slot;SourceCheckpoint=$checkpoint
    ExpectedPrivateSavedDir=$savedDir;SaveSha256=$expectedSave;SaveSha1=$saveSha1;MetadataSha256=$expectedMeta
    OriginalUserSaveHashes=$userSaves;SourceHashes=@(Get-FileHash -LiteralPath $sourcePaths -Algorithm SHA256 | Select-Object Path,Hash)
    NativeHashes=@(Get-FileHash -LiteralPath $nativeFiles -Algorithm SHA256 | Select-Object Path,Hash)
    StellarDll=(Get-FileHash -LiteralPath $starDll -Algorithm SHA256 | Select-Object Path,Hash)
    CharacterRoundTrip=[bool]$CharacterRoundTrip
    LidimApproach=[bool]$LidimApproach
    NativeGlobeRoundTrip=[bool]$NativeGlobeRoundTrip
    GlobeWireframe=[bool]$GlobeWireframe
    GlobeBaseColor=[bool]$GlobeBaseColor
    GlobeWorldNormal=[bool]$GlobeWorldNormal
    GlobeOpaqueOnly=[bool]$GlobeOpaqueOnly
    GlobeNoShadows=[bool]$GlobeNoShadows
    GlobeFreshShadowCache=[bool]$GlobeFreshShadowCache
    BufferVisualizationTarget=$bufferTarget
    OriginalWarpPixel=[bool]$OriginalWarpPixel
    OriginalWarpPixelAssets=$warpAssets
    GlobeWireframeScope='Opt-in VMI_Wireframe game viewport with ApplyViewMode/materials-off and process-only r.ForceDebugViewModes=1. Original view mode and all show flags restored on Release. GT configuration logs are not rendered wire-edge proof; not surface material visual acceptance.'
    GlobeBaseColorScope='Opt-in VMI_VisualizeBuffer with the engine BaseColor target. ForceDebugViewModes=1 uses process-only INI; late-registered Cheat target is set by startup ExecCmds before automation. Original viewport mode and all show flags restored on Release. GBuffer diagnostic excludes normal lit atmosphere/lighting; not lit visual acceptance or a guarantee for translucent objects.'
    GlobeWorldNormalScope='Opt-in VMI_VisualizeBuffer with the engine WorldNormal target, through the same viewport lease and startup ExecCmds as BaseColor. No material graph or parameters change; no lit visual acceptance or a guarantee for translucent objects.'
    GlobeOpaqueOnlyScope='Separate lit lifecycle route with process-only startup ShowFlag.Translucency=0. Excludes transparent atmosphere/cloud compositing for diagnosis; no project settings or assets changed; not normal-lit acceptance.'
    GlobeNoShadowsScope='Separate default-material lifecycle route with process-only startup ShowFlag.DynamicShadows=0. Isolates shadow contribution; no project settings, material or light changes. Not normal-lit visual acceptance.'
    GlobeFreshShadowCacheScope='Separate default-material lit lifecycle route with process-only r.Shadow.Virtual.Cache.ForceInvalidateDirectional=1. Dynamic shadows remain on; forced cache refresh is diagnostic only, not a production performance setting.'
    NativeGlobeScope='Opt-in controlled observer relocation outside every generated body unload sphere, then return to Lidim AnchorA. Native streaming/root lifecycle untouched; no physics flight claim; far body can be subpixel.'
    SlopeIsolation=$SlopeIsolation
    Scope='Ordinary LoadWorldSlot of protected private Khoax save. Default passive20s; CharacterRoundTrip native movement. Separate LidimApproach uses controlled camera/pawn transforms along logged generation anchors then native sampled ground+34m and reverse. LidimApproach is NOT native ship physics or exact user camera. Optional SlopeIsolation recompiles the current source graph in this process only; Native changes just colour-slope Code. No saved model/terrain/material asset/save edits. Not global visual acceptance.'
    VisualAcceptance=$false
} | ConvertTo-Json -Depth 6 | Out-File -LiteralPath (Join-Path $runDir 'replay.json') -Encoding utf8
$testCommand='Automation RunTests APS.Rendered.Gameplay.ExistingWorldPlanetReplay'
if($NativeGlobeRoundTrip){$testCommand += '+APS.Gameplay.World.PlanetSurface.SurfaceFillContinuity+APS.Gameplay.World.PlanetSurface.StellarTargetFrame+APS.UI.Generation.GasMaterialSeed.SetterAndPersistence'}
$startupCommand=if($bufferTarget){'r.BufferVisualizationTarget '+$bufferTarget+','+$testCommand}elseif($GlobeOpaqueOnly){'ShowFlag.Translucency 0,'+$testCommand}elseif($GlobeNoShadows){'ShowFlag.DynamicShadows 0,'+$testCommand}elseif($GlobeFreshShadowCache){'r.Shadow.Virtual.Cache.ForceInvalidateDirectional 1,'+$testCommand}else{$testCommand}
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=1000',
    '-APSSavedPlanetReplay',('-APSSavedPlanetReplaySlot="'+$slot+'"'),
    ('-APSSavedPlanetExpectedSavedDir="'+$savedDir+'"'),('-APSSavedPlanetSaveSha1='+$saveSha1),
    ('-ExecCmds="'+$startupCommand+'"'),
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/gameplay.log"')
)
if($CharacterRoundTrip){$arguments += '-APSSavedPlanetCharacterRoundTrip'}
if($LidimApproach){$arguments += '-APSSavedPlanetLidimApproach'}
if($NativeGlobeRoundTrip){$arguments += '-APSSavedPlanetNativeGlobeRoundTrip'}
if($OriginalWarpPixel){$arguments += '-APSOriginalWarpPixelGlobe'}
if($GlobeWireframe){$arguments += @('-APSSavedPlanetGlobeWireframe','-ini:Engine:[SystemSettings]:r.ForceDebugViewModes=1,[SystemSettingsEditor]:r.ForceDebugViewModes=1')}
if($GlobeBaseColor){$arguments += @('-APSSavedPlanetGlobeBaseColor','-ini:Engine:[SystemSettings]:r.ForceDebugViewModes=1,[SystemSettingsEditor]:r.ForceDebugViewModes=1')}
if($GlobeWorldNormal){$arguments += @('-APSSavedPlanetGlobeWorldNormal','-ini:Engine:[SystemSettings]:r.ForceDebugViewModes=1,[SystemSettingsEditor]:r.ForceDebugViewModes=1')}
if($SlopeIsolation -ne 'None'){$arguments += $(if($SlopeIsolation -eq 'Native'){'-APSSavedPlanetSlope=1'}else{'-APSSavedPlanetSlope=0'})}
$arguments -join ' ' | Out-File -LiteralPath (Join-Path $runDir 'command.txt') -Encoding utf8
Assert-NoSharedUnrealProcess
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $runDir 'stdout.txt') -RedirectStandardError (Join-Path $runDir 'stderr.txt')
[pscustomobject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;VisualAcceptance=$false;RequiredAfterExit='Inspect report, frames and strict original/private save hashes. Do not restart a living process.'} | ConvertTo-Json
