$ErrorActionPreference='Stop'
if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Editor active; no session touched'}
$project=Join-Path $PSScriptRoot 'host/APS_ALPHA.uproject'
if(-not (Test-Path -LiteralPath $project)){throw 'Prepare the isolated host first'}
$buildLog=Join-Path $PSScriptRoot ('build-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.log')
& 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat' APS_ALPHAEditor Win64 Development ('-Project='+$project) -NoHotReloadFromIDE -NoXGE -WaitMutex ('-Log='+$buildLog)
if($LASTEXITCODE -ne 0){throw ('Integration build failed: '+$LASTEXITCODE)}
