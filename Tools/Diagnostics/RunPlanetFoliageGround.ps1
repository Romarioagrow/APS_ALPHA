param(
    [ValidateSet('Rocky','Terrestrial','Greenhouse','Melted','Dwarf','Ocean','Water','Desert','Forest','Volcanic','Ice','Frozen','Ammonia','Metal','Carbon','SuperEarth','Lava','Metallic','Nordic','Tundra','HighMountain','Sand','Oasis','Archipelago','Pangea','Rogue','Exoplanet','Unknown','Basalt','Savanna','Sulfur','Crystal')][string]$Family='Terrestrial',
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$Label,
    [switch]$LeafFadeAB,
    [switch]$LeafDistanceAB,
    [switch]$SurfaceScatter,
    [switch]$Published,
    [switch]$WalkingCollisionContact,
    [switch]$WalkingCollisionDefault,
    [switch]$StructureExclusion,
    [switch]$StructureExclusionDefault,
    [switch]$Locomotion,
    [switch]$WalkTree,
    [switch]$HabitatAudit,
    [switch]$Habitable,
    [switch]$TerrestrialVegetation,
    [ValidateRange(0,2147483647)][int]$SurfaceSeed=424242,
    [ValidateRange(3,16)][int]$OasisTreeAttempts=3
)
$ErrorActionPreference='Stop'
if($TerrestrialVegetation -and ($Family -ne 'Terrestrial' -or !$SurfaceScatter -or $Published)){throw 'Terrestrial ecology candidate requires explicit unpublished Terrestrial scatter'}
if($PSBoundParameters.ContainsKey('OasisTreeAttempts') -and ($Family -ne 'Oasis' -or !$SurfaceScatter -or $Published)){throw 'Oasis density comparison requires explicit unpublished scatter trial'}
if($WalkingCollisionDefault -and !$WalkingCollisionContact){throw 'Query-only collision default requires natural contact mode'}
if($Locomotion -and (!$WalkingCollisionContact -or !$StructureExclusionDefault)){throw 'Locomotion requires published natural contact with default structure exclusion'}
if($WalkTree -and (!$Locomotion -or $Family -ne 'Forest')){throw 'Tree walking requires explicit Forest locomotion'}
if($WalkingCollisionContact -and !$Published){throw 'Natural contact proof requires the published foliage palette'}
if(($StructureExclusion -or $StructureExclusionDefault) -and !$WalkingCollisionContact){throw 'Exclusion trial uses natural three-frame contact mode'}
if($StructureExclusion -and $StructureExclusionDefault){throw 'Default verification cannot force the exclusion CVar'}
if($Published -and ($SurfaceScatter -or $LeafFadeAB -or $LeafDistanceAB)){throw 'Published proof must not force a scatter or leaf override'}
if(!$SurfaceScatter -and !$Published -and $Family -notin @('Frozen','Terrestrial','Forest')){throw 'Other types require the explicit five-mesh SurfaceScatter trial'}
if($SurfaceScatter -and ($LeafFadeAB -or $LeafDistanceAB)){throw 'SurfaceScatter is a normal-placement trial, not a leaf material override'}
if(($LeafFadeAB -or $LeafDistanceAB) -and $Family -ne 'Forest'){throw 'Leaf diagnostic needs Forest'}
if($LeafFadeAB -and $LeafDistanceAB){throw 'Choose one leaf isolation trial'}
$projectRoot='F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA'
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/'+$Family.ToLowerInvariant()+'-'+$Label
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl,link,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Editor/compiler active; no process touched'}
if(Test-Path -LiteralPath $runDir){throw 'Evidence exists; use a fresh label'}
$palette=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1/FC_APS_Proto_'+$Family+'.uasset'
if($SurfaceScatter -or $Published){$palette=$projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/SurfaceScatter20260930V2/FC_APS_Scatter_'+$Family+'.uasset'}
if(-not(Test-Path -LiteralPath $palette)){throw 'Prototype palette missing'}
if($SurfaceScatter -or $Published){
    foreach($shape in @('Pebble','RockA','RockB','SlabA','SlabB')){
        if(!(Test-Path -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ScatterMaterial20260930V1/MI_APS_Scatter_'+$shape+'.uasset'))){throw 'Bake the isolated scatter material first'}
    }
}
$dll=$projectRoot+'/Binaries/Win64/UnrealEditor-APS_ALPHA.dll'
foreach($file in @('Source/APS_ALPHA/Tests/APSPlanetFoliageRenderedProbe.h','Source/APS_ALPHA/Tests/APSFoliageWalkingRenderedProbe.h','Source/APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.cpp','Source/APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.h','Source/APS_ALPHA/Tests/APSGeneratedGameplayHandoffSmokeTests.cpp','Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h','Source/APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.cpp','Source/APS_ALPHA/Tests/APSPlanetSurfaceScatterTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$file)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Known newer diagnostic source; build first'}
}
& (Join-Path $PSScriptRoot 'AssertPlanetGpuHeadroom.ps1')
foreach($file in @('Source/APS_ALPHA/Generation/AstroGenerator.cpp','Source/APS_ALPHA/Core/Planetary/APSSurfaceLandingRelief.h','Source/APS_ALPHA/Tests/APSSurfaceLandingReliefTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$file)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Newer landing recovery source; build first'}
}
foreach($file in @('Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.cpp','Source/APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h','Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.cpp','Source/APS_ALPHA/Core/Planetary/APSTerrestrialVegetation.h','Source/APS_ALPHA/Tests/APSTerrestrialVegetationTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$file)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Newer terrestrial ecology source; build first'}
}
if((Get-Item -LiteralPath ($projectRoot+'/Source/APS_ALPHA/Tests/APSFoliageLocomotionProbe.h')).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Newer locomotion diagnostic; build first'}
foreach($file in @('Source/APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.cpp','Source/APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.h','Source/APS_ALPHA/Tests/APSFoliageExclusionTests.cpp')) {
    if((Get-Item -LiteralPath ($projectRoot+'/'+$file)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $dll).LastWriteTimeUtc){throw 'Newer exclusion source; build first'}
}
New-Item -ItemType Directory -Path $runDir | Out-Null
Get-ChildItem -LiteralPath ($projectRoot+'/Content/APS/APS_ALPHA/WSC/PlanetSurface/Shared') -Recurse -File -Filter '*.uasset' |
    Get-FileHash | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/assets-before.json') -Encoding utf8
Get-FileHash -LiteralPath $dll | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/dll.json') -Encoding utf8
Get-FileHash -LiteralPath $palette | Select-Object Path,Hash | ConvertTo-Json | Out-File ($runDir+'/palette.json') -Encoding utf8
$foliageCommands=if($SurfaceScatter){'aps.WorldScapeFoliage.Enable 1,aps.WorldScapeFoliage.Prototype 0,aps.WorldScapeFoliage.SurfaceScatter 1'}else{'aps.WorldScapeFoliage.Enable 1,aps.WorldScapeFoliage.Prototype 1,aps.WorldScapeFoliage.SurfaceScatter 0'}
if($Published){$foliageCommands=''}else{$foliageCommands+=','}
if($TerrestrialVegetation){$foliageCommands+='aps.WorldScapeFoliage.TerrestrialVegetation 1,'}
if($Published -and $Family -eq 'Oasis'){$foliageCommands+='aps.WorldScapeFoliage.OasisTreeAttempts,'}
if($Published -and $Family -eq 'Terrestrial'){$foliageCommands+='aps.WorldScapeFoliage.TerrestrialVegetation,'}
if($PSBoundParameters.ContainsKey('OasisTreeAttempts')){$foliageCommands+=('aps.WorldScapeFoliage.OasisTreeAttempts '+$OasisTreeAttempts+',')}
if($WalkingCollisionContact){$foliageCommands+=if($WalkingCollisionDefault){'aps.WorldScapeFoliage.WalkingCollision,'}else{'aps.WorldScapeFoliage.WalkingCollision 1,'}}
if($StructureExclusion){$foliageCommands+='aps.WorldScapeFoliage.StructureExclusion 1,'}
if($StructureExclusionDefault){$foliageCommands+='aps.WorldScapeFoliage.StructureExclusion,'}
$arguments=@(
    ('"'+$projectRoot+'/APS_ALPHA.uproject"'),'/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha',
    '-ddc=InstalledNoZenLocalFallback','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
    '-d3d12','-sm6','-RenderOffscreen','-Windowed','-ForceRes','-ResX=1600','-ResY=1000',
    ('-APSDiagnosticPlanet='+$Family),'-APSProbeFoliageGround','-APSProbeDefaultAtmosphere',
    ('-ExecCmds="'+$foliageCommands+'Automation RunTests APS.Gameplay.World.PlanetSurface.Foliage+APS.Gameplay.World.PlanetSurface.LandingReliefRecovery+APS.Gameplay.World.PlanetSurface.WaterSurface+APS.Rendered.Gameplay.GeneratedSurfaceLightingDiagnostics"'),
    '-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$runDir+'/report"'),
    ('-UserDir="'+$runDir+'"'),('-abslog="'+$runDir+'/gameplay.log"')
)
if($LeafFadeAB){$arguments += '-APSProbeLeafFadeAB'}
if($LeafDistanceAB){$arguments += '-APSProbeLeafDistanceAB'}
if($Published){$arguments += '-APSProbeFoliageGroundPublished'}
if($WalkingCollisionContact){$arguments += '-APSProbeFoliageWalkingContact'}
if($Locomotion){$arguments += '-APSProbeFoliageLocomotion'}
if($WalkTree){$arguments += '-APSProbeFoliageWalkTree'}
if($HabitatAudit){$arguments += '-APSProbeFoliageHabitatAudit'}
if($Habitable){$arguments += '-APSProbeFoliageHabitable'}
if($PSBoundParameters.ContainsKey('SurfaceSeed')){$arguments += ('-APSProbeFoliageSeed='+$SurfaceSeed)}
if($StructureExclusion -or $StructureExclusionDefault){$arguments += '-APSProbeFoliageStructureExclusion'}
$arguments -join ' ' | Out-File ($runDir+'/command.txt') -Encoding utf8
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput ($runDir+'/stdout.txt') -RedirectStandardError ($runDir+'/stderr.txt')
[PSCustomObject]@{Id=$process.Id;StartTime=$process.StartTime;Evidence=$runDir;Family=$Family;FoliagePrototype=(-not $Published);Published=[bool]$Published} | ConvertTo-Json
