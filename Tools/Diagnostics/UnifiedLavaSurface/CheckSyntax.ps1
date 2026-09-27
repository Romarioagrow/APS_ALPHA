param([string]$OutputDirectory = 'F:/ChatGPT/APOSFERA/work/unified_lava_20260927/checks')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$engineSource = 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Source'
$compiler = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe'
$responseRoot = Join-Path $project 'Intermediate/Build/Win64/x64/UnrealEditor/Development/APS_ALPHA'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
function Expand-Response([string]$Path) {
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^@"(.+)"$') { Expand-Response $Matches[1] } else { $line }
    }
}
$targets = @{
    'Noise' = @('APSWorldScapePlanetNoise', 'Generation/APSWorldScapePlanetNoise.cpp')
    'Streaming' = @('PlanetarySurfaceGeneratorStreaming', 'Generation/PlanetarySurfaceGeneratorStreaming.cpp')
    'Builder' = @('APSPlanetSurfaceAssetCommandlet', 'Editor/APSPlanetSurfaceAssetCommandlet.cpp')
    'Tests' = @('APSPlanetSurfaceProfileTests', 'Tests/APSWorldScapeSurfaceEnvelopeTests.cpp')
}
foreach ($key in @('Noise', 'Streaming', 'Builder', 'Tests')) {
    $target = $targets[$key]
    $argsList = [System.Collections.Generic.List[string]]::new()
    foreach ($line in Expand-Response (Join-Path $responseRoot ($target[0] + '.cpp.obj.rsp'))) {
        if ($line -match '^".*\.cpp"$' -or $line -match '^/(Fo|Fd|sourceDependencies|errorReport|analyze)' -or $line -eq '/c') { continue }
        if ($line -match '^/(I|external:I)\s*"(.+)"$') {
            $option = $Matches[1]; $path = $Matches[2]
            if (-not [IO.Path]::IsPathRooted($path)) { $path = [IO.Path]::GetFullPath((Join-Path $engineSource $path)) }
            $argsList.Add('/' + $option + ' "' + $path + '"')
        } else { $argsList.Add($line) }
    }
    $argsList.Add('/Zs'); $argsList.Add('/errorReport:none')
    $argsList.Add('"' + (Join-Path $project ('Source/APS_ALPHA/' + $target[1])) + '"')
    $rsp = Join-Path $OutputDirectory ($key + '.rsp')
    # Mechanical derivative of current UBT response; only scratch output paths.
    [IO.File]::WriteAllLines($rsp, $argsList, [Text.UTF8Encoding]::new($false))
    Push-Location $engineSource
    try { & $compiler ('@' + $rsp); if ($LASTEXITCODE -ne 0) { throw "Syntax failed: $key" } }
    finally { Pop-Location }
    Write-Output "PASS syntax: $key (no UHT, linker, DLL, bake or Unreal launch)"
}
