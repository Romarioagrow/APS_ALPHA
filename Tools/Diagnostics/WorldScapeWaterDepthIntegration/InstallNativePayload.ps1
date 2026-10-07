param([switch]$Rollback,
    [ValidatePattern('^before-installed(?:-v[0-9]+)?$')][string]$BackupLabel='before-installed-v3')
$ErrorActionPreference='Stop'
# Coordinated offline transaction: native class layout changes, so ALL plugin
# modules and APS must match before another editor is allowed to start.
$project='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$installed='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$candidate='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild/Plugins/WorldScape'
$backup='F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/'+$BackupLabel
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue) {
    throw 'Editor/compiler active; no process touched'
}
function Restore-Payload {
    $entries=Get-Content -Raw -LiteralPath ($backup+'/manifest.json')|ConvertFrom-Json
    foreach($entry in $entries) {
        $target=[IO.Path]::GetFullPath($entry.Target)
        if(!$target.StartsWith([IO.Path]::GetFullPath($installed) + [IO.Path]::DirectorySeparatorChar) -and
           !$target.StartsWith([IO.Path]::GetFullPath($project+'/Binaries/Win64') + [IO.Path]::DirectorySeparatorChar)) {
            throw ('Unsafe rollback target: '+$target)
        }
        if($entry.Before) { Copy-Item -LiteralPath $entry.Backup -Destination $target -Force }
        elseif(Test-Path -LiteralPath $target) {
            if((Get-FileHash -LiteralPath $target).Hash -ne $entry.Candidate) { throw 'New source changed after install; refusing removal' }
            Remove-Item -LiteralPath $target
        }
    }
    'Restored backed-up plugin source/binaries and APS binaries; no assets removed.'
}
if($Rollback) { Restore-Payload; exit 0 }
if(Test-Path -LiteralPath $backup) { throw 'Backup exists; inspect previous transaction first' }
$report=Get-Content -Raw -LiteralPath 'F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/native-payload-v2/report/index.json'|ConvertFrom-Json
if($report.failed -ne 0 -or $report.notRun -ne 0 -or ($report.succeeded+$report.succeededWithWarnings) -ne 7) { throw 'Native suite must pass all seven tests' }
$oldModules=Get-Content -Raw -LiteralPath ($installed+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
$newModules=Get-Content -Raw -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
if($oldModules.BuildId -ne $newModules.BuildId -or $oldModules.BuildId -ne '33043543') { throw 'Engine BuildId mismatch' }
$source=@('Private/LodData.cpp','Private/WorldScapeLod.cpp','Private/WorldScapeOceanDepthTestNoise.h',
    'Private/WorldScapeOceanDepthTests.cpp','Private/WorldScapeOceanDepthWorkerTests.cpp',
    'Private/WorldScapeRoot_Thread.cpp','Public/LodData.h','Public/WorldScapeLod.h','Public/WorldScapeOceanDepth.h')
$relative=@($source|ForEach-Object {'Source/WorldScapeCore/'+$_})
# Refuse to carry unrelated source changes inside a rebuilt DLL. Ignore CRLF only.
foreach($file in Get-ChildItem -LiteralPath ($candidate+'/Source') -File -Recurse) {
    $rel=$file.FullName.Substring($candidate.Length+1).Replace('\','/')
    if($relative -contains $rel) { continue }
    $other=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $other) -or
       [IO.File]::ReadAllText($file.FullName).Replace("`r`n","`n") -cne [IO.File]::ReadAllText($other).Replace("`r`n","`n")) {
        throw ('Unrelated private source differs: '+$rel)
    }
}
foreach($module in $newModules.Modules.psobject.Properties.Name) {
    $relative+=@(('Binaries/Win64/UnrealEditor-'+$module+'.dll'),('Binaries/Win64/UnrealEditor-'+$module+'.pdb'),
        ('Intermediate/Build/Win64/x64/UnrealEditor/Development/'+$module+'/UnrealEditor-'+$module+'.lib'))
}
$relative+='Binaries/Win64/UnrealEditor.modules'
# Installed engine plugins use their precompiled UHT headers. GENERATED_BODY
# macros are line-number keyed: source, generated headers and DLL must agree.
foreach($header in @('LodData','WorldScapeLod','WorldScapeOceanDepthTestNoise')) {
    $relative+=('Intermediate/Build/Win64/UnrealEditor/Inc/WorldScapeCore/UHT/'+$header+'.generated.h')
}
$entries=@()
foreach($rel in $relative) {
    $src=Join-Path $candidate $rel; $dst=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $src)) { throw ('Missing candidate file: '+$src) }
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
    foreach($entry in $entries) {
        if($entry.Source) { Copy-Item -LiteralPath $entry.Source -Destination $entry.Target -Force }
    }
    & 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat' APS_ALPHAEditor Win64 Development ('-Project='+$project+'/APS_ALPHA.uproject') -NoHotReloadFromIDE -NoXGE -MaxParallelActions=2 -WaitMutex ('-Log='+$backup+'/aps-build.log')
    if($LASTEXITCODE -ne 0) { throw ('APS rebuild failed: '+$LASTEXITCODE) }
    $entries|ForEach-Object {[pscustomobject]@{Target=$_.Target;Hash=(Get-FileHash -LiteralPath $_.Target).Hash}}|ConvertTo-Json|Out-File ($backup+'/installed-hashes.json') -Encoding utf8
    'Matched plugin/APS build installed; depth remains process opt-in, no material assets changed. Runtime validation still required.'
} catch {
    Restore-Payload
    throw
}
