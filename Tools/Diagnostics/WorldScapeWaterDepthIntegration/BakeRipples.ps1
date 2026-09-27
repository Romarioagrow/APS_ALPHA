param([ValidatePattern('^[a-z0-9-]+$')][string]$Label='bake-ripples-v1')
$ErrorActionPreference='Stop'
if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Editor active; no session touched'}
$hostProject=Join-Path $PSScriptRoot 'host'
$output=Join-Path $hostProject 'Intermediate/WaterRippleAssets/ViewAnchorRipples20260927'
if(Test-Path -LiteralPath $output){throw 'New assets only; refusing to overwrite'}
$run=Join-Path $PSScriptRoot $Label
if(Test-Path -LiteralPath $run){throw 'Evidence exists; refusing to overwrite'}
New-Item -ItemType Directory -Path $run | Out-Null
# Only the private mount under host/Intermediate may be saved. Never /Game,
# whose Content junction leads back to the accepted production project.
$arguments=@(('"'+$hostProject+'/APS_ALPHA.uproject"'),'-run=APSPlanetSurfaceAsset',
 '-OnlyWaterRippleCandidate','-AllowCommandletRendering','-ddc=InstalledNoZenLocalFallback',
 '-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding','-d3d12','-sm6','-RenderOffscreen',
 ('-UserDir="'+$run+'"'),('-abslog="'+$run+'/bake.log"'))
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt')
$process | Select-Object Id,StartTime | ConvertTo-Json
