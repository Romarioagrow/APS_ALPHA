param([Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$Payload)
$ErrorActionPreference='Stop'
# This is a current-menu-world regression runner, NOT a forced water fixture.
# The first pair selected Frozen/ocean=false. Inspect the Profile log AND
# WS_OceanDepth trace before interpreting any output as active-water cost.
Write-Warning 'Uses the current menu world. A dry world measures opt-out only; verify ocean=true and WS_OceanDepth before claiming water cost.'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker,UnrealInsights -ErrorAction SilentlyContinue) { throw 'Another editor/compiler/analysis process active' }
$project='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$evidence='F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/'+$Label
if(Test-Path -LiteralPath $evidence) { throw 'Evidence exists; use a new label' }
& (Join-Path $PSScriptRoot '../AssertPlanetGpuHeadroom.ps1')
New-Item -ItemType Directory -Path $evidence|Out-Null
$planetArguments=@(('"'+$project+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-game','-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    '-ExecCmds="worldscape.CollisionSampleTasks 4, aps.Ship.StartGenerated 1, aps.Char.AutoProbe run map=L_WorldGeneration warmup=30 duration=20 sprint=1 mode=0 period=0.7 look=40 quit"',
    ('-UserDir="'+$evidence+'"'),('-abslog="'+$evidence+'/game.log"'),
    '-trace=cpu,gpu,frame,bookmark,region,log','-statnamedevents',('-tracefile="'+$evidence+'/trace.utrace"'))
if($Payload) { $planetArguments+='-APSWaterDepthPayload' }
$planetArguments -join ' '|Out-File ($evidence+'/command.txt') -Encoding utf8
Get-FileHash -LiteralPath ($project+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll'),
    'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4/Binaries/Win64/UnrealEditor-WorldScapeCore.dll'|ConvertTo-Json|Out-File ($evidence+'/dlls.json') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $planetArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($evidence+'/stdout.txt') -RedirectStandardError ($evidence+'/stderr.txt')
[pscustomobject]@{Id=$process.Id;Start=$process.StartTime;Evidence=$evidence;Payload=[bool]$Payload;WaterMaterial='unchanged'}|ConvertTo-Json
