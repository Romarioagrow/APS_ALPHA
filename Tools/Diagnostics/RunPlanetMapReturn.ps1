param(
    [ValidateSet('Water','Terrestrial','Frozen','Metallic','Volcanic')][string]$Family='Water',
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label
)
$ErrorActionPreference='Stop'
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/'+$Family.ToLowerInvariant()+'-map-return-'+$Label
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; no process touched'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
$dll=$projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll'
foreach($file in @('Source/APS_ALPHA/Tests/APSGameplayMapReturnProbe.h','Source/APS_ALPHA/Tests/APSGeneratedGameplayHandoffSmokeTests.cpp','Source/APS_ALPHA/Generation/AstroGenerator.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$file)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Map-return source is newer than DLL; build first'}
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-FileHash -LiteralPath $dll | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
# Fresh isolated UserDir: do not load, overwrite or delete a user's save.
# Radius639.1442/seed487132 test inputs are explicit in the opt-in harness.
# Actual F10 + scripted controlled-pawn travel, not ship-input or FPS acceptance.
$arguments=@(('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    '-APSProbeMapReturn','-APSProbeDefaultAtmosphere',('-APSDiagnosticPlanet='+$Family),
    '-ExecCmds="Automation RunTests APS.Contracts.PlanetSurface.MapReturnRoute+APS.Gameplay.World.PlanetSurface.StrategicMapPreservesStreaming+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/gameplay.log"'))
$arguments -join ' ' | Out-File ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Family=$Family;Route='actual F10, scripted pawn, two unload-return cycles'} | ConvertTo-Json
