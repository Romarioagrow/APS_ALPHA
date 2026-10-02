param([ValidatePattern('^base-batch-private-v[0-9]+$')][string]$Label='base-batch-private-v1')
$ErrorActionPreference='Stop'
$hostProject='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild'
$runDir='F:/ChatGPT/APOSFERA/work/planet_flight_residency_20260930/'+$Label
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use fresh label'}
$dll=$hostProject+'/Plugins/WorldScape/Binaries/Win64/UnrealEditor-WorldScapeCore.dll'
foreach($source in @('Private/WorldScapeRoot_Main.cpp','Public/WorldScapeRoot.h','Private/Tests/APSWorldScapeFreshRootTests.cpp')){
    if((Get-Item -LiteralPath ($hostProject+'/Plugins/WorldScape/Source/WorldScapeCore/'+$source)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Private DLL is stale'}
}
New-Item -ItemType Directory -Path $runDir|Out-Null
Get-FileHash -LiteralPath $dll|Select-Object Path,Hash|ConvertTo-Json|Out-File ($runDir+'/native-dll.json') -Encoding utf8
$tests='WorldScape.APS.OceanDepth+APS.Gameplay.World.PlanetSurface.CollisionSamplingParity+APS.Gameplay.World.PlanetSurface.WorkerCompletionOwnership+APS.Gameplay.World.PlanetSurface.LodGeneratedSewingNormals+APS.Gameplay.World.PlanetSurface.LodCoincidentNormalPayload+APS.Gameplay.World.PlanetSurface.MeshPublicationParity+APS.Gameplay.World.PlanetSurface.FreshRootProfilePriming'
$arguments=@(('"'+$hostProject+'/HostProject.uproject"'),'/Engine/Maps/Entry','-NullRHI','-unattended','-nop4','-nosound','-nosplash','-NoLiveCoding','-ddc=InstalledNoZenLocalFallback',('-UserDir="'+$runDir+'"'),('-ExecCmds="Automation RunTests '+$tests+'"'),'-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),('-abslog="'+$runDir+'/probe.log"'))
$arguments -join ' '|Out-File ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
$process.PriorityClass='BelowNormal'
[pscustomobject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir}|ConvertTo-Json
