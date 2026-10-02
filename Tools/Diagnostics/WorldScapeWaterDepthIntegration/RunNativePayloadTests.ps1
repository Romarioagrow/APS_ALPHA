param([ValidatePattern('^[a-z0-9-]+$')][string]$Label='native-payload-v1')
$ErrorActionPreference='Stop'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue) {
    throw 'Editor/compiler active; no process touched'
}
$privateHost='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild'
$evidence='F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/'+$Label
if(Test-Path -LiteralPath $evidence) { throw 'Evidence exists; choose a fresh label' }
$module=Join-Path $privateHost 'Plugins/WorldScape/Binaries/Win64/UnrealEditor-WorldScapeCore.dll'
if(!(Test-Path -LiteralPath $module)) { throw 'Build the isolated native plugin first' }
New-Item -ItemType Directory -Path $evidence | Out-Null
Get-FileHash -LiteralPath $module | ConvertTo-Json | Out-File ($evidence+'/native-dll.json') -Encoding utf8
$tests='WorldScape.APS.OceanDepth+APS.Gameplay.World.PlanetSurface.CollisionSamplingParity+APS.Gameplay.World.PlanetSurface.WorkerCompletionOwnership+APS.Gameplay.World.PlanetSurface.LodGeneratedSewingNormals+APS.Gameplay.World.PlanetSurface.LodCoincidentNormalPayload'
$tests+='+APS.Gameplay.World.PlanetSurface.MeshPublicationParity'
$arguments=@(
    ('"'+$privateHost+'/HostProject.uproject"'),'/Engine/Maps/Entry',
    '-NullRHI','-unattended','-nop4','-nosound','-nosplash','-NoLiveCoding',
    '-ddc=InstalledNoZenLocalFallback',('-UserDir="'+$evidence+'"'),
    ('-ExecCmds="Automation RunTests '+$tests+'"'),
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$evidence+'/report"'),('-abslog="'+$evidence+'/probe.log"')
)
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($evidence+'/stdout.txt') -RedirectStandardError ($evidence+'/stderr.txt')
[pscustomobject]@{Id=$process.Id;Start=$process.StartTime;Evidence=$evidence;VisualAcceptance=$false}|ConvertTo-Json
