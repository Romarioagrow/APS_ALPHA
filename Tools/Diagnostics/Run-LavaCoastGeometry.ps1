[CmdletBinding()]
param(
    [ValidateSet('Volcanic', 'Melted')][string]$Planet = 'Volcanic',
    [string]$OutputRoot = 'F:/ChatGPT/APOSFERA/work/planet_finish_20260927',
    [string]$EditorExecutable = 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$taskProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$taskProject = Join-Path $taskProjectRoot 'APS_ALPHA.uproject'
if (!(Test-Path -LiteralPath $taskProject) -or !(Test-Path -LiteralPath $EditorExecutable)) {
    throw 'Project or Unreal executable is missing.'
}
# Fail closed: printing the existing processes and then continuing is NOT safe.
$taskEditors = @(Get-CimInstance Win32_Process -ErrorAction Stop |
    Where-Object { $_.Name -match '^UnrealEditor' })
if ($taskEditors.Count -gt 0) {
    throw ('Unreal is already running (PIDs: {0}). No diagnostic was launched; do not close user sessions.' -f
        (($taskEditors | ForEach-Object { $_.ProcessId }) -join ', '))
}
$taskRun = Join-Path $OutputRoot ('lava-coast-geometry-{0}-{1}' -f $Planet.ToLowerInvariant(), (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
if (Test-Path -LiteralPath $taskRun) { throw 'Refusing to reuse an existing evidence directory.' }
New-Item -ItemType Directory -Path $taskRun | Out-Null
$taskArguments = @(
    ('"{0}"' -f $taskProject), '/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback', '-unattended', '-nop4', '-nosplash', '-nosound',
    '-NoLiveCoding', '-d3d12', '-sm6', '-RenderOffscreen', '-Windowed', '-ForceRes', '-ResX=1600', '-ResY=900',
    ('-APSDiagnosticPlanet={0}' -f $Planet), '-APSLavaCoverage', '-APSLavaShoreline', '-APSLavaFarShoreline', '-APSCoastGeometry',
    ('-UserDir="{0}"' -f $taskRun), ('-ReportExportPath="{0}/report"' -f $taskRun), ('-abslog="{0}/run.log"' -f $taskRun),
    '-ExecCmds="Automation RunTests APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"'
) -join ' '
$taskProcess = Start-Process -FilePath $EditorExecutable -ArgumentList $taskArguments -WorkingDirectory $taskProjectRoot `
    -WindowStyle Hidden -RedirectStandardOutput (Join-Path $taskRun 'stdout.log') `
    -RedirectStandardError (Join-Path $taskRun 'stderr.log') -PassThru
[PSCustomObject]@{ ProcessId = $taskProcess.Id; Evidence = $taskRun; Planet = $Planet }
# Returns an owned PID to observe; do not launch again because observation times out.
