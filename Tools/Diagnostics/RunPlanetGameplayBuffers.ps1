param(
    [ValidateSet('Terrestrial','Frozen')][string]$Family='Terrestrial',
    [ValidateRange(0.1,1000)][double]$HeightKm=3,
    [switch]$NormalSources,
    [switch]$MeshCurvature,
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label
)
$ErrorActionPreference='Stop'
if($MeshCurvature -and $NormalSources){throw 'Select a single normal experiment'}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/'+$Family.ToLowerInvariant()+'-'+$Label
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; no process touched'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
$dll=$projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll'
foreach($file in @('Source/APS_ALPHA/Tests/APSMeshCurvatureProbe.h','Source/APS_ALPHA/Tests/APSOrbitalMacroVariationTests.cpp','Source/APS_ALPHA/Tests/APSSharedTerrainLodABProbe.h','Source/APS_ALPHA/Tests/APSGeneratedGameplayHandoffSmokeTests.cpp','Source/APS_ALPHA/Tests/APSPlanetBufferViewsProbe.h','Source/APS_ALPHA/Tests/APSPlanetNormalSourcesProbe.h')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$file)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Known newer diagnostic source: build first'}
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -Recurse -File -Filter '*.uasset' |
    Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath $dll | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
$arguments=@(('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    '-APSDiagnosticOrbitOverview','-APSDiagnosticTerrainLodAB',$(if($MeshCurvature){'-APSProbeMeshCurvature'}elseif($NormalSources){'-APSProbeNormalSources'}else{'-APSProbeTerrainBuffers'}),'-APSProbeDefaultAtmosphere',
    ('-APSDiagnosticPlanet='+$Family),('-APSDiagnosticOrbitHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture)),
    '-ExecCmds="Automation RunTests APS.Contracts.PlanetSurface.MeshCurvatureProbe+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/gameplay.log"'))
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Family=$Family;HeightKm=$HeightKm;NativeBuffers=(!$NormalSources -and !$MeshCurvature);NormalSources=[bool]$NormalSources;MeshCurvature=[bool]$MeshCurvature} | ConvertTo-Json
