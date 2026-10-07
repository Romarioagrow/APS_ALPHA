param(
    [string]$OutputDirectory='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/coast-mesh-20261003/checks',
    [switch]$SyntaxOnly
)
$ErrorActionPreference='Stop'
$project=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$toolchain='C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717'
$compiler=Join-Path $toolchain 'bin/Hostx64/x64/cl.exe'
$kit='C:/Program Files (x86)/Windows Kits/10'
$kitVersion='10.0.26100.0'
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
if(!$SyntaxOnly){
    $exe=Join-Path $OutputDirectory 'CoastalMeshCheck.exe'
    $object=Join-Path $OutputDirectory 'CoastalMeshCheck.obj'
    & $compiler /nologo /EHsc /std:c++20 /MT /W4 /WX `
        "/I$toolchain/include" "/I$kit/Include/$kitVersion/ucrt" `
        "/Fo$object" "/Fe$exe" "$PSScriptRoot/Tests/APSCoastalMeshRefinementTests.cpp" `
        /link "/LIBPATH:$toolchain/lib/x64" "/LIBPATH:$kit/Lib/$kitVersion/ucrt/x64" "/LIBPATH:$kit/Lib/$kitVersion/um/x64" 2>&1 |
        Tee-Object -FilePath (Join-Path $OutputDirectory 'compile.txt')
    if($LASTEXITCODE -ne 0){throw 'Standalone coastal topology compilation failed'}
    & $exe 2>&1 | Tee-Object -FilePath (Join-Path $OutputDirectory 'topology.txt')
    if($LASTEXITCODE -ne 0){throw 'Standalone coastal topology tests failed'}
}

# Syntax only: consume the installed UBT settings, without writing object/PCH/
# dependency/PDB/DLL files or invoking UHT, UBT, the linker or an Unreal process.
$engineSource='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Source'
$response=Join-Path $project 'Intermediate/Build/Win64/x64/UnrealEditor/Development/APS_ALPHA/AstroGenerator.cpp.obj.rsp'
function Expand-CoastResponse([string]$Path){
    foreach($line in Get-Content -LiteralPath $Path){
        if($line -match '^@"(.+)"$'){Expand-CoastResponse $Matches[1]}else{$line}
    }
}
$arguments=[System.Collections.Generic.List[string]]::new()
foreach($line in Expand-CoastResponse $response){
    if($line -match '^/(Fo|Fd|Fp|Yu|Yc|sourceDependencies|errorReport|analyze)' -or $line -eq '/c'){continue}
    if($line -match '^/(I|external:I)\s*"(.+)"$'){
        $option=$Matches[1]; $includePath=$Matches[2]
        if(![IO.Path]::IsPathRooted($includePath)){$includePath=[IO.Path]::GetFullPath((Join-Path $engineSource $includePath))}
        $arguments.Add('/'+$option+' "'+$includePath+'"')
    }else{$arguments.Add($line)}
}
$arguments.Add('/Zs'); $arguments.Add('/Y-'); $arguments.Add('/errorReport:none')
$scratchResponse=Join-Path $OutputDirectory 'AstroGenerator-syntax.rsp'
# Mechanical derivative of current UBT response, not a source/config edit.
[IO.File]::WriteAllLines($scratchResponse,$arguments,[Text.UTF8Encoding]::new($false))
Push-Location $engineSource
try{
    & $compiler ('@'+$scratchResponse) 2>&1 | Tee-Object -FilePath (Join-Path $OutputDirectory 'syntax.txt')
    if($LASTEXITCODE -ne 0){throw 'AstroGenerator/coastal integration syntax failed'}
}finally{Pop-Location}
'PASS: coastal source syntax; no DLL, asset, runtime or visual acceptance.'
