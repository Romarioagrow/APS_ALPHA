param(
    [ValidateSet('Oasis','Terrestrial','Ice','Frozen','Water')][string]$Family='Oasis',
    [ValidatePattern('^[a-z0-9-]+$')][string]$Label='orbit-100-v1',
    [ValidateRange(0.1,10000)][double]$HeightKm=100,
    [switch]$ApproachRange,
    [switch]$NormalWarp,
    [switch]$NormalHex,
    [switch]$Combined,
    [ValidateRange(0,3840)][int]$ViewportWidth=0,
    [ValidateRange(0,2160)][int]$ViewportHeight=0
)
$ErrorActionPreference='Stop'
if($Combined){$NormalHex=[switch]$true}
if(($ViewportWidth -eq 0) -ne ($ViewportHeight -eq 0)){throw 'Specify both viewport dimensions or neither'}
if($ViewportWidth -gt 0 -and ($ViewportWidth -lt 640 -or $ViewportHeight -lt 360)){throw 'Diagnostic viewport is too small'}
$projectRoot = 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
if (Get-Process UnrealEditor,UnrealEditor-Cmd,ShaderCompileWorker,cl -ErrorAction SilentlyContinue) {
    throw 'An editor/compiler is already active; no session touched'
}
$runDir = 'F:/ChatGPT/APOSFERA/work/planet_macro_20260928/' + $Family.ToLowerInvariant() + '-' + $Label
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
if(([int]$ApproachRange.IsPresent+[int]$NormalWarp.IsPresent+[int]$NormalHex.IsPresent) -gt 1){throw 'Choose only one isolated candidate'}
if($ApproachRange -or $NormalWarp -or $NormalHex) {
    $candidate=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'+$(if($Combined){'ContinuityCombined20260929V1/MI_APS_NormalWarpTerra.uasset'}elseif($NormalHex){'NormalHex20260929V1/MI_APS_NormalWarpTerra.uasset'}elseif($NormalWarp){'NormalMacroWarp20260929V1/MI_APS_NormalWarpTerra.uasset'}else{'MacroApproach20260929V1/MI_APS_MacroTerra.uasset'})
    if(-not(Test-Path -LiteralPath $candidate)){throw 'Approach candidate not baked'}
    $dll=Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
    foreach($source in @('Source/APS_ALPHA/Tests/APSSharedTerrainLodABProbe.h','Source/APS_ALPHA/Tests/APSGeneratedGameplayHandoffSmokeTests.cpp')) {
        if((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw 'Known newer diagnostic source: build first'}
    }
}
if (Test-Path -LiteralPath $runDir) { throw 'Evidence directory exists; choose a fresh label' }
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -Recurse -File -Filter '*.uasset' |
    Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll') |
    Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
$arguments = @(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),
    '/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound',
    '-NoLiveCoding','-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes',
    ('-ResX='+$(if($ViewportWidth -gt 0){$ViewportWidth+16}else{1600})),
    ('-ResY='+$(if($ViewportHeight -gt 0){$ViewportHeight+64}else{900})),
    '-APSDiagnosticOrbitOverview','-APSDiagnosticTerrainLodAB','-APSProbeMacroAB',
    ('-APSDiagnosticPlanet='+$Family),
    ('-APSDiagnosticOrbitHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture)),
    ('-UserDir="'+$runDir+'"'),
    '-ExecCmds="aps.Surface.TerrainContinuity 0,Automation RunTests APS.Contracts.PlanetSurface.OrbitalMacroFamilyGate+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'), ('-abslog="'+$runDir+'/surface.log"')
)
if($ApproachRange){$arguments+='-APSProbeMacroApproachRange'}
if($NormalWarp){$arguments+=@('-APSProbeNormalMacroWarp','-APSProbeDefaultAtmosphere')}
if($NormalHex){$arguments+=@('-APSProbeNormalHex','-APSProbeDefaultAtmosphere')}
if($Combined){$arguments+='-APSProbeContinuityCombined'}
if($ViewportWidth -gt 0){
    # PIE has its own size; offscreen work-area bounds must also leave room for
    # that window's borders. A larger PIE request alone is clamped to editor size.
    # Per-process ini overrides are excluded from config writes by the engine;
    # the run also has its own UserDir. Actual capture dimensions remain evidence.
    $arguments+=('-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.LevelEditorPlaySettings]:NewWindowWidth='+$ViewportWidth+',[/Script/UnrealEd.LevelEditorPlaySettings]:NewWindowHeight='+$ViewportHeight)
}
$process = Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{ Id=$process.Id; StartTime=$process.StartTime; Evidence=$runDir } | ConvertTo-Json
