# Dot-source only. These helpers do not launch Unreal or change project settings.
function Get-APSCloudWeatherDiagnostic {
    param([string]$ProjectRoot,[switch]$Candidate,[switch]$LayeredCandidate)
    if($Candidate -and $LayeredCandidate){throw 'Select one cloud candidate: refined V31 or layered V30'}
    $header=Join-Path $ProjectRoot 'Source/APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h'
    $constant=if($LayeredCandidate){'LayeredMaterialPath'}elseif($Candidate){'CandidateMaterialPath'}else{'MaterialPath'}
    $match=[regex]::Match((Get-Content -LiteralPath $header -Raw),('(?m)^inline constexpr const TCHAR\* '+$constant+'=TEXT\("([^"]+)"\);'))
    $version=if($LayeredCandidate){'CloudWeather20261002V30'}elseif($Candidate){'CloudWeather20261003V31'}else{'CloudWeather20261002V27'}
    $expected='/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'+$version+'/M_APS_PlanetCloud.M_APS_PlanetCloud'
    if(!$match.Success -or $match.Groups[1].Value -cne $expected){throw 'Cloud diagnostic path differs from its constrained policy constant'}
    $sources=@('Core/Planetary/APSPlanetCloudWeather.h','Core/Planetary/APSPlanetCloudSettings.h',
        'Core/Planetary/APSPlanetCloudPolicy.h','Core/Planetary/APSPlanetCloudLayers.h','Core/Rendering/APSPlanetCloudComponent.cpp',
        'Core/Rendering/APSPlanetCloudComponent.h','Editor/APSPlanetCloudBuilder.h','Editor/APSPlanetCloudHlsl.h',
        'Editor/APSPlanetCloudLayeredHlsl.h','Editor/APSPlanetCloudRefinedHlsl.h',
        'Editor/APSPlanetSurfaceAssetCommandlet.cpp','Tests/APSCloudFlightProbe.h',
        'Tests/APSPlanetCloudWeatherTests.cpp','Tests/APSPlanetCloudLayersTests.cpp','Tests/APSPlanetCloudRefinedTests.cpp',
        'Tests/APSPlanetRefinementRenderedTests.cpp','Tests/APSGeneratedGameplayHandoffSmokeTests.cpp') |
        ForEach-Object {Join-Path $ProjectRoot ('Source/APS_ALPHA/'+$_)}
    $dll=Get-Item -LiteralPath (Join-Path $ProjectRoot 'Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
    foreach($source in $sources){
        if((Get-Item -LiteralPath $source).LastWriteTimeUtc -gt $dll.LastWriteTimeUtc){throw ('Cloud diagnostic source newer than DLL: '+$source)}
    }
    [pscustomobject]@{ObjectPath=$expected;Asset=(Join-Path $ProjectRoot ('Content/'+$expected.Substring(6).Split('.')[0]+'.uasset'));Sources=$sources;Candidate=[bool]$Candidate;LayeredCandidate=[bool]$LayeredCandidate}
}
function Save-APSCloudWeatherDiagnostic {
    param([string]$ProjectRoot,[string]$RunDir,$Selection,[string]$FeatureScale='')
    $Selection.Sources | ForEach-Object {Get-FileHash -LiteralPath $_ -Algorithm SHA256} |
        Select-Object Path,Hash | ConvertTo-Json | Out-File -LiteralPath (Join-Path $RunDir 'cloud-sources.json') -Encoding utf8
    $protected=@('Config/DefaultEngine.ini','Config/DefaultGame.ini',
        'Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261002V27/M_APS_PlanetCloud.uasset',
        'Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261001V26/M_APS_PlanetCloud.uasset',
        'Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudVolume20261001V24/M_APS_PlanetCloud.uasset') |
        ForEach-Object {Join-Path $ProjectRoot $_}
    # Keep independently baked candidates intact when testing a different one.
    foreach($version in @('CloudWeather20261002V28','CloudWeather20261002V29','CloudWeather20261002V30','CloudWeather20261003V31')){
        $otherCandidate=Join-Path $ProjectRoot ('Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'+$version+'/M_APS_PlanetCloud.uasset')
        if($otherCandidate -ne $Selection.Asset -and (Test-Path -LiteralPath $otherCandidate)){$protected+=$otherCandidate}
    }
    # The accepted near-water parent is ShoreTransmission, not WaterV1 alone.
    # A cloud-only bake must preserve the installed lava graph as well.
    foreach($folder in @('Shared','SharedLiquid','ContinuityV1','WaterV1',
        'Diagnostics/WaterShoreTransmission20261001','UnifiedLava')){
        $protected+=@(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot ('Content/APS/APS_ALPHA/WSC/PlanetSurface/'+$folder)) -File -Recurse -Filter '*.uasset' -ErrorAction Stop | ForEach-Object {$_.FullName})
    }
    $protected | Sort-Object -Unique | ForEach-Object {Get-FileHash -LiteralPath $_ -Algorithm SHA256} |
        Select-Object Path,Hash | ConvertTo-Json | Out-File -LiteralPath (Join-Path $RunDir 'cloud-protected-before.json') -Encoding utf8
    [pscustomobject]@{ObjectPath=$Selection.ObjectPath;Candidate=$Selection.Candidate;LayeredCandidate=$Selection.LayeredCandidate;FeatureScale=$FeatureScale;FrozenWind=($FeatureScale -ne '')} |
        ConvertTo-Json | Out-File -LiteralPath (Join-Path $RunDir 'cloud-selection.json') -Encoding utf8
    if(Test-Path -LiteralPath $Selection.Asset){
        $identity=Get-FileHash -LiteralPath $Selection.Asset -Algorithm SHA256
        [pscustomobject]@{ObjectPath=$Selection.ObjectPath;Path=$identity.Path;Hash=$identity.Hash} |
            ConvertTo-Json | Out-File -LiteralPath (Join-Path $RunDir 'cloud-asset.json') -Encoding utf8
    }
}
function Assert-APSCloudWeatherProtection {
    param([Parameter(Mandatory)][string]$RunDir)
    foreach($item in (Get-Content -LiteralPath (Join-Path $RunDir 'cloud-protected-before.json') -Raw | ConvertFrom-Json)){
        if((Get-FileHash -LiteralPath $item.Path -Algorithm SHA256).Hash -cne $item.Hash){throw ('Protected cloud baseline changed: '+$item.Path)}
    }
    'Cloud baseline/config hashes unchanged; this does not establish rendered acceptance.'
}
