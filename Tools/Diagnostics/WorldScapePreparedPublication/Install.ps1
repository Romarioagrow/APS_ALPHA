param([switch]$Rollback,
    [ValidatePattern('^prepared-publication-private-v[0-9]+$')][string]$TestLabel='prepared-publication-private-v3',
    [ValidatePattern('^prepared-publication-install-v[0-9]+$')][string]$BackupLabel='prepared-publication-install-v3')
$ErrorActionPreference='Stop'
$project='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$installed='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$candidate='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild/Plugins/WorldScape'
$base='F:/ChatGPT/APOSFERA/work/planet_water_live_20260929'
$backup=$base+'/'+$BackupLabel
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue) {
    throw 'Editor/compiler active; no process touched'
}
function Restore-Publication([switch]$FailedBuild) {
    $entries=Get-Content -Raw -LiteralPath ($backup+'/manifest.json')|ConvertFrom-Json
    $post=@{}
    if(Test-Path -LiteralPath ($backup+'/installed-hashes.json')) {
        Get-Content -Raw -LiteralPath ($backup+'/installed-hashes.json')|ConvertFrom-Json|ForEach-Object {$post[$_.Target]=$_.Hash}
    }
    # Preflight the complete transaction before touching any installed file.
    foreach($entry in $entries) {
        $target=[IO.Path]::GetFullPath($entry.Target)
        if(!$target.StartsWith([IO.Path]::GetFullPath($installed)+[IO.Path]::DirectorySeparatorChar) -and
           !$target.StartsWith([IO.Path]::GetFullPath($project+'/Binaries/Win64')+[IO.Path]::DirectorySeparatorChar)) { throw 'Unsafe rollback path' }
        if($entry.Before -and (Get-FileHash -LiteralPath $entry.Backup).Hash -ne $entry.Before) { throw 'Backup hash mismatch' }
        if(Test-Path -LiteralPath $target) {
            $actual=(Get-FileHash -LiteralPath $target).Hash
            $allowed=@($entry.Before,$entry.Candidate,$post[$entry.Target])
            if($actual -notin $allowed -and !($FailedBuild -and !$entry.Source)) { throw ('Target changed outside this transaction: '+$target) }
        }
    }
    foreach($entry in $entries) {
        if($entry.Before) { Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force }
        elseif(Test-Path -LiteralPath $entry.Target) { Remove-Item -LiteralPath $entry.Target }
    }
    'Restored the exact pre-install plugin and APS binaries. Only newly installed candidate sources removed; backups retained.'
}
if($Rollback) { Restore-Publication; exit 0 }
if(Test-Path -LiteralPath $backup) { throw 'Backup exists; choose a fresh transaction label' }
$testDir=$base+'/'+$TestLabel
$report=Get-Content -Raw -LiteralPath ($testDir+'/report/index.json')|ConvertFrom-Json
if($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.succeeded -ne 8 -or $report.succeededWithWarnings -ne 0) { throw 'Require eight clean private tests' }
$tested=Get-Content -Raw -LiteralPath ($testDir+'/native-dll.json')|ConvertFrom-Json
if($tested.Hash -ne (Get-FileHash -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor-WorldScapeCore.dll')).Hash) { throw 'Candidate changed since private tests' }
$oldModules=Get-Content -Raw -LiteralPath ($installed+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
$newModules=Get-Content -Raw -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
if($oldModules.BuildId -ne $newModules.BuildId -or $oldModules.BuildId -ne '33043543') { throw 'Engine BuildId mismatch' }
$source=@('Private/WorldScapeLod.cpp','Private/WorldScapeMeshComponent.cpp','Private/WorldScapePreparedMesh.cpp',
    'Private/WorldScapeRoot_Main.cpp','Private/WorldScapeRoot_Thread.cpp',
    'Private/Tests/APSWorldScapeMeshPublicationTests.cpp','Private/Tests/APSWorldScapeWorkerCompletionTests.cpp',
    'Public/WorldScapeLod.h','Public/WorldScapeMeshComponent.h','Public/WorldScapePreparedMesh.h','Public/WorldScapeRoot.h')
$relative=@($source|ForEach-Object {'Source/WorldScapeCore/'+$_})
foreach($file in Get-ChildItem -LiteralPath ($candidate+'/Source') -File -Recurse) {
    $rel=$file.FullName.Substring($candidate.Length+1).Replace('\','/')
    if($relative -contains $rel) { continue }
    $other=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $other) -or
        [IO.File]::ReadAllText($file.FullName).Replace("`r`n","`n") -cne [IO.File]::ReadAllText($other).Replace("`r`n","`n")) { throw ('Unrelated candidate source: '+$rel) }
}
foreach($module in $newModules.Modules.psobject.Properties.Name) {
    $relative+=@(('Binaries/Win64/UnrealEditor-'+$module+'.dll'),('Binaries/Win64/UnrealEditor-'+$module+'.pdb'),
        ('Intermediate/Build/Win64/x64/UnrealEditor/Development/'+$module+'/UnrealEditor-'+$module+'.lib'))
}
$relative+='Binaries/Win64/UnrealEditor.modules'
foreach($header in @('WorldScapeRoot','WorldScapeLod','WorldScapeMeshComponent')) {
    $relative+=('Intermediate/Build/Win64/UnrealEditor/Inc/WorldScapeCore/UHT/'+$header+'.generated.h')
}
$entries=@()
foreach($rel in $relative) {
    $src=Join-Path $candidate $rel; $dst=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $src)) { throw ('Missing candidate: '+$src) }
    $entries+=[pscustomobject]@{Source=$src;Target=$dst;Backup=($backup+'/plugin/'+$rel);Before=$(if(Test-Path -LiteralPath $dst){(Get-FileHash -LiteralPath $dst).Hash}else{$null});Candidate=(Get-FileHash -LiteralPath $src).Hash}
}
foreach($name in @('UnrealEditor-APS_ALPHA.dll','UnrealEditor-APS_ALPHA.pdb','UnrealEditor.modules')) {
    $dst=$project+'/Binaries/Win64/'+$name
    $entries+=[pscustomobject]@{Source=$null;Target=$dst;Backup=($backup+'/project/'+$name);Before=(Get-FileHash -LiteralPath $dst).Hash;Candidate=$null}
}
foreach($entry in $entries) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $entry.Backup)|Out-Null
    if($entry.Before) { Copy-Item -LiteralPath $entry.Target -Destination $entry.Backup }
}
$entries|ConvertTo-Json -Depth 4|Out-File ($backup+'/manifest.json') -Encoding utf8
try {
    foreach($entry in $entries) { if($entry.Source) { Copy-Item -LiteralPath $entry.Source -Destination $entry.Target -Force } }
    # FAsyncTask includes the task's C++ layout: APS consumers MUST be rebuilt.
    & 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat' APS_ALPHAEditor Win64 Development ('-Project='+$project+'/APS_ALPHA.uproject') -NoHotReloadFromIDE -NoXGE -MaxParallelActions=2 -WaitMutex ('-Log='+$backup+'/aps-build.log')
    if($LASTEXITCODE -ne 0) { throw ('APS rebuild failed: '+$LASTEXITCODE) }
    $entries|ForEach-Object {[pscustomobject]@{Target=$_.Target;Hash=(Get-FileHash -LiteralPath $_.Target).Hash}}|ConvertTo-Json|Out-File ($backup+'/installed-hashes.json') -Encoding utf8
    'Coherent plugin/APS installed. Native policy and APS owner tags determine scope; runtime validation still required. No assets changed.'
} catch {
    Restore-Publication -FailedBuild
    throw
}
