#include "WorldActor.h"
#include "HAL/IConsoleManager.h"

namespace APSWorldActorTick
{
	// Rio 15.10 (code audit, flight FPS: every star, planet, moon, orbit, system and station ran an empty Tick every frame;
	// their Blueprints' Event Tick nodes are disabled): no C++ class under AWorldActor overrides Tick, so one whose Blueprint
	// has no Event Tick stops ticking at BeginPlay (its components keep their own ticks, latent actions still run).
	TAutoConsoleVariable<int32> CVarSkipEmptyTick(TEXT("aps.Actors.SkipEmptyWorldTick"), 1,
		TEXT("Rio 15.10: 1 = a world actor (star, planet, moon, orbit, system, station) without a Blueprint Event Tick does not ")
		TEXT("tick; 0 = every one ticks its empty Tick each frame, as 0.6.5.4 (read at BeginPlay)."),
		ECVF_Default);
}

// Sets default values
AWorldActor::AWorldActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AWorldActor::BeginPlay()
{
	Super::BeginPlay();
	if (APSWorldActorTick::CVarSkipEmptyTick.GetValueOnGameThread() != 0
		&& !GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(AActor, ReceiveTick)))
	{
		SetActorTickEnabled(false);
	}
}

// Called every frame
void AWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWorldActor::CalculateAffectionRadius()
{
	FVector Origin, BoxExtent;
	double SphereRadius;
	this->GetActorBounds(false, Origin, BoxExtent);
	SphereRadius = BoxExtent.GetMax();
	this->AffectionRadiusKM = SphereRadius / 100000.0;

	UE_LOG(LogTemp, VeryVerbose, TEXT("[APS.WorldActor] actor=%s affectionRadiusKm=%.6f"),
		*GetName(), AffectionRadiusKM);
}

/*FText AWorldActor::GetInGameName_Implementation() const
{
	return IItemInfoInterface::GetInGameName_Implementation();
}

UTexture2D* AWorldActor::GetAvatarPicture_Implementation() const
{
	return IItemInfoInterface::GetAvatarPicture_Implementation();
}*/


FText AWorldActor::GetInGameName_Implementation() const
{
	return InGameName;
}

UTexture2D* AWorldActor::GetAvatarPicture_Implementation() const
{
	return AvatarPicture;
}
