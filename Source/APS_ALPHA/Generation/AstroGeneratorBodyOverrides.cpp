#include "AstroGenerator.h"

#include "PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"

namespace
{
	// Keep translation-unit constants uniquely named: Unreal unity builds concatenate
	// several .cpp files, so a generic anonymous-namespace name can unexpectedly
	// trigger C4459 in an otherwise unrelated source file.
	constexpr double BodyOverrideEarthRadiusKm = 6371.0;

	FString MakePlanetKey(const int32 StarIndex, const int32 PlanetIndex)
	{
		return FString::Printf(TEXT("SYS0/S%d/P%d"), StarIndex, PlanetIndex);
	}

	FString MakeMoonKey(const int32 StarIndex, const int32 PlanetIndex, const int32 MoonIndex)
	{
		return FString::Printf(TEXT("SYS0/S%d/P%d/M%d"), StarIndex, PlanetIndex, MoonIndex);
	}

	FAPSPreviewBodyEditOverride CaptureEditorBuffer(const UGeneratedWorld& WorldModel)
	{
		FAPSPreviewBodyEditOverride Result;
		Result.PlanetType = WorldModel.PlanetType;
		Result.RadiusKm = FMath::Clamp(WorldModel.PlanetRadius, 100.0, 20000.0);
		Result.SurfaceSeed = FMath::Clamp(WorldModel.PlanetSurfaceSeed, 0, 999983);
		Result.SurfaceFeatureScale = FMath::Clamp(WorldModel.SurfaceFeatureScale, 0.25, 4.0);
		Result.SurfaceReliefScale = FMath::Clamp(WorldModel.SurfaceReliefScale, 0.25, 2.5);
		Result.SurfaceLandCoverageScale = FMath::Clamp(
			WorldModel.SurfaceLandCoverageScale, 0.25, 2.0);
		Result.SurfaceMountainScale = FMath::Clamp(WorldModel.SurfaceMountainScale, 0.0, 2.0);
		Result.SurfaceCraterScale = FMath::Clamp(WorldModel.SurfaceCraterScale, 0.0, 2.0);
		Result.SurfaceRoughnessScale = FMath::Clamp(WorldModel.SurfaceRoughnessScale, 0.25, 2.0);
		Result.AtmosphereHeight = FMath::Clamp(WorldModel.AtmosphereHeight, 0.0, 2000.0);
		Result.AtmosphereOpacity = FMath::Clamp(WorldModel.AtmosphereOpacity, 0.0, 40.0);
		Result.AtmosphereMultiScattering = FMath::Clamp(
			WorldModel.AtmosphereMultiScattering, 0.0, 10.0);
		Result.AtmosphereRayleighScattering = FMath::Clamp(
			WorldModel.AtmosphereRayleighScattering, 0.0, 64.0);
		Result.AtmosphereColor = WorldModel.AtmosphereColor;
		return Result;
	}

	void LoadEditorBuffer(
		UGeneratedWorld& WorldModel, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		WorldModel.PlanetType = BodyOverride.PlanetType;
		WorldModel.PlanetRadius = BodyOverride.RadiusKm;
		WorldModel.PlanetSurfaceSeed = BodyOverride.SurfaceSeed;
		WorldModel.SurfaceFeatureScale = BodyOverride.SurfaceFeatureScale;
		WorldModel.SurfaceReliefScale = BodyOverride.SurfaceReliefScale;
		WorldModel.SurfaceLandCoverageScale = BodyOverride.SurfaceLandCoverageScale;
		WorldModel.SurfaceMountainScale = BodyOverride.SurfaceMountainScale;
		WorldModel.SurfaceCraterScale = BodyOverride.SurfaceCraterScale;
		WorldModel.SurfaceRoughnessScale = BodyOverride.SurfaceRoughnessScale;
		WorldModel.AtmosphereHeight = BodyOverride.AtmosphereHeight;
		WorldModel.AtmosphereOpacity = BodyOverride.AtmosphereOpacity;
		WorldModel.AtmosphereMultiScattering = BodyOverride.AtmosphereMultiScattering;
		WorldModel.AtmosphereRayleighScattering = BodyOverride.AtmosphereRayleighScattering;
		WorldModel.AtmosphereColor = BodyOverride.AtmosphereColor;
	}

	void ApplyToPlanetModelData(
		FPlanetModel& Model, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		Model.PlanetType = BodyOverride.PlanetType;
		Model.Radius = static_cast<float>(
			BodyOverride.RadiusKm / BodyOverrideEarthRadiusKm);
		Model.RadiusKM = static_cast<float>(BodyOverride.RadiusKm);
		Model.SurfaceSeed = BodyOverride.SurfaceSeed;
		Model.SurfaceFeatureScale = BodyOverride.SurfaceFeatureScale;
		Model.SurfaceReliefScale = BodyOverride.SurfaceReliefScale;
		Model.SurfaceLandCoverageScale = BodyOverride.SurfaceLandCoverageScale;
		Model.SurfaceMountainScale = BodyOverride.SurfaceMountainScale;
		Model.SurfaceCraterScale = BodyOverride.SurfaceCraterScale;
		Model.SurfaceRoughnessScale = BodyOverride.SurfaceRoughnessScale;
		Model.AtmosphereHeight = BodyOverride.AtmosphereHeight;
	}

	void ApplyToMoonModelData(
		FMoonModel& Model, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		Model.PlanetType = BodyOverride.PlanetType;
		Model.Radius = static_cast<float>(
			BodyOverride.RadiusKm / BodyOverrideEarthRadiusKm);
		Model.RadiusKM = static_cast<float>(BodyOverride.RadiusKm);
		Model.SurfaceSeed = BodyOverride.SurfaceSeed;
		Model.SurfaceFeatureScale = BodyOverride.SurfaceFeatureScale;
		Model.SurfaceReliefScale = BodyOverride.SurfaceReliefScale;
		Model.SurfaceLandCoverageScale = BodyOverride.SurfaceLandCoverageScale;
		Model.SurfaceMountainScale = BodyOverride.SurfaceMountainScale;
		Model.SurfaceCraterScale = BodyOverride.SurfaceCraterScale;
		Model.SurfaceRoughnessScale = BodyOverride.SurfaceRoughnessScale;
		Model.MoonAtmosphereHeight = BodyOverride.AtmosphereHeight;
	}

	void ApplyToGenerationModel(
		APlanetaryBody& Body, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		if (APlanet* Planet = Cast<APlanet>(&Body))
		{
			Planet->SetPlanetType(BodyOverride.PlanetType);
			if (!Planet->PlanetData.PlanetModel.IsValid())
			{
				Planet->PlanetData.PlanetModel = MakeShared<FPlanetModel>();
			}

			FPlanetModel& Model = *Planet->PlanetData.PlanetModel;
			ApplyToPlanetModelData(Model, BodyOverride);
			Planet->PlanetData.PlanetModelData = Model;
			Planet->PlanetData.PlanetRadiusKM = FMath::RoundToInt(BodyOverride.RadiusKm);
		}
		else if (AMoon* Moon = Cast<AMoon>(&Body))
		{
			if (!Moon->GenerationModel.IsValid())
			{
				Moon->GenerationModel = MakeShared<FMoonModel>();
			}

			FMoonModel& Model = *Moon->GenerationModel;
			ApplyToMoonModelData(Model, BodyOverride);

			// Keep the serializable parent snapshots in sync with the shared model.
			if (IsValid(Moon->ParentPlanet)
				&& Moon->ParentPlanet->PlanetData.PlanetModel.IsValid())
			{
				FPlanetModel& ParentModel = *Moon->ParentPlanet->PlanetData.PlanetModel;
				const int32 MoonIndex = Moon->ParentPlanet->Moons.IndexOfByKey(Moon);
				if (ParentModel.MoonsList.IsValidIndex(MoonIndex)
					&& ParentModel.MoonsList[MoonIndex].IsValid())
				{
					ParentModel.MoonsList[MoonIndex]->MoonModel = Moon->GenerationModel;
					ParentModel.MoonsList[MoonIndex]->MoonModelData = Model;
				}
				if (ParentModel.MoonsListData.IsValidIndex(MoonIndex))
				{
					ParentModel.MoonsListData[MoonIndex].MoonModel = Moon->GenerationModel;
					ParentModel.MoonsListData[MoonIndex].MoonModelData = Model;
				}
				Moon->ParentPlanet->PlanetData.PlanetModelData = ParentModel;
			}
		}
	}

	void ApplyToBody(APlanetaryBody& Body, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		AAstroGenerator::ApplyPlanetaryBodyRadius(Body, BodyOverride.RadiusKm);
		Body.PlanetType = BodyOverride.PlanetType;
		Body.WorldScapeSeed = BodyOverride.SurfaceSeed;
		Body.SurfaceFeatureScale = BodyOverride.SurfaceFeatureScale;
		Body.SurfaceReliefScale = BodyOverride.SurfaceReliefScale;
		Body.SurfaceLandCoverageScale = BodyOverride.SurfaceLandCoverageScale;
		Body.SurfaceMountainScale = BodyOverride.SurfaceMountainScale;
		Body.SurfaceCraterScale = BodyOverride.SurfaceCraterScale;
		Body.SurfaceRoughnessScale = BodyOverride.SurfaceRoughnessScale;
		Body.AtmosphereHeight = BodyOverride.AtmosphereHeight;
		ApplyToGenerationModel(Body, BodyOverride);

		if (IsValid(Body.PlanetaryEnvironmentGenerator)
			&& IsValid(Body.PlanetaryEnvironmentGenerator->PlanetAtmosphere))
		{
			AAtmoScape* Atmosphere = Body.PlanetaryEnvironmentGenerator->PlanetAtmosphere;
			Atmosphere->bKeepRelativeScale = false;
			Atmosphere->PlanetRadius = FMath::Max(BodyOverride.RadiusKm - 1.0, 0.5);
			Atmosphere->AtmosphereHeight = BodyOverride.AtmosphereHeight;
			Atmosphere->AtmosphereOpacity = BodyOverride.AtmosphereOpacity;
			Atmosphere->MultiScatering = FMath::Max(0.01, BodyOverride.AtmosphereMultiScattering);
			Atmosphere->RayleighHeight = BodyOverride.AtmosphereRayleighScattering;
			Atmosphere->RayleighScattering = BodyOverride.AtmosphereColor;
			Atmosphere->UpdateScale();
		}
	}
}

void AAstroGenerator::ApplyPlanetaryBodyRadius(APlanetaryBody& Body, const double RadiusKm)
{
	const double NewRadiusKm = FMath::Clamp(RadiusKm, 100.0, 20000.0);
	const double PreviousRadiusKm = FMath::Max(
		Body.RadiusKM, static_cast<double>(Body.PlanetRadiusKM));
	const FVector CurrentScale = Body.GetActorScale3D();
	if (FMath::IsFinite(PreviousRadiusKm) && PreviousRadiusKm > UE_DOUBLE_SMALL_NUMBER
		&& !CurrentScale.ContainsNaN())
	{
		// Scale by the radius ratio instead of assigning an absolute value. Preview
		// hierarchies carry one parent normalization transform; an absolute scale here
		// would discard it and make the edited body explode on return to SYSTEM.
		const double ScaleRatio = FMath::Clamp(
			NewRadiusKm / PreviousRadiusKm, 1.0e-6, 1.0e6);
		Body.SetActorScale3D(CurrentScale * ScaleRatio);
	}

	Body.SetRadius(static_cast<float>(NewRadiusKm / BodyOverrideEarthRadiusKm));
	Body.RadiusKM = NewRadiusKm;
	Body.PlanetRadiusKM = FMath::RoundToInt(NewRadiusKm);
	Body.AffectionRadiusKM = NewRadiusKm;
}

int32 AAstroGenerator::ApplyPreviewBodyEditOverridesToModels(
	const UGeneratedWorld* InGeneratedWorld, const int32 StarIndex,
	FPlanetarySystemModel& PlanetarySystem)
{
	if (!IsValid(InGeneratedWorld) || StarIndex < 0
		|| InGeneratedWorld->GetPreviewBodyEditOverrideCount() == 0)
	{
		return 0;
	}

	int32 AppliedCount = 0;
	for (int32 PlanetIndex = 0; PlanetIndex < PlanetarySystem.PlanetsList.Num(); ++PlanetIndex)
	{
		TSharedPtr<FPlanetData>& PlanetData = PlanetarySystem.PlanetsList[PlanetIndex];
		if (!PlanetData.IsValid())
		{
			continue;
		}
		if (!PlanetData->PlanetModel.IsValid())
		{
			PlanetData->PlanetModel = MakeShared<FPlanetModel>(PlanetData->PlanetModelData);
		}
		FPlanetModel& PlanetModel = *PlanetData->PlanetModel;
		if (const FAPSPreviewBodyEditOverride* PlanetOverride =
			InGeneratedWorld->FindPreviewBodyEditOverride(
				MakePlanetKey(StarIndex, PlanetIndex)))
		{
			ApplyToPlanetModelData(PlanetModel, *PlanetOverride);
			PlanetData->PlanetRadiusKM = FMath::RoundToInt(PlanetOverride->RadiusKm);
			++AppliedCount;
		}

		for (int32 MoonIndex = 0; MoonIndex < PlanetModel.MoonsList.Num(); ++MoonIndex)
		{
			TSharedPtr<FMoonData>& MoonData = PlanetModel.MoonsList[MoonIndex];
			if (!MoonData.IsValid())
			{
				continue;
			}
			if (!MoonData->MoonModel.IsValid())
			{
				MoonData->MoonModel = MakeShared<FMoonModel>(MoonData->MoonModelData);
			}
			if (const FAPSPreviewBodyEditOverride* MoonOverride =
				InGeneratedWorld->FindPreviewBodyEditOverride(
					MakeMoonKey(StarIndex, PlanetIndex, MoonIndex)))
			{
				ApplyToMoonModelData(*MoonData->MoonModel, *MoonOverride);
				MoonData->MoonModelData = *MoonData->MoonModel;
				++AppliedCount;
			}
			if (PlanetModel.MoonsListData.IsValidIndex(MoonIndex))
			{
				PlanetModel.MoonsListData[MoonIndex] = *MoonData;
			}
		}
		PlanetData->PlanetModelData = PlanetModel;
	}
	return AppliedCount;
}

FString AAstroGenerator::GetPreviewBodyStableKey(const APlanetaryBody* Body) const
{
	if (!IsValid(Body))
	{
		return FString();
	}

	const AMoon* Moon = Cast<AMoon>(Body);
	const APlanet* Planet = Moon ? Moon->ParentPlanet : Cast<APlanet>(Body);
	if (!IsValid(Planet))
	{
		return FString();
	}

	const AStar* ParentStar = Planet->ParentStar;
	int32 StarIndex = INDEX_NONE;
	if (IsValid(GeneratedHomeStarSystem) && IsValid(ParentStar))
	{
		StarIndex = GeneratedHomeStarSystem->GetStars().IndexOfByKey(ParentStar);
	}
	if (StarIndex == INDEX_NONE && (ParentStar == HomeStar || Planet == HomePlanet))
	{
		StarIndex = 0;
	}

	int32 PlanetIndex = INDEX_NONE;
	if (IsValid(ParentStar) && IsValid(ParentStar->PlanetarySystem))
	{
		PlanetIndex = ParentStar->PlanetarySystem->PlanetsActorsList.IndexOfByKey(Planet);
	}
	if (PlanetIndex == INDEX_NONE && Planet == HomePlanet)
	{
		PlanetIndex = 0;
	}
	if (StarIndex == INDEX_NONE || PlanetIndex == INDEX_NONE)
	{
		return FString();
	}

	if (!Moon)
	{
		return MakePlanetKey(StarIndex, PlanetIndex);
	}

	const int32 MoonIndex = Planet->Moons.IndexOfByKey(Moon);
	return MoonIndex == INDEX_NONE ? FString() : MakeMoonKey(StarIndex, PlanetIndex, MoonIndex);
}

bool AAstroGenerator::SavePreviewBodyEditOverride(
	UGeneratedWorld* InGeneratedWorld, const APlanetaryBody* Body) const
{
	if (!IsValid(InGeneratedWorld) || !IsValid(Body))
	{
		return false;
	}

	const FString StableKey = GetPreviewBodyStableKey(Body);
	if (StableKey.IsEmpty())
	{
		return false;
	}

	InGeneratedWorld->SetPreviewBodyEditOverride(StableKey, CaptureEditorBuffer(*InGeneratedWorld));
	UE_LOG(LogTemp, Verbose,
		TEXT("[APS.PreviewBodyEdit] Saved %s seed=%d feature=%.3f relief=%.3f land=%.3f mountain=%.3f crater=%.3f roughness=%.3f"),
		*StableKey, InGeneratedWorld->PlanetSurfaceSeed, InGeneratedWorld->SurfaceFeatureScale,
		InGeneratedWorld->SurfaceReliefScale, InGeneratedWorld->SurfaceLandCoverageScale,
		InGeneratedWorld->SurfaceMountainScale, InGeneratedWorld->SurfaceCraterScale,
		InGeneratedWorld->SurfaceRoughnessScale);
	return true;
}

bool AAstroGenerator::SaveSelectedPreviewBodyEditOverride(UGeneratedWorld* InGeneratedWorld) const
{
	APlanetaryBody* Body = Cast<APlanetaryBody>(SelectedPreviewBodyActor.Get());
	if (!IsValid(Body))
	{
		Body = ActivePreviewWorldScapeBody.Get();
	}
	if (!IsValid(Body))
	{
		Body = HomePlanet;
	}
	return SavePreviewBodyEditOverride(InGeneratedWorld, Body);
}

bool AAstroGenerator::LoadPreviewBodyEditOverride(
	UGeneratedWorld* InGeneratedWorld, const APlanetaryBody* Body) const
{
	if (!IsValid(InGeneratedWorld) || !IsValid(Body))
	{
		return false;
	}

	const FString StableKey = GetPreviewBodyStableKey(Body);
	const FAPSPreviewBodyEditOverride* BodyOverride =
		InGeneratedWorld->FindPreviewBodyEditOverride(StableKey);
	if (!BodyOverride)
	{
		return false;
	}

	LoadEditorBuffer(*InGeneratedWorld, *BodyOverride);
	return true;
}

bool AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(
	const UGeneratedWorld* InGeneratedWorld, const FString& StableBodyKey, APlanetaryBody* Body)
{
	if (!IsValid(InGeneratedWorld) || !IsValid(Body))
	{
		return false;
	}

	const FAPSPreviewBodyEditOverride* BodyOverride =
		InGeneratedWorld->FindPreviewBodyEditOverride(StableBodyKey);
	if (!BodyOverride)
	{
		return false;
	}

	ApplyToBody(*Body, *BodyOverride);
	return true;
}

int32 AAstroGenerator::ApplyPreviewBodyEditOverrides(UGeneratedWorld* InGeneratedWorld)
{
	if (!IsValid(InGeneratedWorld) || InGeneratedWorld->GetPreviewBodyEditOverrideCount() == 0)
	{
		return 0;
	}

	int32 AppliedCount = 0;
	TSet<APlanetaryBody*> VisitedBodies;
	const auto TryApply = [&](APlanetaryBody* Body, const FString& StableKey)
	{
		if (!IsValid(Body) || VisitedBodies.Contains(Body))
		{
			return;
		}
		VisitedBodies.Add(Body);
		if (ApplyPreviewBodyEditOverrideByKey(InGeneratedWorld, StableKey, Body))
		{
			++AppliedCount;
		}
	};

	if (IsValid(GeneratedHomeStarSystem))
	{
		const TArray<AStar*>& Stars = GeneratedHomeStarSystem->GetStars();
		for (int32 StarIndex = 0; StarIndex < Stars.Num(); ++StarIndex)
		{
			const AStar* Star = Stars[StarIndex];
			if (!IsValid(Star) || !IsValid(Star->PlanetarySystem))
			{
				continue;
			}

			const TArray<APlanet*>& Planets = Star->PlanetarySystem->PlanetsActorsList;
			for (int32 PlanetIndex = 0; PlanetIndex < Planets.Num(); ++PlanetIndex)
			{
				APlanet* Planet = Planets[PlanetIndex];
				TryApply(Planet, MakePlanetKey(StarIndex, PlanetIndex));
				if (!IsValid(Planet))
				{
					continue;
				}
				for (int32 MoonIndex = 0; MoonIndex < Planet->Moons.Num(); ++MoonIndex)
				{
					TryApply(Planet->Moons[MoonIndex],
						MakeMoonKey(StarIndex, PlanetIndex, MoonIndex));
				}
			}
		}
	}

	// Single-body generation levels do not always materialize a StarSystem actor.
	// They still use the same canonical home-body path so edits remain stable when
	// the user moves between PLANET and SYSTEM scopes.
	TryApply(HomePlanet, MakePlanetKey(0, 0));
	if (IsValid(HomePlanet))
	{
		for (int32 MoonIndex = 0; MoonIndex < HomePlanet->Moons.Num(); ++MoonIndex)
		{
			TryApply(HomePlanet->Moons[MoonIndex], MakeMoonKey(0, 0, MoonIndex));
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[APS.PreviewBodyEdit] Reapplied %d/%d retained body overrides after preview rebuild"),
		AppliedCount, InGeneratedWorld->GetPreviewBodyEditOverrideCount());
	return AppliedCount;
}
