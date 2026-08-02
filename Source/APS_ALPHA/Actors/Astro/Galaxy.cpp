#include "Galaxy.h"

AGalaxy::AGalaxy()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);
	StarMeshInstances->NumCustomDataFloats = 6;
	StarMeshInstances->bAutoRebuildTreeOnInstanceChanges = false;
	StarMeshInstances->bUseTranslatedInstanceSpace = true;
	StarMeshInstances->bEnableDensityScaling = false;
	StarMeshInstances->bNeverDistanceCull = true;
	StarMeshInstances->SetCullDistances(0, 0);
	StarMeshInstances->bDisableCollision = true;
	StarMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarMeshInstances->SetGenerateOverlapEvents(false);
	StarMeshInstances->SetCanEverAffectNavigation(false);
	StarMeshInstances->bEvaluateWorldPositionOffset = false;
	StarMeshInstances->bWorldPositionOffsetWritesVelocity = false;
	StarMeshInstances->SetCastShadow(false);
	StarMeshInstances->bAffectDynamicIndirectLighting = false;
	StarMeshInstances->bAffectDistanceFieldLighting = false;
	StarMeshInstances->SetReceivesDecals(false);
}
