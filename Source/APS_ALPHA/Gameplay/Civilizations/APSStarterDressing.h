#pragma once

#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

/**
 * The colony modules' look for the starter base and pad (B8, Rio 01.10): engine shapes in hull, metal and dark
 * colours, amber markings and warm windows that glow, and a few lights. Decoration only: no collision, no navigation.
 */
namespace APSStarterDressing
{
	enum class EShape : uint8
	{
		Cube,
		Cylinder,
		Sphere
	};

	inline const FLinearColor Hull(0.72f, 0.74f, 0.76f);
	inline const FLinearColor Metal(0.22f, 0.23f, 0.25f);
	inline const FLinearColor Deck(0.12f, 0.13f, 0.14f);
	inline const FLinearColor Dark(0.03f, 0.035f, 0.04f);
	inline const FLinearColor Solar(0.015f, 0.03f, 0.10f);
	inline const FLinearColor Amber(0.90f, 0.45f, 0.05f);
	inline const FLinearColor Marking(0.80f, 0.80f, 0.78f);
	inline const FLinearColor Warm(1.0f, 0.85f, 0.65f);
	inline const FLinearColor Red(1.0f, 0.05f, 0.03f);
	inline const FLinearColor Green(0.10f, 1.0f, 0.20f);

	inline UMaterialInterface* Material(AActor* Owner, const FLinearColor& Color, const float Glow)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, Glow > 0.0f
			? TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial")
			: TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Parent)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Parent, Owner);
		Instance->SetVectorParameterValue(TEXT("Color"), Glow > 0.0f ? Color * Glow : Color);
		return Instance;
	}

	inline void Paint(AActor* Owner, UStaticMeshComponent* Component, const FLinearColor& Color)
	{
		if (Component)
		{
			if (UMaterialInterface* Instance = Material(Owner, Color, 0.0f))
			{
				Component->SetMaterial(0, Instance);
			}
		}
	}

	/** A shape centred at Center, Size in centimetres (engine shapes are a metre and centred). */
	inline void Part(AActor* Owner, USceneComponent* Root, const EShape Shape, const FVector& Center, const FVector& Size,
		const FLinearColor& Color, const float Glow = 0.0f, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Shape == EShape::Cylinder ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
			: Shape == EShape::Sphere ? TEXT("/Engine/BasicShapes/Sphere.Sphere") : TEXT("/Engine/BasicShapes/Cube.Cube"),
			nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh || !Root)
		{
			return;
		}
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner);
		Component->SetupAttachment(Root);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeTransform(FTransform(Rotation, Center, Size / 100.0));
		Component->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(Glow <= 0.0f);
		Component->SetMaterial(0, Material(Owner, Color, Glow));
		Component->RegisterComponent();
		Owner->AddInstanceComponent(Component);
	}

	inline void Lamp(AActor* Owner, USceneComponent* Root, const FVector& Location, const FLinearColor& Color, const float Candelas,
		const float RadiusCm)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
		Light->SetupAttachment(Root);
		Light->SetRelativeLocation(Location);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Candelas);
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(RadiusCm);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		Owner->AddInstanceComponent(Light);
	}
}
