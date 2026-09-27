param([ValidatePattern('^[a-z0-9-]+$')][string]$Label='physical-water-v1',
 [switch]$PaletteBudget, [switch]$Oblique, [switch]$LiveLod, [switch]$Ripples, [switch]$MaterialPerf, [switch]$RippleIsolation, [switch]$NormalBuffer, [switch]$SingleLayerSurfaceControl, [switch]$WaterGpuDump, [switch]$WaterFilteredShadows, [switch]$WaterSurfaceFill, [switch]$WaterFillBaselineControl,
 [ValidateSet('WorldNormal','BaseColor','Roughness','Specular')][string]$BufferView,
 [ValidateSet('NoSpecular','NoReflections','NoIndirect','NoShortRangeAO','SHDiffuse','WaterCaptures','NoWaterComposite','NoWaterDFShadow')][string]$LightingIsolation,
 [ValidateSet('Water','Terrestrial','Ocean','Forest','Oasis','Savanna','Nordic','Tundra','Archipelago','Pangea','SuperEarth','HighMountain')][string]$Planet='Water',
 [ValidateRange(1,500)][int]$CameraHeightM=50)
$ErrorActionPreference='Stop'
$waterPassControl=$LightingIsolation -in @('WaterCaptures','NoWaterComposite','NoWaterDFShadow')
if($waterPassControl -and -not $SingleLayerSurfaceControl){throw 'Water pass isolation requires the explicit single-layer candidate'}
if($SingleLayerSurfaceControl -and (-not $Ripples -or ($LightingIsolation -and -not $waterPassControl) -or $NormalBuffer -or $BufferView -or ($LiveLod -and -not $WaterSurfaceFill) -or $RippleIsolation)){throw 'Single-layer live views require the guarded scene-fill adapter; other pass controls remain static'}
if($MaterialPerf -and $LiveLod){throw 'Material timing requires the frozen A/B fixture, not moving LODs'}
if($RippleIsolation -and (-not $Ripples -or $LiveLod -or $MaterialPerf)){throw 'Isolation requires Ripples and static views only'}
if(($NormalBuffer -or $BufferView) -and ($LiveLod -or $MaterialPerf)){throw 'Buffer capture is not a live/performance run'}
if($NormalBuffer -and $BufferView){throw 'Choose one buffer view'}
if($LightingIsolation -and ($NormalBuffer -or $BufferView -or $MaterialPerf -or $LiveLod)){throw 'Lighting isolation requires a static lit view without timing/buffer overrides'}
if($WaterGpuDump -and (-not $SingleLayerSurfaceControl -or -not $Oblique -or $LightingIsolation -or $MaterialPerf -or $LiveLod -or $NormalBuffer -or $BufferView -or $RippleIsolation)){throw 'Water GPU dump requires the unmodified single-layer oblique fixture without other diagnostic overrides'}
if($WaterFilteredShadows -and (-not $SingleLayerSurfaceControl -or $LightingIsolation -or $NormalBuffer -or $BufferView -or $RippleIsolation)){throw 'Filtered water shadows require the single-layer lit fixture without lighting isolation'}
if($WaterSurfaceFill -and (-not $WaterFilteredShadows -or $WaterGpuDump)){throw 'Surface fill requires filtered water shadows without GPU dump'}
if($WaterFillBaselineControl -and (-not $WaterSurfaceFill -or $MaterialPerf -or $LiveLod)){throw 'Fill baseline control requires the static scene fill candidate without timing'}
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
 ('-APSDiagnosticPlanet='+$Planet),'-APSSharedLiquidCoverage','-APSWaterDepthPayload','-APSProbeWaterDepthGameplay',
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
if($WaterGpuDump){$arguments+='-APSWaterGpuDump'}
if($WaterSurfaceFill){$arguments+='-APSWaterSurfaceFill'}
if($WaterFillBaselineControl){$arguments+='-APSWaterFillBaselineControl'}
if($WaterFilteredShadows){
 # Read-only shader-support setting must be established at startup, not forced
 # at runtime. The existing DF-shadow support already enables the same separated
 # main-light shader output/key. No persistent Config or engine file is edited.
 $arguments+='-ini:Engine:[SystemSettings]:r.Water.SingleLayer.ShadersSupportVSMFiltering=1,[SystemSettings]:r.Water.SingleLayer.VSMFiltering=1'
 $arguments+='-APSWaterFilteredShadows'
}
$arguments+=('-APSWaterDepthCameraHeightM='+$CameraHeightM)
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt')
$process | Select-Object Id,StartTime | ConvertTo-Json
