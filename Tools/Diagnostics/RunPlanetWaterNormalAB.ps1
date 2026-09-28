param(
    [ValidateSet('Water','Terrestrial','Oasis')][string]$Family='Terrestrial',
    [ValidatePattern('^[a-z0-9-]+$')][string]$Label='coast-50m-v1',
    [ValidateRange(0.002,10000)][double]$HeightKm=0.05,
    [switch]$OpenWater
)
$ErrorActionPreference='Stop'
$projectRoot = 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
# Run only during the coordinated Codex editor window. No process is killed.
$coordination = Get-Content -Encoding UTF8 -LiteralPath ($projectRoot+'/Docs/coordination/PLANET_EDITOR_WINDOW.md')
# ASCII source also works in Windows PowerShell 5.1 (UTF-8 without BOM).
$windowWord = '\u041e\u043a\u043d\u043e'
$releasedWord = '\u043e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0435\u043d\u043e'
$acceptedWord = '\u043f\u0440\u0438\u043d\u044f\u043b\u0430'
$lowerWindowWord = '\u043e\u043a\u043d\u043e'
$releases = @($coordination | Select-String -Pattern ('^'+$windowWord+' Claude '+$releasedWord+':'))
$handoffs = @($coordination | Select-String -Pattern ('^'+$windowWord+' '+$releasedWord+':|^Claude '+$acceptedWord+' '+$lowerWindowWord))
if (!$releases.Count -or ($handoffs.Count -and $releases[-1].LineNumber -le $handoffs[-1].LineNumber)) {
    throw 'Claude has not explicitly returned the latest editor window; no launch'
}
if (Get-Process UnrealEditor,UnrealEditor-Cmd,ShaderCompileWorker,cl -ErrorAction SilentlyContinue) {
    throw 'An editor/compiler is active; no session touched'
}
$runDir = 'F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/' + $Family.ToLowerInvariant() + '-' + $Label
if (Test-Path -LiteralPath $runDir) { throw 'Evidence directory exists; choose a fresh label' }
New-Item -ItemType Directory -Path $runDir | Out-Null
$arguments = @(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),
    '/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound',
    '-NoLiveCoding','-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    '-APSDiagnosticOrbitOverview','-APSProbeWaterNormalAB',
    ('-APSDiagnosticPlanet='+$Family),
    ('-APSDiagnosticOrbitHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture)),
    ('-UserDir="'+$runDir+'"'),
    '-ExecCmds="Automation RunTests APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'), ('-abslog="'+$runDir+'/surface.log"')
)
if ($OpenWater) { $arguments += '-APSWaterABOpenWater' }
$process = Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{ Id=$process.Id; StartTime=$process.StartTime; Evidence=$runDir } | ConvertTo-Json
