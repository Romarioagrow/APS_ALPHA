param([Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string[]]$SourceTargets = @('Generation/APSPlanetReliefField.cpp', 'Tests/APSPlanetReliefFieldTests.cpp', 'Core/Planetary/APSPlanetReliefTexture.cpp'),
    [switch]$SkipMath)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$engineSource = 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Source'
$compilerRoot = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717'
$compiler = Join-Path $compilerRoot 'bin/Hostx64/x64/cl.exe'
$sdkRoot = 'C:/Program Files (x86)/Windows Kits/10'
$sdkVersion = '10.0.26100.0'
$responseRoot = Join-Path $project 'Intermediate/Build/Win64/x64/UnrealEditor/Development/APS_ALPHA'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
function Expand-Response([string]$Path) {
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^@"(.+)"$') { Expand-Response $Matches[1] } else { $line }
    }
}
# Read existing UBT arguments; never run UBT/UHT/link Unreal or replace its DLL.
foreach ($target in $SourceTargets) {
    $argsList = [System.Collections.Generic.List[string]]::new()
    foreach ($line in Expand-Response (Join-Path $responseRoot 'APSClosedGlobeMesh.cpp.obj.rsp')) {
        if ($line -match '^".*\.cpp"$' -or $line -match '^/(Fo|Fd|Fp|Yu|Yc|sourceDependencies|errorReport|analyze)' -or $line -eq '/c') { continue }
        $argsList.Add($line)
    }
    $argsList.Add('/Zs'); $argsList.Add('/Y-'); $argsList.Add('/errorReport:none')
    $argsList.Add('"' + (Join-Path $project ('Source/APS_ALPHA/' + $target)) + '"')
    $name = [IO.Path]::GetFileNameWithoutExtension($target)
    $rsp = Join-Path $output ($name + '.rsp')
    # Mechanical derivative of an existing response, generated only in scratch.
    [IO.File]::WriteAllLines($rsp, $argsList, [Text.UTF8Encoding]::new($false))
    Push-Location $engineSource
    try {
        & $compiler ('@' + $rsp) 2>&1 | Tee-Object -FilePath (Join-Path $output ($name + '.log'))
        if ($LASTEXITCODE -ne 0) { throw "Syntax failed: $target" }
    } finally { Pop-Location }
    Write-Output "PASS source syntax: $target (not a linked Unreal build or rendered proof)"
}
if ($SkipMath) { Write-Output 'Syntax only; no UBT/UE/bake/link/visual claim.'; exit 0 }
$standalone = Join-Path $PSScriptRoot 'NormalMathContracts.cpp'
$binary = Join-Path $output 'NormalMathContracts.exe'
$compilerArgs = @('/nologo', '/std:c++20', '/EHsc', '/W4', '/WX', '/MT',
    ('/I' + (Join-Path $compilerRoot 'include')),
    ('/I' + (Join-Path $sdkRoot ('Include/' + $sdkVersion + '/ucrt'))),
    ('/Fo' + (Join-Path $output 'NormalMathContracts.obj')), ('/Fe' + $binary), $standalone,
    '/link', ('/LIBPATH:' + (Join-Path $compilerRoot 'lib/x64')),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkVersion + '/ucrt/x64'))),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkVersion + '/um/x64'))))
& $compiler @compilerArgs 2>&1 | Tee-Object -FilePath (Join-Path $output 'NormalMathBuild.log')
if ($LASTEXITCODE -ne 0) { throw 'Portable normal math build failed' }
& $binary | Tee-Object -FilePath (Join-Path $output 'NormalMathContracts.log')
if ($LASTEXITCODE -ne 0) { throw 'Portable normal math contracts failed' }
Write-Output 'No Unreal launch, shader/material bake, DLL link, or visual claim.'
