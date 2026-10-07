param([switch]$Rollback,
    [ValidatePattern('^mesh-publication-install-v[0-9]+$')][string]$BackupLabel='mesh-publication-install-v1',
    [ValidatePattern('^mesh-publication-private-v[0-9]+$')][string]$EvidenceLabel='mesh-publication-private-v2')
$ErrorActionPreference='Stop'
$installed='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$candidate='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild/Plugins/WorldScape'
$backup='F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/'+$BackupLabel
$evidence='F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/'+$EvidenceLabel
if(Get-Process UnrealEditor,UnrealEditor-Cmd,UnrealInsights,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue) {throw 'Editor/compiler/profiler active; nothing changed'}
function Restore-Publication {
    $entries=Get-Content -Raw -LiteralPath ($backup+'/manifest.json')|ConvertFrom-Json
    foreach($entry in $entries) {
        $target=[IO.Path]::GetFullPath($entry.Target)
        if(!$target.StartsWith([IO.Path]::GetFullPath($installed)+[IO.Path]::DirectorySeparatorChar)) {throw 'Unsafe target'}
        if((Test-Path -LiteralPath $target) -and (Get-FileHash -LiteralPath $target).Hash -notin @($entry.Before,$entry.Candidate)) {throw ('Target changed since install: '+$target)}
    }
    foreach($entry in $entries) {
        if($entry.Before) {Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force}
        elseif(Test-Path -LiteralPath $entry.Target) {Remove-Item -LiteralPath $entry.Target}
    }
    'Restored Core binary/import library and source; only added parity-test source removed.'
}
if($Rollback) {Restore-Publication; exit 0}
if(Test-Path -LiteralPath $backup) {throw 'Backup already exists'}
$report=Get-Content -Raw -LiteralPath ($evidence+'/report/index.json')|ConvertFrom-Json
if($report.failed -ne 0 -or $report.notRun -ne 0 -or ($report.succeeded+$report.succeededWithWarnings) -ne 8) {throw 'Private 8-test suite must pass'}
$tested=Get-Content -Raw -LiteralPath ($evidence+'/native-dll.json')|ConvertFrom-Json
if($tested.Hash -ne (Get-FileHash -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor-WorldScapeCore.dll')).Hash) {throw 'Candidate differs from tested DLL'}
$oldModules=Get-Content -Raw -LiteralPath ($installed+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
$newModules=Get-Content -Raw -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
if($oldModules.BuildId -ne $newModules.BuildId -or $newModules.BuildId -ne '33043543') {throw 'BuildId mismatch'}
$sources=@('Source/WorldScapeCore/Private/WorldScapeMeshComponent.cpp','Source/WorldScapeCore/Private/Tests/APSWorldScapeMeshPublicationTests.cpp')
# No public headers, generated layout or other module source may differ.
foreach($file in Get-ChildItem -LiteralPath ($candidate+'/Source') -File -Recurse) {
    $rel=$file.FullName.Substring($candidate.Length+1).Replace('\','/')
    if($sources -contains $rel) {continue}
    $other=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $other) -or [IO.File]::ReadAllText($file.FullName).Replace("`r`n","`n") -cne [IO.File]::ReadAllText($other).Replace("`r`n","`n")) {throw ('Unrelated source differs: '+$rel)}
}
$relative=$sources+@('Binaries/Win64/UnrealEditor-WorldScapeCore.dll','Binaries/Win64/UnrealEditor-WorldScapeCore.pdb','Intermediate/Build/Win64/x64/UnrealEditor/Development/WorldScapeCore/UnrealEditor-WorldScapeCore.lib')
$entries=@()
foreach($rel in $relative) {
    $src=Join-Path $candidate $rel; $dst=Join-Path $installed $rel
    $entries+=[pscustomobject]@{Source=$src;Target=$dst;Backup=($backup+'/'+$rel);Before=$(if(Test-Path -LiteralPath $dst){(Get-FileHash -LiteralPath $dst).Hash}else{$null});Candidate=(Get-FileHash -LiteralPath $src).Hash}
}
foreach($entry in $entries) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $entry.Backup)|Out-Null
    if($entry.Before) {Copy-Item -LiteralPath $entry.Target -Destination $entry.Backup}
}
$entries|ConvertTo-Json -Depth 4|Out-File ($backup+'/manifest.json') -Encoding utf8
try {
    foreach($entry in $entries) {Copy-Item -LiteralPath $entry.Source -Destination $entry.Target -Force}
    foreach($entry in $entries) {if((Get-FileHash -LiteralPath $entry.Target).Hash -ne $entry.Candidate) {throw 'Post-copy hash mismatch'}}
    'Installed narrow Core implementation (no API/layout change). APS DLL/assets untouched; runtime A/B still required.'
} catch {Restore-Publication; throw}
