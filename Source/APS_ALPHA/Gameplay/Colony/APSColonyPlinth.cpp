#include "APSColonyPlinth.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AAPSColonyPlinth::AAPSColonyPlinth()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(TEXT("APS.Colony.Plinth"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	SetRootComponent(Body);
}

void AAPSColonyPlinth::Configure(const bool bRound)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, bRound
		? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Cube.Cube"), nullptr,
		LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh)
	{
		return;
	}
	Body->SetStaticMesh(Mesh);
	if (UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UMaterialInstanceDynamic* Concrete = UMaterialInstanceDynamic::Create(Parent, this);
		Concrete->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.30f, 0.29f, 0.27f));
		Body->SetMaterial(0, Concrete);
	}
}
