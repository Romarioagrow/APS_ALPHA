param([switch]$Rollback)
$ErrorActionPreference='Stop'
$project='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$installed='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$candidate='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild/Plugins/WorldScape'
$base='F:/ChatGPT/APOSFERA/work/planet_flight_residency_20260930'
$backup=$base+'/base-batch-install-v1'
$testDir=$base+'/base-batch-private-v1'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; no process touched'}
if($Rollback) {
    $entries=Get-Content -Raw -LiteralPath ($backup+'/manifest.json')|ConvertFrom-Json
    $after=@{}; Get-Content -Raw -LiteralPath ($backup+'/installed-hashes.json')|ConvertFrom-Json|ForEach-Object {$after[$_.Target]=$_.Hash}
    foreach($entry in $entries) {
        $target=[IO.Path]::GetFullPath($entry.Target)
        if(!$target.StartsWith([IO.Path]::GetFullPath($installed)+[IO.Path]::DirectorySeparatorChar) -and !$target.StartsWith([IO.Path]::GetFullPath($project)+[IO.Path]::DirectorySeparatorChar)){throw 'Unsafe rollback path'}
        if($target.StartsWith([IO.Path]::GetFullPath($project+'/Binaries/Win64')+[IO.Path]::DirectorySeparatorChar)){continue}
        if($entry.Before -and (Get-FileHash -LiteralPath $entry.Backup).Hash -ne $entry.Before){throw 'Backup modified'}
        if((Get-FileHash -LiteralPath $entry.Target).Hash -ne $after[$entry.Target]){throw ('Subsequent change: '+$entry.Target)}
    }
    foreach($entry in $entries){
        if([IO.Path]::GetFullPath($entry.Target).StartsWith([IO.Path]::GetFullPath($project+'/Binaries/Win64')+[IO.Path]::DirectorySeparatorChar)){continue}
        if($entry.Before){Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force}else{Remove-Item -LiteralPath $entry.Target}
    }
    # Never restore an old shared project DLL over a colleague's compiled work.
    & 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat' APS_ALPHAEditor Win64 Development ('-Project='+$project+'/APS_ALPHA.uproject') -NoHotReloadFromIDE -NoXGE -MaxParallelActions=2 -WaitMutex ('-Log='+$backup+'/rollback-build.log')
    if($LASTEXITCODE -ne 0){throw 'Native/flight source restored, current project rebuild failed; do not launch until resolved'}
    'Restored V5 plugin and flight source, rebuilt current shared project sources; backups retained.'
    exit 0
}
if(Test-Path -LiteralPath $backup){throw 'Transaction already exists'}
if((Get-FileHash -LiteralPath ($installed+'/Binaries/Win64/UnrealEditor-WorldScapeCore.dll')).Hash -ne '00E857570217EC4E5A4969F8DE78FC0DEDEAD95C55BC2D6C8F4876A81EA1A162'){throw 'Native baseline changed'}
if((Get-FileHash -LiteralPath ($project+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll')).Hash -ne 'C929E7D01A771AB0322D43D7D5BE25A2A08204AF5370902F2BE8C737C27649A1'){throw 'APS baseline changed'}
$report=Get-Content -Raw -LiteralPath ($testDir+'/report/index.json')|ConvertFrom-Json
if($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.succeeded -ne 9 -or $report.succeededWithWarnings -ne 0){throw 'Require nine clean private tests'}
$tested=Get-Content -Raw -LiteralPath ($testDir+'/native-dll.json')|ConvertFrom-Json
if($tested.Hash -ne (Get-FileHash -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor-WorldScapeCore.dll')).Hash){throw 'Candidate changed after tests'}
$oldModules=Get-Content -Raw -LiteralPath ($installed+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
$newModules=Get-Content -Raw -LiteralPath ($candidate+'/Binaries/Win64/UnrealEditor.modules')|ConvertFrom-Json
if($oldModules.BuildId -ne $newModules.BuildId -or $oldModules.BuildId -ne '33043543'){throw 'Engine BuildId mismatch'}
$source=@('Source/WorldScapeCore/Private/WorldScapeRoot_Main.cpp','Source/WorldScapeCore/Public/WorldScapeRoot.h','Source/WorldScapeCore/Private/Tests/APSWorldScapeFreshRootTests.cpp')
foreach($file in Get-ChildItem -LiteralPath ($candidate+'/Source') -File -Recurse) {
    $rel=$file.FullName.Substring($candidate.Length+1).Replace('\','/')
    if($source -contains $rel){continue}
    $other=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $other) -or [IO.File]::ReadAllText($file.FullName).Replace("`r`n","`n") -cne [IO.File]::ReadAllText($other).Replace("`r`n","`n")){throw ('Unrelated source: '+$rel)}
}
$relative=@($source)
foreach($module in $newModules.Modules.psobject.Properties.Name){
    $relative+=@(('Binaries/Win64/UnrealEditor-'+$module+'.dll'),('Binaries/Win64/UnrealEditor-'+$module+'.pdb'),('Intermediate/Build/Win64/x64/UnrealEditor/Development/'+$module+'/UnrealEditor-'+$module+'.lib'))
}
$relative+='Binaries/Win64/UnrealEditor.modules'
$relative+='Intermediate/Build/Win64/UnrealEditor/Inc/WorldScapeCore/UHT/WorldScapeRoot.generated.h'
$entries=@()
foreach($rel in $relative){
    $src=Join-Path $candidate $rel; $dst=Join-Path $installed $rel
    if(!(Test-Path -LiteralPath $src)){throw ('Missing '+$src)}
    $entries+=[pscustomobject]@{Source=$src;Target=$dst;Backup=($backup+'/plugin/'+$rel);Before=$(if(Test-Path -LiteralPath $dst){(Get-FileHash -LiteralPath $dst).Hash}else{$null})}
}
foreach($name in @('UnrealEditor-APS_ALPHA.dll','UnrealEditor-APS_ALPHA.pdb','UnrealEditor.modules')){
    $dst=$project+'/Binaries/Win64/'+$name
    $entries+=[pscustomobject]@{Source=$null;Target=$dst;Backup=($backup+'/project/'+$name);Before=(Get-FileHash -LiteralPath $dst).Hash}
}
$flightSource=$project+'/Source/APS_ALPHA/Core/World/APSPlanetEnvironmentFlightResidency.cpp'
$flightBefore=$base+'/base-batch-source-before/APSPlanetEnvironmentFlightResidency.cpp'
$entries+=[pscustomobject]@{Source=$null;Target=$flightSource;Backup=$flightBefore;Before=(Get-FileHash -LiteralPath $flightBefore).Hash}
foreach($entry in $entries){
    if($entry.Backup -eq $flightBefore){continue}
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $entry.Backup)|Out-Null
    if($entry.Before){Copy-Item -LiteralPath $entry.Target -Destination $entry.Backup}
}
$entries|ConvertTo-Json -Depth 4|Out-File ($backup+'/manifest.json') -Encoding utf8
foreach($entry in $entries){if($entry.Source){Copy-Item -LiteralPath $entry.Source -Destination $entry.Target -Force}}
& 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat' APS_ALPHAEditor Win64 Development ('-Project='+$project+'/APS_ALPHA.uproject') -NoHotReloadFromIDE -NoXGE -MaxParallelActions=2 -WaitMutex ('-Log='+$backup+'/aps-build.log')
$buildCode=$LASTEXITCODE
$entries|ForEach-Object {[pscustomobject]@{Target=$_.Target;Hash=$(if(Test-Path -LiteralPath $_.Target){(Get-FileHash -LiteralPath $_.Target).Hash}else{$null})}}|ConvertTo-Json|Out-File ($backup+'/installed-hashes.json') -Encoding utf8
if($buildCode -ne 0){throw ('APS rebuild failed; coherent native installation retained, rollback available: '+$buildCode)}
'Coherent native + APS installation complete. FlightResidency remains OFF until runtime acceptance.'
