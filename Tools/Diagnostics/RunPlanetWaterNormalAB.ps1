param(
    [ValidateSet('Water','Terrestrial','Oasis','Volcanic')][string]$Family='Terrestrial',
    [ValidatePattern('^[a-z0-9-]+$')][string]$Label='coast-50m-v1',
    [ValidateRange(0.002,10000)][double]$HeightKm=0.05,
    [ValidateRange(0.05,1000)][double]$DepthM=20,
    [double[]]$ViewFrame=@(),
    [switch]$OpenWater,
    [switch]$CoastalRelief,
    [switch]$CoastGeometryOnly,
    [switch]$CoastMotion,
    [switch]$CoastTranslate,
    [switch]$WaterReference,
    [switch]$LavaReference,
    [switch]$ClearCloudOcclusion,
    [ValidatePattern('^[a-z0-9-]+$')][string]$WindowToken='',
    [switch]$ColumnSnapshot,
    [switch]$ColumnArtPalette,
    [switch]$NativeColumn,
    [switch]$SurfaceFilter,
    [switch]$SurfaceAOIsolation,
    [switch]$SurfaceNormalBuffer,
    [switch]$SurfacePass,
    [switch]$SurfacePrecise,
    [switch]$SurfaceRelative,
    [switch]$SurfaceWaveIsolation,
    [switch]$SurfaceAnalytic,
    [switch]$AnchorSplit,
    [switch]$AnchorNoise,
    [switch]$DomainAudit,
    [switch]$FinalNormalAudit,
    [switch]$SecondaryDomainAudit,
    [ValidateSet('1','0.1')][string]$WaveScaleFactor='1'
)
$ErrorActionPreference='Stop'
if($CoastMotion -and !$CoastGeometryOnly){throw 'CoastMotion requires unchanged published geometry-only mode'}
if($CoastTranslate -and !$CoastMotion){throw 'CoastTranslate requires CoastMotion, fixed observer and frozen geometry'}
if($WaterReference -and (!$CoastGeometryOnly -or $Family -ne 'Water')){throw 'WaterReference is only Water geometry mode; no saved-world replay'}
if($LavaReference -and (!$CoastGeometryOnly -or $Family -ne 'Volcanic' -or $WaterReference)){throw 'LavaReference is only Volcanic geometry mode; no saved-world replay'}
if($Family -eq 'Volcanic' -and !$CoastGeometryOnly){throw 'Volcanic only supports unchanged geometry-only capture, no Water shader experiments'}
if($ClearCloudOcclusion -and !$CoastGeometryOnly){throw 'Cloud occluder isolation requires geometry-only coastal capture; not production or cloud acceptance'}
if($AnchorNoise -and !$AnchorSplit){throw 'AnchorNoise requires AnchorSplit'}
if($AnchorNoise -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930/MI_APS_WaterAnalytic.uasset')){throw 'Bake anchored noise first'}
if($AnchorSplit -and (!$SurfaceAnalytic -or $DomainAudit)){throw 'AnchorSplit requires analytic Lit comparison'}
if($AnchorSplit -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchored20260930/MI_APS_WaterAnalytic.uasset')){throw 'Bake anchored candidate first'}
if($SecondaryDomainAudit -and (!$DomainAudit -or !$SurfaceAnalytic -or $FinalNormalAudit)){throw 'SecondaryDomainAudit requires exclusive analytic DomainAudit'}
if($SecondaryDomainAudit -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSecondaryAudit20260930/MI_APS_WaterDomainAudit.uasset')){throw 'Bake secondary domain diagnostic first'}
if($FinalNormalAudit -and (!$DomainAudit -or !$SurfaceAnalytic)){throw 'FinalNormalAudit requires DomainAudit and SurfaceAnalytic'}
if($FinalNormalAudit -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterFinalNormalAudit20260930/MI_APS_WaterDomainAudit.uasset')){throw 'Bake final normal diagnostic first'}
if($DomainAudit -and (!$SurfaceAnalytic -or $SurfaceNormalBuffer -or $SurfaceWaveIsolation)){throw 'Domain audit requires analytic Lit route with no other isolation'}
if($DomainAudit -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDomainAudit20260930/MI_APS_WaterDomainAudit.uasset')){throw 'Bake WaterDomainAudit first'}
if ($NativeColumn) { $ColumnSnapshot=$true }
if ($SurfaceRelative) { $SurfacePrecise=$true }
if ($SurfaceAnalytic -and (!$SurfaceRelative -or $SurfaceWaveIsolation)) { throw 'Analytic kernel needs camera-relative control without wave isolation' }
if ($SurfaceAnalytic -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930/MI_APS_WaterAnalytic.uasset')) { throw 'Bake WaterAnalytic first' }
if ($SurfaceWaveIsolation -and (!$SurfaceRelative -or $SurfaceAOIsolation -or $SurfacePass)) { throw 'Wave isolation requires camera-relative water with ordinary lighting' }
if ($SurfaceRelative -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfaceRelative20260930/MI_APS_WaterSurface.uasset')) { throw 'Bake WaterSurfaceRelative first' }
if ($SurfacePrecise -and (!$SurfaceFilter -or $SurfacePass -or $SurfaceAOIsolation)) { throw 'SurfacePrecise requires SurfaceFilter with ordinary lighting' }
if ($SurfacePrecise -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePrecise20260930/MI_APS_WaterSurface.uasset')) { throw 'Bake WaterSurfacePrecise first' }
if ($SurfaceNormalBuffer -and (!$SurfaceFilter -or $SurfaceAOIsolation -or $SurfacePass)) { throw 'Normal buffer requires SurfaceFilter without AO or water-pass changes' }
if ($SurfaceAOIsolation -and !$SurfaceFilter) { throw 'AO isolation requires SurfaceFilter; diagnostics only, never a performance comparison' }
if ($SurfacePass -and (!$SurfaceFilter -or $SurfaceAOIsolation)) { throw 'SurfacePass requires SurfaceFilter and ordinary lighting' }
if ($SurfacePass -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePass20260930/MI_APS_WaterSurface.uasset')) { throw 'Bake WaterSurfacePass first' }
if ($SurfaceFilter -and !$NativeColumn) { throw 'SurfaceFilter requires NativeColumn' }
if ($WaveScaleFactor -ne '1' -and !$SurfaceFilter) { throw 'WaveScaleFactor requires SurfaceFilter' }
if ($SurfaceFilter -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930/MI_APS_WaterSurface.uasset')) { throw 'Bake WaterSurface first' }
if ($CoastGeometryOnly -and $OpenWater) { throw 'Coastal geometry needs a shoreline, not the open-water fixture' }
if ($ColumnSnapshot -and $CoastGeometryOnly) { throw 'Choose geometry-only or column comparison' }
if ($ColumnArtPalette -and !$ColumnSnapshot) { throw 'Art palette requires isolated column snapshot mode' }
if ($ColumnSnapshot -and !(Test-Path -LiteralPath 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/MI_APS_WaterDepth.uasset')) { throw 'Bake the filtered column candidate first' }
if ($ViewFrame.Count -ne 0 -and $ViewFrame.Count -ne 6) { throw 'ViewFrame needs direction XYZ and tangent XYZ' }
foreach ($value in $ViewFrame) { if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) { throw 'Non-finite ViewFrame' } }
$projectRoot = 'F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
# Run only during the coordinated Codex editor window. No process is killed.
$coordination = Get-Content -Encoding UTF8 -LiteralPath ($projectRoot+'/Docs/coordination/PLANET_EDITOR_WINDOW.md')
# ASCII source also works in Windows PowerShell 5.1 (UTF-8 without BOM).
$windowWord = '\u041e\u043a\u043d\u043e'
$releasedWord = '\u043e\u0441\u0432\u043e\u0431\u043e\u0436\u0434\u0435\u043d\u043e'
$acceptedWord = '\u043f\u0440\u0438\u043d\u044f\u043b\u0430'
$lowerWindowWord = '\u043e\u043a\u043d\u043e'
$releases = @($coordination | Select-String -Pattern ('^'+$windowWord+' (Claude|Rio) '+$releasedWord+':'))
$handoffs = @($coordination | Select-String -Pattern ('^'+$windowWord+' '+$releasedWord+':|^Claude '+$acceptedWord+' '+$lowerWindowWord))
if ($WindowToken) {
    # Explicit current window, recorded after checking Claude's latest notes.
    # Any later coordination entry invalidates this lease (fail closed).
    $lastEntry=@($coordination | Where-Object {$_.Trim()})[-1].Trim()
    if($lastEntry -ne ('Codex editor window: '+$WindowToken+' ACTIVE')){throw 'Current explicit editor-window token absent or superseded'}
} elseif (!$releases.Count -or ($handoffs.Count -and $releases[-1].LineNumber -le $handoffs[-1].LineNumber)) {
    throw 'Claude has not explicitly returned the latest editor window; no launch'
}
$coordinationHash=(Get-FileHash -LiteralPath ($projectRoot+'/Docs/coordination/PLANET_EDITOR_WINDOW.md')).Hash
if (Get-Process UnrealEditor,UnrealEditor-Cmd,ShaderCompileWorker,cl,link -ErrorAction SilentlyContinue) {
    throw 'An editor/compiler is active; no session touched'
}
$runDir = 'F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/' + $Family.ToLowerInvariant() + '-' + $Label
if (Test-Path -LiteralPath $runDir) { throw 'Evidence directory exists; choose a fresh label' }
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
$dll = Get-Item -LiteralPath ($projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
foreach ($source in @('Source/APS_ALPHA/Tests/APSWaterNormalABProbe.h',
    'Source/APS_ALPHA/Tests/APSCoastGeometryProbe.h',
    'Source/APS_ALPHA/Tests/APSGeneratedGameplayHandoffSmokeTests.cpp',
    'Source/APS_ALPHA/Tests/APSCoastalReliefTests.cpp',
    'Source/APS_ALPHA/Core/Planetary/APSCoastalRelief.h',
    'Source/APS_ALPHA/Core/Planetary/APSWaterSurfaceFilter.h',
    'Source/APS_ALPHA/Core/Planetary/APSWaterSurfaceLighting.h',
    'Source/APS_ALPHA/Core/Planetary/APSWaterAnalyticWaves.h',
    'Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.cpp',
    'Source/APS_ALPHA/Generation/PlanetarySurfaceGeneratorStreaming.cpp')) {
    if ((Get-Item -LiteralPath ($projectRoot+'/'+$source)).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc) {
        throw 'Coastal diagnostic source newer than DLL: build first'
    }
}
New-Item -ItemType Directory -Path $runDir | Out-Null
$protectedRoots = @('Shared','SharedLiquid','ContinuityV1','WaterV1')
$assetPaths = foreach ($assetRoot in $protectedRoots) {
    Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/'+$assetRoot) -Recurse -File -Filter '*.uasset'
}
$assetPaths | Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath $dll.FullName | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
$tests = 'APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics'
if ($SurfaceFilter) { $tests += '+APS.Gameplay.World.PlanetSurface.WaterSurface.FootprintContract' }
if ($SurfaceAnalytic) { $tests += '+APS.Gameplay.World.PlanetSurface.WaterSurface.AnalyticKernelContract' }
if ($CoastGeometryOnly) { $tests += '+APS.Gameplay.World.PlanetSurface.CoastalRelief' }
$commands='Automation RunTests '+$tests
if($ClearCloudOcclusion){$commands='aps.Surface.Clouds 0,'+$commands}
$arguments = @(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),
    '/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound',
    '-NoLiveCoding','-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=900',
    '-APSDiagnosticOrbitOverview','-APSProbeWaterNormalAB',
    ('-APSDiagnosticPlanet='+$Family),
    ('-APSDiagnosticOrbitHeightKm='+$HeightKm.ToString([Globalization.CultureInfo]::InvariantCulture)),
    ('-APSWaterABDepthM='+$DepthM.ToString([Globalization.CultureInfo]::InvariantCulture)),
    ('-UserDir="'+$runDir+'"'),
    ('-ExecCmds="'+$commands+'"'),
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="'+$runDir+'/report"'), ('-abslog="'+$runDir+'/surface.log"')
)
if ($OpenWater) { $arguments += '-APSWaterABOpenWater' }
if ($CoastalRelief) { $arguments += '-APSProbeCoastalReliefV1' }
if ($CoastGeometryOnly) { $arguments += @('-APSProbeCoastGeometryOnly','-APSProbeDefaultAtmosphere') }
if ($CoastMotion) { $arguments += '-APSProbeCoastCameraMotion' }
if ($CoastTranslate) { $arguments += '-APSProbeCoastCameraTranslation' }
if ($WaterReference) { $arguments += '-APSCoastReferenceWater487132' }
if ($LavaReference) { $arguments += '-APSCoastReferenceLava73875' }
if ($ColumnSnapshot) { $arguments += @('-APSProbeWaterColumnSnapshot','-APSProbeDefaultAtmosphere') }
if ($NativeColumn) { $arguments += @('-APSWaterDepthPayload','-APSProbeWaterColumnNative') }
if ($ColumnArtPalette) { $arguments += '-APSProbeWaterColumnArtPalette' }
if ($SurfaceFilter) { $arguments += @('-APSProbeWaterSurfaceFilter',('-APSWaterSurfaceScaleMultiplier='+$WaveScaleFactor)) }
if ($AnchorSplit) { $arguments += '-APSWaterAnchorSplit' }
if ($AnchorNoise) { $arguments += '-APSWaterAnchorNoise' }
if ($SecondaryDomainAudit) { $arguments += '-APSWaterSecondaryDomainAudit' }
if ($SurfaceAOIsolation) { $arguments += '-APSProbeWaterSurfaceAOIsolation' }
if ($SurfaceNormalBuffer) { $arguments += '-APSProbeWaterNormalBuffer' }
if ($SurfacePrecise) { $arguments += '-APSProbeWaterSurfacePrecise' }
if ($SurfaceRelative) { $arguments += '-APSProbeWaterSurfaceRelative' }
if ($SurfaceWaveIsolation) { $arguments += '-APSProbeWaterSurfaceWaveIsolation' }
if ($SurfaceAnalytic) { $arguments += '-APSProbeWaterAnalytic' }
if ($DomainAudit) { $arguments += '-APSProbeWaterDomainAudit' }
if ($FinalNormalAudit) { $arguments += '-APSWaterFinalNormalAudit' }
if ($SurfacePass) { $arguments += @('-APSProbeWaterSurfacePass','-ini:Engine:[SystemSettings]:r.Water.SingleLayer.ShadersSupportVSMFiltering=1,r.Water.SingleLayer.VSMFiltering=1') }
$frameKeys = @('X','Y','Z','U','V','W')
for ($i=0; $i -lt $ViewFrame.Count; ++$i) {
    $arguments += '-APSWaterABView'+$frameKeys[$i]+'='+$ViewFrame[$i].ToString('G17',[Globalization.CultureInfo]::InvariantCulture)
}
if((Get-FileHash -LiteralPath ($projectRoot+'/Docs/coordination/PLANET_EDITOR_WINDOW.md')).Hash -ne $coordinationHash){throw 'Coordination changed during preflight; no launch'}
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Another editor/compiler started; no launch'}
$process = Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{ Id=$process.Id; StartTime=$process.StartTime; Evidence=$runDir } | ConvertTo-Json
