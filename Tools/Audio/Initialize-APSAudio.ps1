[CmdletBinding()]
param(
    [switch]$ValidateOnly,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE\UE_5.4'
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$projectFile = Join-Path $projectRoot 'APS_ALPHA.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $projectFile) -or !(Test-Path -LiteralPath $editor)) {
    throw 'Project or UE 5.4 commandlet executable not found.'
}
$busy = Get-Process -ErrorAction SilentlyContinue | Where-Object {
    $_.ProcessName -match '^(UnrealEditor(-Cmd)?|UnrealBuildTool|cl|link)$'
}
if ($busy) { throw 'Close Unreal normally and wait for the active build/test slot to finish before creating the audio bank.' }
$bank = Join-Path $projectRoot 'Content\APS\APS_ALPHA\Audio\DA_APSAudioBank.uasset'
$logDir = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$logFile = Join-Path $logDir ('APSAudio-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
$arguments = @($projectFile, '-run=APSAudioAsset', '-unattended', '-NullRHI', '-nosound', '-nop4', "-abslog=$logFile")
if ($ValidateOnly -or (Test-Path -LiteralPath $bank)) { $arguments += '-Validate' }
& $editor @arguments
if ($LASTEXITCODE -ne 0) { throw "Audio bank commandlet failed. Log: $logFile" }
if (!(Test-Path -LiteralPath $bank)) { throw "Audio bank was not created. Build APS_ALPHAEditor in Rider first. Log: $logFile" }
Write-Host "Audio bank ready. Log: $logFile"
Write-Host 'Open the game and check menu volume, footsteps, engine transitions and the Audio sliders.'
