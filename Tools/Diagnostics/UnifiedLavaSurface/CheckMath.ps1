param([string]$OutputDirectory = 'F:/ChatGPT/APOSFERA/work/unified_lava_20260927/checks')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$toolchain = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717'
$kit = 'C:/Program Files (x86)/Windows Kits/10'
$version = '10.0.26100.0'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$exe = Join-Path $OutputDirectory 'EnvelopeCheck.exe'
$object = Join-Path $OutputDirectory 'EnvelopeCheck.obj'
& "$toolchain/bin/Hostx64/x64/cl.exe" /nologo /EHsc /std:c++20 /MT /W4 `
    "/I$project/Source" "/I$toolchain/include" "/I$kit/Include/$version/ucrt" `
    "/Fo$object" "/Fe$exe" "$PSScriptRoot/EnvelopeCheck.cpp" `
    /link "/LIBPATH:$toolchain/lib/x64" "/LIBPATH:$kit/Lib/$version/ucrt/x64" "/LIBPATH:$kit/Lib/$version/um/x64"
if ($LASTEXITCODE -ne 0) { throw 'Standalone math check compilation failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Standalone math check failed' }
