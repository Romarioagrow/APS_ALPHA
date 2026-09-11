#pragma once

#include "APSStellarMaterialContract.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/GameModes/MainMenuGameModeBase.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace APSGameplayStarAppearance
{
inline float GetLuminosityGain(const double LuminositySolar)
{
	if (!FMath::IsFinite(LuminositySolar) || LuminositySolar <= 0.0)
	{
		return 0.0f;
	}
	// Compressed luminosity ranking, not a colour-class multiplier: a red giant
	// keeps its luminosity while cool, small dwarfs lose their artificial HDR floor.
	return static_cast<float>(FMath::Clamp(
		FMath::Pow(FMath::Max(LuminositySolar, 1.0e-6), 0.12), 0.5, 1.2));
}

inline bool IsPreviewHierarchy(const AActor* Actor)
{
	for (const AActor* Parent = Actor; IsValid(Parent); Parent = Parent->GetAttachParentActor())
	{
		if (Parent->ActorHasTag(TEXT("WorldGenerationPreview"))) return true;
		const AAstroGenerator* Generator = Cast<AAstroGenerator>(Parent);
		if (Generator && Generator->UsesContinuousPreviewFrame()) return true;
	}
	return false;
}

inline uint64 GetBuildSerial(const AActor* Actor)
{
	for (const AActor* Parent = Actor; IsValid(Parent); Parent = Parent->GetAttachParentActor())
	{
		if (const AAstroGenerator* Generator = Cast<AAstroGenerator>(Parent))
		{
			return Generator->GetCanonicalStellarProjectionDescriptor().ProxyBuildSerial;
		}
	}
	return 0;
}

struct FConfiguredComponent
{
	TWeakObjectPtr<UMaterialInstanceDynamic> Material;
	int32 InstanceCount = INDEX_NONE;
	int32 CustomDataStride = 0;
	uint64 BuildSerial = 0;
};

inline void Apply(UWorld* World)
{
	if (!IsValid(World)
		|| (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
		|| World->GetAuthGameMode<AMainMenuGameModeBase>()) return;

	// The caller also searches every tick while no key star exists. Keep this
	// appearance discovery bounded to twice a second even in that fallback case.
	static TMap<TWeakObjectPtr<UWorld>, double> NextWorldScans;
	static TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, FConfiguredComponent> Configured;
	for (auto It = NextWorldScans.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) It.RemoveCurrent();
	}
	const double Now = FPlatformTime::Seconds();
	double& NextScan = NextWorldScans.FindOrAdd(World);
	if (Now < NextScan) return;
	NextScan = Now + 0.5;
	for (auto It = Configured.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) It.RemoveCurrent();
	}

	const FMaterialParameterInfo ProfileParameter(TEXT("GameplayPointProfile"));
	const auto Configure = [&](UHierarchicalInstancedStaticMeshComponent* Source,
		const auto& ResolveGain)
	{
		if (!IsValid(Source)) return;
		const int32 Count = Source->GetInstanceCount();
		const int32 OldStride = Source->NumCustomDataFloats;
		if (Count <= 0 || OldStride < 6
			|| static_cast<int64>(Count) * OldStride != Source->PerInstanceSMCustomData.Num()) return;

		UMaterialInterface* Current = Source->GetMaterial(0);
		if (!APSStellarMaterialContract::HasExactBase(Current, APSStellarMaterialContract::HismBaseObjectPath)) return;
		UMaterial* Base = APSStellarMaterialContract::GetBaseMaterial(Current);
		float Profile = 0.0f;
		// Do not attach an ineffective MID before the opt-in shader is available.
		if (!IsValid(Base) || !Base->GetScalarParameterValue(ProfileParameter, Profile)) return;

		FConfiguredComponent& State = Configured.FindOrAdd(Source);
		const uint64 BuildSerial = GetBuildSerial(Source->GetOwner());
		Current->GetScalarParameterValue(ProfileParameter, Profile);
		if (State.Material.Get() == Current && Profile == 1.0f
			&& OldStride >= 7 && State.CustomDataStride == OldStride
			&& State.InstanceCount == Count && State.BuildSerial == BuildSerial) return;

		UMaterialInstanceDynamic* GameplayMaterial = State.Material.Get();
		if (!IsValid(GameplayMaterial) || GameplayMaterial != Current)
		{
			GameplayMaterial = UMaterialInstanceDynamic::Create(Current, Source);
			if (!IsValid(GameplayMaterial)) return;
			GameplayMaterial->SetFlags(RF_Transient);
		}

		const int32 NewStride = FMath::Max(OldStride, 7);
		const int64 NewDataCount = static_cast<int64>(Count) * NewStride;
		if (NewDataCount > MAX_int32) return;
		TArray<float> Repacked;
		Repacked.SetNumUninitialized(static_cast<int32>(NewDataCount));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			float* Row = Repacked.GetData() + Index * NewStride;
			// Preserve all six original channels byte-for-byte, including suppression,
			// seeds and selection. Future channels above our dedicated slot survive.
			FMemory::Memcpy(Row, Source->PerInstanceSMCustomData.GetData() + Index * OldStride,
				OldStride * sizeof(float));
			Row[6] = ResolveGain(Index);
		}
		// UE 5.4 clears the entire custom array when its stride changes. Repack
		// BEFORE calling the setter, then publish through the instance-data API.
		Source->SetNumCustomDataFloats(NewStride);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Source->SetCustomData(Index,
				TArrayView<const float>(Repacked.GetData() + Index * NewStride, NewStride), false);
		}
		GameplayMaterial->SetScalarParameterValue(ProfileParameter.Name, 1.0f);
		Source->SetMaterial(0, GameplayMaterial);
		Source->MarkRenderStateDirty();
		State.Material = GameplayMaterial;
		State.InstanceCount = Count;
		State.CustomDataStride = NewStride;
		State.BuildSerial = BuildSerial;
	};

	for (TActorIterator<AGalaxy> It(World); It; ++It)
	{
		AGalaxy* Galaxy = *It;
		if (!IsValid(Galaxy) || IsPreviewHierarchy(Galaxy)) continue;
		Configure(Galaxy->StarMeshInstances, [Galaxy](const int32 Index)
		{
			FGalaxyCatalogStarRecord Record;
			return Galaxy->GetRenderedCatalogRecord(Index, Record)
				? GetLuminosityGain(APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass))
				: 1.0f; // Legacy fields without records retain their accepted visibility.
		});
	}
	for (TActorIterator<AStarCluster> It(World); It; ++It)
	{
		AStarCluster* Cluster = *It;
		if (!IsValid(Cluster) || IsPreviewHierarchy(Cluster)) continue;
		Configure(Cluster->StarMeshInstances, [Cluster](const int32 Index)
		{
			const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Index);
			return Record ? GetLuminosityGain(Record->PrimaryStarModel.Luminosity) : 1.0f;
		});
	}
}
}
