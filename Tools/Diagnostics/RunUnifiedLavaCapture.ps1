param(
    [ValidateSet('Lava','Melted','Volcanic')][string]$Family='Volcanic',
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$DetailCandidate,
    [ValidateRange(0.0,1.0)][float]$DetailStrength=1.0
)
# Real default gameplay binding. The existing observer captures five settled
# heights and a near-datum collision trace; it is not a continuous-flight test.
$ErrorActionPreference='Stop'
if(!$DetailCandidate -and $DetailStrength -ne 1.0){throw 'DetailStrength requires explicit DetailCandidate'}
$projectRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){
    throw 'Editor/compiler active; no session touched'
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
$dll=Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
foreach($source in @('Core/Planetary/APSUnifiedLavaMaterialPreparation.h',
    'Core/Planetary/APSUnifiedLavaAssets.h',
    'Core/Planetary/APSSharedTerrainMaterial.h',
    'Core/Planetary/APSUnifiedLavaSurface.h','Generation/PlanetarySurfaceGenerator.cpp',
    'Generation/PlanetarySurfaceGenerator.h','Generation/PlanetarySurfaceGeneratorStreaming.cpp',
    'Generation/AstroGenerator.cpp','Generation/AstroGenerator.h','Actors/Astro/PlanetaryBodyStreaming.cpp',
    'Core/GameModes/GravityGameModeBase.cpp','Core/GameModes/GravityGameModeBase.h',
    'Tests/APSUnifiedLavaSurfaceProbe.h','Tests/APSUnifiedLavaPendingDiagnostics.h','Tests/APSSharedLavaCoverageProbe.cpp',
    'Tests/APSGeneratedGameplayHandoffSmokeTests.cpp')){
    if((Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){
        throw ('Source newer than DLL: '+$source)
    }
}
if($DetailCandidate -and !(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/UnifiedLavaDetail20261002V1/MI_APS_UnifiedLavaSurface.uasset'))){
    throw 'Isolated fine-lava candidate has not been baked'
}
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/unified-'+$Family.ToLowerInvariant()+'-'+$Label
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-FileHash -LiteralPath $dll.FullName | Select-Object Path,Hash | ConvertTo-Json |
    Out-File -LiteralPath ($runDir+'/dll.json') -Encoding utf8
Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava') -File -Recurse -Filter '*.uasset' |
    Get-FileHash | Select-Object Path,Hash | ConvertTo-Json |
    Out-File -LiteralPath ($runDir+'/assets-before.json') -Encoding utf8
if($DetailCandidate){
    Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/UnifiedLavaDetail20261002V1') -File -Filter '*.uasset' |
        Get-FileHash | Select-Object Path,Hash | ConvertTo-Json |
        Out-File -LiteralPath ($runDir+'/candidate-before.json') -Encoding utf8
}
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-unattended','-nop4','-nosound','-nosplash','-NoLiveCoding',
    '-ddc=InstalledNoZenLocalFallback','-d3d12','-sm6','-RenderOffscreen',
    '-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    ('-APSDiagnosticPlanet='+$Family),'-APSLavaCoverage','-APSUnifiedLavaProbe','-APSRequireUnifiedLava','-APSProbeDefaultAtmosphere',
    ('-APSUnifiedEvidence="'+$runDir+'/frames"'),('-UserDir="'+$runDir+'"'),
    '-ExecCmds="Automation RunTests APS.Gameplay.World.PlanetSurface.UnifiedLava.StarterObservationBudget+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-abslog="'+$runDir+'/game.log"')
)
if($DetailCandidate){
    $arguments+=@('-APSUnifiedLavaDetailCandidate',('-APSUnifiedLavaDetailStrength='+$DetailStrength.ToString([Globalization.CultureInfo]::InvariantCulture)))
}
$arguments -join ' ' | Out-File -LiteralPath ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' `
    -ArgumentList $arguments -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
$process.PriorityClass='BelowNormal'
[pscustomobject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Family=$Family}|ConvertTo-Json
