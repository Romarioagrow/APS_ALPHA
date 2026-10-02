param(
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$Stage
)
# Read-only preflight by default. -Stage writes ONLY a new evidence bundle.
# There is deliberately no install mode while Rio's resource reservation is active.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$seedInstalled='C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4'
$seedCandidate='F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild/Plugins/WorldScape'
$seedOutput='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/native-scatter-seed-'+$Label
$seedBuildLog='F:/ChatGPT/APOSFERA/work/build-worldscape-scatter-seed-v2.log'
$seedWorker='Source/WorldScapeCore/Private/WorldScapeRoot_Foliages.cpp'
$seedHeader='Source/WorldScapeCore/Public/APSBoundedFoliageSeed.h'
$seedFiles=@($seedWorker,$seedHeader,
    'Binaries/Win64/UnrealEditor-WorldScapeCore.dll',
    'Binaries/Win64/UnrealEditor-WorldScapeCore.pdb',
    'Intermediate/Build/Win64/x64/UnrealEditor/Development/WorldScapeCore/UnrealEditor-WorldScapeCore.lib')
function Seed-Text([string]$Path) {
    return [IO.File]::ReadAllText($Path).Replace("`r`n","`n")
}
function Seed-Hash([string]$Path) {
    if(!(Test-Path -LiteralPath $Path -PathType Leaf)){return $null}
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}
$seedBusy=@(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -match '^(cl|link|ShaderCompileWorker)\.exe$' -or
    ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool')
})
if($seedBusy.Count){throw 'Compiler active; cannot freeze a coherent candidate. No process touched.'}
if(Test-Path -LiteralPath $seedOutput){throw 'Evidence already exists; use a fresh label.'}
if(!(Select-String -LiteralPath $seedBuildLog -SimpleMatch 'Total execution time: 25.57 seconds' -Quiet)){
    throw 'Expected private build evidence missing; inspect the actual build before proceeding.'
}
$seedOldModules=Get-Content -Raw -LiteralPath ($seedInstalled+'/Binaries/Win64/UnrealEditor.modules') | ConvertFrom-Json
$seedNewModules=Get-Content -Raw -LiteralPath ($seedCandidate+'/Binaries/Win64/UnrealEditor.modules') | ConvertFrom-Json
if($seedOldModules.BuildId -ne '33043543' -or $seedNewModules.BuildId -ne $seedOldModules.BuildId){
    throw 'Engine BuildId mismatch.'
}
if(($seedOldModules.Modules | ConvertTo-Json -Compress) -cne ($seedNewModules.Modules | ConvertTo-Json -Compress)){
    throw 'Unrelated module metadata changed.'
}
if((Seed-Hash ($seedCandidate+'/'+$seedHeader)) -ne (Seed-Hash ($PSScriptRoot+'/APSBoundedFoliageSeed.h'))){
    throw 'Compiled helper differs from the CPU regression helper.'
}
$seedCpp=Seed-Text ($seedCandidate+'/'+$seedWorker)
$seedInclude=@'
#include "APSBoundedFoliageSeed.h"
#include "HAL/IConsoleManager.h"
namespace {
TAutoConsoleVariable<int32> APSFoliageSeedRevision(TEXT("worldscape.APS.FoliageSeedRevision"),
    2, TEXT("Bounded planet-local scatter seed, owned V2 roots only."), ECVF_ReadOnly);
}
'@
$seedBranch=@'
	if (RootRef && RootRef->ActorHasTag(FName(TEXT("APS.SurfaceScatter.V2"))))
		seed = int32(APSBoundedFoliageSeed::ForSector(Sector.Position.X, Sector.Position.Y, Sector.Position.Z,
			Sector.Size, RootRef->Seed, FoliageCollectionID, FoliageID, FLID));
'@
$seedInclude=$seedInclude.Replace("`r`n","`n").TrimEnd()+"`n"
$seedBranch=$seedBranch.Replace("`r`n","`n").TrimEnd()+"`n"
if([regex]::Matches($seedCpp,[regex]::Escape($seedInclude)).Count -ne 1 -or
   [regex]::Matches($seedCpp,[regex]::Escape($seedBranch)).Count -ne 2){throw 'Unexpected seed delta.'}
$seedBeforeCpp=$seedCpp.Replace($seedInclude,'').Replace($seedBranch,'')
# apply_patch can add a final newline; only EOF newlines are immaterial here.
if($seedBeforeCpp.TrimEnd([char[]]"`n") -cne (Seed-Text ($seedInstalled+'/'+$seedWorker)).TrimEnd([char[]]"`n")){
    throw 'Worker has unrelated changes or installed baseline drifted. Do not overwrite.'
}
if(Test-Path -LiteralPath ($seedInstalled+'/'+$seedHeader)){throw 'Helper already exists in installed plugin; re-audit baseline.'}
# Compare BOTH source inventories: candidate additions and installed-only files matter.
$seedOldSources=@(Get-ChildItem -LiteralPath ($seedInstalled+'/Source') -File -Recurse |
    ForEach-Object {$_.FullName.Substring($seedInstalled.Length+1).Replace('\','/')})
$seedNewSources=@(Get-ChildItem -LiteralPath ($seedCandidate+'/Source') -File -Recurse |
    ForEach-Object {$_.FullName.Substring($seedCandidate.Length+1).Replace('\','/')})
foreach($seedRel in @($seedOldSources+$seedNewSources | Sort-Object -Unique)){
    if($seedRel -eq $seedHeader -or $seedRel -eq $seedWorker){continue}
    if($seedRel -notin $seedOldSources -or $seedRel -notin $seedNewSources -or
        (Seed-Text ($seedInstalled+'/'+$seedRel)) -cne (Seed-Text ($seedCandidate+'/'+$seedRel))){
        throw ('Unrelated plugin source differs: '+$seedRel)
    }
}
$seedDll=Get-Item -LiteralPath ($seedCandidate+'/'+$seedFiles[2])
foreach($seedRel in $seedNewSources){
    if((Get-Item -LiteralPath ($seedCandidate+'/'+$seedRel)).LastWriteTimeUtc -gt $seedDll.LastWriteTimeUtc){
        throw ('Source newer than candidate DLL: '+$seedRel)
    }
}
$seedEntries=@(foreach($seedRel in $seedFiles){
    $seedAfter=Seed-Hash ($seedCandidate+'/'+$seedRel)
    if(!$seedAfter){throw ('Missing candidate artifact: '+$seedRel)}
    $seedBefore=Seed-Hash ($seedInstalled+'/'+$seedRel)
    if(!$seedBefore -and $seedRel -ne $seedHeader){throw ('Missing installed artifact: '+$seedRel)}
    [pscustomobject]@{Relative=$seedRel;Before=$seedBefore;After=$seedAfter}
})
$seedBaseline=@(foreach($seedRel in $seedOldSources+@('Binaries/Win64/UnrealEditor.modules')){
    [pscustomobject]@{Relative=$seedRel;Hash=(Seed-Hash ($seedInstalled+'/'+$seedRel))}
})
if($Stage){
    New-Item -ItemType Directory -Path $seedOutput | Out-Null
    foreach($seedEntry in $seedEntries){
        foreach($seedSide in @('before','after')){
            if($seedSide -eq 'before' -and !$seedEntry.Before){continue}
            $seedFrom=if($seedSide -eq 'before'){$seedInstalled}else{$seedCandidate}
            $seedDest=Join-Path ($seedOutput+'/'+$seedSide) $seedEntry.Relative
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $seedDest) | Out-Null
            Copy-Item -LiteralPath ($seedFrom+'/'+$seedEntry.Relative) -Destination $seedDest
            $seedExpected=if($seedSide -eq 'before'){$seedEntry.Before}else{$seedEntry.After}
            if((Seed-Hash $seedDest) -ne $seedExpected){throw 'Snapshot drifted during copy; do not use incomplete bundle.'}
        }
    }
    # A later installer must revalidate this entire baseline and all target hashes,
    # require a released editor/resource window, and keep feature opt-in until renders.
    [pscustomobject]@{
        Version=1;State='STAGED_NOT_INSTALLED';CreatedUtc=[DateTime]::UtcNow.ToString('o');
        InstalledRoot=$seedInstalled;CandidateRoot=$seedCandidate;BuildId=$seedOldModules.BuildId;
        BuildLog=$seedBuildLog;BuildLogHash=(Seed-Hash $seedBuildLog);
        Entries=$seedEntries;SourceBaseline=$seedBaseline;
        NewFileRollback=$seedHeader;
        RuntimeVerified=$false;AssetsBaked=$false;
        Requirement='Rio releases ComfyUI window; revalidate all hashes before install, then runtime/rendered tests.'
    } | ConvertTo-Json -Depth 6 | Out-File -LiteralPath ($seedOutput+'/manifest.json') -Encoding utf8
}
[pscustomobject]@{Staged=[bool]$Stage;Installed=$false;Files=$seedEntries.Count;
    UnrelatedSourcesVerified=$seedOldSources.Count-1;Output=$(if($Stage){$seedOutput}else{$null});
    Scope='Core worker, new helper, Core DLL/PDB/import lib only; no project/asset/UHT/global switches'}
