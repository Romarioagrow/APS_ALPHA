param(
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$GasVisual,
    [switch]$GasSeed
)
# Isolated rendered diagnostic: no asset writes, no forced shell visibility.
# GasSeed opts into 18 fixed-radius seed/snapshot frames instead of the radius matrix.
# Snapshot means in-memory model roundtrip, not full disk/game save replay.
# A successful report still requires inspecting the PNGs.
$ErrorActionPreference='Stop'
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){
    throw 'Editor/compiler active; no session touched'
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
$dll=Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
$sources=@(
    'Tests/APSMainMenuPreviewSmokeTests.cpp','Tests/APSGasAtmosphereControlBoundsTests.cpp',
    'Generation/AstroGenerator.cpp','Generation/AstroGenerator.h','Generation/AstroGeneratorPreviewFrame.cpp',
    'Generation/AstroGeneratorBodyOverrides.cpp','UI/MainMenu/WorldGenerationViewModel.cpp',
    'UI/MainMenu/APSAtmosphereControlBounds.h','Actors/Astro/Planet.cpp',
    'Core/Planetary/APSGasGiantMaterial.h')
if($GasSeed){
    $sources+=@('Tests/APSGasMaterialSeedTests.cpp','Core/Saves/APSWorldSaveSnapshot.cpp',
        'Core/Saves/APSWorldSaveSnapshot.h','Core/Saves/GameSave.h',
        'Core/Model/GeneratedWorld.cpp','Core/Model/GeneratedWorld.h')
}
foreach($source in $sources){
    if((Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){
        throw ('Source newer than DLL; build before rendering: '+$source)
    }
}
if($GasVisual -and !(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/Diagnostics/GasCloudBelts20261002V2/M_APS_GasGiantAtmosphere_V2.uasset'))){
    throw 'Bake the separate gas visual candidate before requesting its rendered matrix'
}
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/gas-atmosphere-'+$Label
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-FileHash -LiteralPath $dll.FullName | Select-Object Path,Hash | ConvertTo-Json |
    Out-File -LiteralPath ($runDir+'/dll.json') -Encoding utf8
$protected=@(
    $projectRoot+'/Content/APS/APS_ALPHA/Assets/Materials/M_APS_GasGiantAtmosphere.uasset'
    $projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.uasset'
)
$protected+=@(Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -Filter '*.uasset' -File -Recurse | ForEach-Object {$_.FullName})
$protected | ForEach-Object {Get-FileHash -LiteralPath $_} | Select-Object Path,Hash |
    ConvertTo-Json | Out-File -LiteralPath ($runDir+'/assets-before.json') -Encoding utf8
$probeArgument=if($GasSeed){'-APSProbeGasSeed'}else{'-APSProbeGasAtmosphereMatrix'}
$testFilter='APS.UI.Generation.GasAtmosphere+APS.Rendered.MainMenu.AstronomicalPreview'
if($GasSeed){$testFilter+='+APS.UI.Generation.GasMaterialSeed'}
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1920','-ResY=1080',
    '-unattended','-nop4','-nosound','-nosplash','-NoLiveCoding',
    '-ddc=InstalledNoZenLocalFallback',$probeArgument,
    ('-UserDir="'+$runDir+'"'),
    ('-ExecCmds="Automation RunTests '+$testFilter+'"'),
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'),('-abslog="'+$runDir+'/gas.log"')
)
$arguments+=('-ini:Engine:[SystemSettings]:aps.Surface.GasVisual='+[int][bool]$GasVisual)
$arguments -join ' ' | Out-File -LiteralPath ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
    -ArgumentList $arguments -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
$process.PriorityClass='BelowNormal'
[pscustomobject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir}|ConvertTo-Json
