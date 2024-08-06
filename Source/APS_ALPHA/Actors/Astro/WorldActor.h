#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/BaseActor.h"
#include "APS_ALPHA/Core/Interfaces/AstroLocation.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "GameFramework/Actor.h"
#include "WorldActor.generated.h"

UCLASS()
class APS_ALPHA_API AWorldActor : public ABaseActor, public IAstroLocation, public IItemInfoInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, Category = "World Actor")
	double AffectionRadiusKM{0};

	UPROPERTY(EditAnywhere, Category = "World Actor")
	double RadiusKM{0};

	void CalculateAffectionRadius();

	virtual FText GetInGameName_Implementation() const override;
	
	virtual UTexture2D* GetAvatarPicture_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	FText InGameName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Info")
	UTexture2D* AvatarPicture;
};
