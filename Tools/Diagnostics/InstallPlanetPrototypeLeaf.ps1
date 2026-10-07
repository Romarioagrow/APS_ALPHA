param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label)
$ErrorActionPreference='Stop'
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/leaf-prototype-install-'+$Label
$mesh=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1/SM_APS_Proto_TreeBudget.uasset'
$collection=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1/FC_APS_Proto_Forest.uasset'
$leaf=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliageLeaf20260930/MI_APS_PrototypeLeaf.uasset'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; nothing touched'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; refusing overwrite'}
if(!(Test-Path -LiteralPath $leaf)){throw 'Owned leaf material missing'}
$before=Get-FileHash -LiteralPath $mesh
if($before.Hash -ne '8B5C305BEF7218DBCAA03CFB486963C39D6EA4B70BE806C5CE5BA5BEEED469DB'){throw 'Owned tree changed; no replacement'}
$collectionBefore=Get-FileHash -LiteralPath $collection
if($collectionBefore.Hash -ne '1B9BFC94671683989FFB0946C2493BA0ECDADB5A4CD6F91587C0056273176DC3'){throw 'Forest collection changed; no replacement'}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
New-Item -ItemType Directory -Path $runDir | Out-Null
Copy-Item -LiteralPath $mesh -Destination ($runDir+'/tree-before.uasset')
if((Get-FileHash -LiteralPath ($runDir+'/tree-before.uasset')).Hash -ne $before.Hash){throw 'Backup hash mismatch; editor not started'}
$before | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/tree-before.json') -Encoding utf8
Copy-Item -LiteralPath $collection -Destination ($runDir+'/forest-before.uasset')
if((Get-FileHash -LiteralPath ($runDir+'/forest-before.uasset')).Hash -ne $collectionBefore.Hash){throw 'Forest backup mismatch; editor not started'}
$collectionBefore | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/forest-before.json') -Encoding utf8
$arguments=@(('"'+$projectRoot+'/APS_ALPHA.uproject"'),'-run=pythonscript',
    ('-script="'+$projectRoot+'/Tools/Diagnostics/InstallPlanetPrototypeLeaf.py"'),
    '-AllowCommandletRendering','-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen',('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/install.log"'))
$arguments -join ' ' | Out-File ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[pscustomobject]@{Id=$process.Id;Evidence=$runDir;Target=$collection;Backup=$runDir+'/forest-before.uasset';ProtectedMesh=$mesh} | ConvertTo-Json
# Returning a PID does not confirm installation or rendered normal-LOD acceptance.
