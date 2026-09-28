param(
    [ValidateSet('Oasis','Terrestrial','Ice','Frozen','Water')][string]$Family='Oasis',
    [ValidatePattern('^[a-z0-9-]+$')][string]$Label='orbit-100-v1',
    [ValidateRange(0.1,10000)][double]$HeightKm=100
)
$ErrorActionPreference='Stop'
$projectRoot = 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
if (Get-Process UnrealEditor,UnrealEditor-Cmd,ShaderCompileWorker,cl -ErrorAction SilentlyContinue) {
    throw 'An editor/compiler is already active; no session touched'
}
$runDir = 'F:/ChatGPT/APOSFERA/work/planet_macro_20260928/' + $Family.ToLowerInvariant() + '-' + $Label
if (Test-Path -LiteralPath $runDir) { throw 'Evidence directory exists; choose a fresh label' }
New-Item -ItemType Directory -Path $runDir | Out-Null
$arguments = @(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),
    '/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound',
    '-NoLiveCoding','-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    '-APSDiagnosticOrbitOverview','-APSDiagnosticTerrainLodAB','-APSProbeMacroAB',
    ('-APSDiagnosticPlanet='+$Family),
    ('-APSDiagnosticOrbitHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture)),
    ('-UserDir="'+$runDir+'"'),
    '-ExecCmds="Automation RunTests APS.Contracts.PlanetSurface.OrbitalMacroFamilyGate+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'), ('-abslog="'+$runDir+'/surface.log"')
)
$process = Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{ Id=$process.Id; StartTime=$process.StartTime; Evidence=$runDir } | ConvertTo-Json
