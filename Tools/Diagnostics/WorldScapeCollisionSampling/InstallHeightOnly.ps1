param([switch]$Rollback,
    [ValidatePattern('^collision-height-private-v[0-9]+$')][string]$TestLabel='collision-height-private-v1',
    [ValidatePattern('^collision-height-install-v[0-9]+$')][string]$BackupLabel='collision-height-install-v1')
$ErrorActionPreference='Stop'
$project='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$installed='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$candidate='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild/Plugins/WorldScape'
$base='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929'
$stage=$base+'/collision-height-stage-20261001'
$header=$project+'/Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.h'
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
           !$target.StartsWith([IO.Path]::GetFullPath($project+'/Binaries/Win64')+[IO.Path]::DirectorySeparatorChar) -and
           $target -ne [IO.Path]::GetFullPath($header)) { throw 'Unsafe rollback path' }
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
    # Copy-Item preserves old timestamps. UBT can otherwise reuse candidate .obj
    # files after an ABI/header rollback and link a mixed build on the next run.
    # Invalidate header dependents without changing their restored byte content.
    foreach($entry in $entries) {
        if($entry.Before -and $entry.Target.EndsWith('.h')) {
            (Get-Item -LiteralPath $entry.Target).LastWriteTimeUtc=[DateTime]::UtcNow
        }
    }
    'Restored exact pre-install plugin, APS header and binaries. Only newly installed candidate sources removed; backups retained.'
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
$source=@('Private/WorldScapeRoot_Main.cpp','Private/WorldScapeRoot_Collision.cpp',
    'Private/Tests/APSWorldScapeCollisionSamplingTests.cpp','Public/WorldScapeLod.h','Public/WorldScapeRoot.h')
$relative=@($source|ForEach-Object {'Source/WorldScapeCore/'+$_})
$relative+='Source/WorldScapeNoise/Public/WorldScapeNoiseClass.h'
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
$header=$project+'/Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.h'
$relative+='Intermediate/Build/Win64/UnrealEditor/Inc/WorldScapeNoise/UHT/WorldScapeNoiseClass.generated.h'
$headerBefore=Get-Content -Raw -LiteralPath ($stage+'/header-before.json')|ConvertFrom-Json
if((Get-FileHash -LiteralPath $header).Hash -ne $headerBefore.Hash) { throw 'APS header changed since staging; no files installed' }
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
$entries+=[pscustomobject]@{Source=($stage+'/APSWorldScapePlanetNoise.h');Target=$header;Backup=($backup+'/project/APSWorldScapePlanetNoise.h');Before=$headerBefore.Hash;Candidate=(Get-FileHash -LiteralPath ($stage+'/APSWorldScapePlanetNoise.h')).Hash}
foreach($entry in $entries) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $entry.Backup)|Out-Null
    if($entry.Before) { Copy-Item -LiteralPath $entry.Target -Destination $entry.Backup }
}
$entries|ConvertTo-Json -Depth 4|Out-File ($backup+'/manifest.json') -Encoding utf8
try {
    foreach($entry in $entries) { if($entry.Source) { Copy-Item -LiteralPath $entry.Source -Destination $entry.Target -Force } }
    foreach($entry in $entries) {
        if($entry.Source -and $entry.Target.EndsWith('.h')) {
            (Get-Item -LiteralPath $entry.Target).LastWriteTimeUtc=[DateTime]::UtcNow
        }
    }
    # Base noise vtable and LOD layout changed: ALL plugin modules and APS must match.
    & 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat' APS_ALPHAEditor Win64 Development ('-Project='+$project+'/APS_ALPHA.uproject') -NoHotReloadFromIDE -NoXGE -MaxParallelActions=2 -WaitMutex ('-Log='+$backup+'/aps-build.log')
    if($LASTEXITCODE -ne 0) { throw ('APS rebuild failed: '+$LASTEXITCODE) }
    $entries|ForEach-Object {[pscustomobject]@{Target=$_.Target;Hash=(Get-FileHash -LiteralPath $_.Target).Hash}}|ConvertTo-Json|Out-File ($backup+'/installed-hashes.json') -Encoding utf8
    'Coherent plugin/APS installed; height-only remains default OFF. Runtime integration/flight validation still required; no assets changed.'
} catch {
    Restore-Publication -FailedBuild
    throw
}
