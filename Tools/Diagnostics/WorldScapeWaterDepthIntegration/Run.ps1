param([ValidatePattern('^[a-z0-9-]+$')][string]$Label='physical-water-v1',
 [switch]$PaletteBudget, [switch]$Oblique, [switch]$LiveLod, [switch]$Ripples, [switch]$MaterialPerf, [switch]$RippleIsolation, [switch]$NormalBuffer, [switch]$SingleLayerSurfaceControl,
 [ValidateSet('WorldNormal','BaseColor','Roughness','Specular')][string]$BufferView,
 [ValidateSet('NoSpecular','NoReflections','NoIndirect','NoShortRangeAO','SHDiffuse')][string]$LightingIsolation,
 [ValidateRange(1,500)][int]$CameraHeightM=50)
$ErrorActionPreference='Stop'
if($SingleLayerSurfaceControl -and (-not $Ripples -or $LightingIsolation -or $NormalBuffer -or $BufferView -or $LiveLod -or $RippleIsolation)){throw 'Single-layer pass control requires a static ripple lit view with ordinary renderer settings'}
if($MaterialPerf -and $LiveLod){throw 'Material timing requires the frozen A/B fixture, not moving LODs'}
if($RippleIsolation -and (-not $Ripples -or $LiveLod -or $MaterialPerf)){throw 'Isolation requires Ripples and static views only'}
if(($NormalBuffer -or $BufferView) -and ($LiveLod -or $MaterialPerf)){throw 'Buffer capture is not a live/performance run'}
if($NormalBuffer -and $BufferView){throw 'Choose one buffer view'}
if($LightingIsolation -and ($NormalBuffer -or $BufferView -or $MaterialPerf -or $LiveLod)){throw 'Lighting isolation requires a static lit view without timing/buffer overrides'}
if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Editor active; no session touched'}
$project=Join-Path $PSScriptRoot 'host/APS_ALPHA.uproject'
$run=Join-Path $PSScriptRoot $Label
if(Test-Path -LiteralPath $run){throw 'Evidence exists; never overwrite'}
if(-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'host/Binaries/Win64/UnrealEditor-APS_ALPHA.dll'))){throw 'Integration build required'}
New-Item -ItemType Directory -Path $run | Out-Null
# Read-only shared Content. This is a finite automation run, never a bake,
# SavePackage or resave commandlet. All code and matching plugin DLLs are private.
$arguments=@(
 ('"'+$project+'"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
 '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
 '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
 '-APSDiagnosticPlanet=Water','-APSSharedLiquidCoverage','-APSWaterDepthPayload','-APSProbeWaterDepthGameplay',
 ('-UserDir="'+$run+'"'),('-ReportExportPath="'+$run+'/report"'),('-abslog="'+$run+'/surface.log"'),
 '-ExecCmds="Automation RunTests APS.Materials.WaterDepth.PaletteBudget+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"',
 '-TestExit="Automation Test Queue Empty"')
if($PaletteBudget){$arguments+='-APSWaterDepthPaletteBudget'}
if($Oblique){$arguments+='-APSWaterDepthOblique'}
if($LiveLod){$arguments+='-APSWaterDepthLiveLod'}
if($Ripples){$arguments+='-APSWaterRipples'}
if($SingleLayerSurfaceControl){$arguments+='-APSWaterSingleLayerCandidate'}
if($RippleIsolation){$arguments+='-APSRippleIsolation'}
if($NormalBuffer){$arguments+='-APSWaterNormalBuffer'}
if($BufferView){$arguments+=('-APSWaterBufferView='+$BufferView)}
if($LightingIsolation){$arguments+=('-APSWaterLightingIsolation='+$LightingIsolation)}
if($MaterialPerf){$arguments+='-APSWaterMaterialPerf'}
$arguments+=('-APSWaterDepthCameraHeightM='+$CameraHeightM)
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt')
$process | Select-Object Id,StartTime | ConvertTo-Json
