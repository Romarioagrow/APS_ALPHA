#include "StarfieldScape.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMaterialLibrary.h"

AStarfieldScape::AStarfieldScape()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    Root->SetMobility(EComponentMobility::Static);

    StarfieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarfieldMesh"));
    StarfieldMesh->SetupAttachment(Root);

    StarfieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StarfieldMesh->bCastDynamicShadow = false;
    StarfieldMesh->bCastStaticShadow = false;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/AtmoScape/Mesh/SM_StarfieldMesh.SM_StarfieldMesh'"));
    if (MeshAsset.Succeeded())
    {
        StarfieldMesh->SetStaticMesh(MeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("MaterialInterface'/AtmoScape/Materials/Master/MM_StarfieldScape.MM_StarfieldScape'"));
    if (MaterialAsset.Succeeded())
    {
        Material = MaterialAsset.Object;
    }

    // Définir une échelle très grande pour le mesh
    StarfieldMesh->SetWorldScale3D(FVector(10000000000.0f, 10000000000.0f, 10000000000.0f)); // Ajustez cette valeur selon les besoins

    // Initialisation des valeurs par défaut des paramètres
    SunIntensity = 1.0f;
    SunAngularDiameter = 0.5357f;
    StarFieldIntensity = 1.3f;
    StarfieldDecrease = 1.0f;
    StarfieldTileScale = 1.5f;
}

void AStarfieldScape::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    StarfieldInstance = UMaterialInstanceDynamic::Create(Material, this);
    if (StarfieldInstance)
    {
        StarfieldInstance->SetScalarParameterValue(FName("SunIntensity"), SunIntensity);
        StarfieldInstance->SetScalarParameterValue(FName("SunAngularDiameter"), SunAngularDiameter);
        StarfieldInstance->SetScalarParameterValue(FName("StarFieldIntensity"), StarFieldIntensity);
        StarfieldInstance->SetScalarParameterValue(FName("StarfieldDecrease"), StarfieldDecrease);
        StarfieldInstance->SetScalarParameterValue(FName("StarfieldTileScale"), StarfieldTileScale);

        StarfieldMesh->SetMaterial(0, StarfieldInstance);
    }
}
