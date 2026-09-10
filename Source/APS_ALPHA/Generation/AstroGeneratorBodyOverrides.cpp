#include "AstroGenerator.h"

#include "PlanetarySurfaceGenerator.h"
#include "PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
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

	FString MakePlanetKey(const int32 StarIndex, const int32 PlanetIndex, const FString& SystemPrefix = TEXT("SYS0"))
	{
		return FString::Printf(TEXT("%s/S%d/P%d"), *SystemPrefix, StarIndex, PlanetIndex);
	}

	FString MakeMoonKey(const int32 StarIndex, const int32 PlanetIndex, const int32 MoonIndex,
		const FString& SystemPrefix = TEXT("SYS0"))
	{
		return FString::Printf(TEXT("%s/S%d/P%d/M%d"), *SystemPrefix, StarIndex, PlanetIndex, MoonIndex);
	}

	FAPSPreviewBodyEditOverride CaptureEditorBuffer(
		const UGeneratedWorld& WorldModel, const FString& StableBodyKey)
	{
		FAPSPreviewBodyEditOverride Result;
		Result.PlanetType = WorldModel.PlanetType;
		Result.PlanetHabitability = WorldModel.PlanetHabitability;
		// Retained physical data is not an authoring slider. Merely selecting a
		// generated gas giant must not shrink it to a terrestrial editor limit.
		Result.RadiusKm = FMath::IsFinite(WorldModel.PlanetRadius) && WorldModel.PlanetRadius > 0.0
			? WorldModel.PlanetRadius : BodyOverrideEarthRadiusKm;
		Result.MoonOrbitRadiusKm = FMath::Clamp(
			WorldModel.MoonOrbitRadiusKm, 0.0, 100000000.0);
		Result.SurfaceSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(
			WorldModel.PlanetSurfaceSeed, WorldModel.GenerationSeed, StableBodyKey);
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
		WorldModel.PlanetHabitability = BodyOverride.PlanetHabitability;
		WorldModel.PlanetRadius = BodyOverride.RadiusKm;
		WorldModel.MoonOrbitRadiusKm = BodyOverride.MoonOrbitRadiusKm;
		if (BodyOverride.MoonCount != INDEX_NONE)
		{
			WorldModel.MoonsAmount = FMath::Clamp(BodyOverride.MoonCount, 0, 10);
		}
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
		Model.PlanetHabitability = BodyOverride.PlanetHabitability;
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

	bool ApplyToPlanetModelDataPreservingMoonCenters(
		FPlanetModel& Model, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		const double PreviousParentRadiusKm = Model.RadiusKM > 0.0f
			? static_cast<double>(Model.RadiusKM)
			: static_cast<double>(Model.Radius) * BodyOverrideEarthRadiusKm;
		const bool bParentRadiusChanged = !FMath::IsNearlyEqual(
			PreviousParentRadiusKm, BodyOverride.RadiusKm,
			FMath::Max(PreviousParentRadiusKm * 1.0e-9, 1.0e-6));
		if (!bParentRadiusChanged)
		{
			// Surface/atmosphere-only edits must not rewrite moon orbit models. Apart
			// from being unnecessary, even a numerically equivalent rewrite used to
			// trigger the actor layout path and visibly move a moon on selection.
			ApplyToPlanetModelData(Model, BodyOverride);
			return false;
		}

		TArray<double> PreviousOrbitRadii;
		PreviousOrbitRadii.Reserve(Model.MoonsList.Num());
		for (const TSharedPtr<FMoonData>& MoonData : Model.MoonsList)
		{
			PreviousOrbitRadii.Add(MoonData.IsValid() ? MoonData->OrbitRadius : -1.0);
		}
		TArray<double> MoonCenterRadiiKm;
		MoonCenterRadiiKm.Init(-1.0, Model.MoonsList.Num());
		if (FMath::IsFinite(PreviousParentRadiusKm)
			&& PreviousParentRadiusKm > UE_DOUBLE_SMALL_NUMBER)
		{
			for (int32 MoonIndex = 0; MoonIndex < Model.MoonsList.Num(); ++MoonIndex)
			{
				const TSharedPtr<FMoonData>& MoonData = Model.MoonsList[MoonIndex];
				if (MoonData.IsValid() && FMath::IsFinite(MoonData->OrbitRadius))
				{
					MoonCenterRadiiKm[MoonIndex] = PreviousParentRadiusKm
						* FMath::Max(1.0 + MoonData->OrbitRadius, 1.0);
				}
			}
		}

		ApplyToPlanetModelData(Model, BodyOverride);
		const double NewParentRadiusKm = FMath::Max(
			static_cast<double>(Model.RadiusKM),
			static_cast<double>(Model.Radius) * BodyOverrideEarthRadiusKm);
		if (!FMath::IsFinite(NewParentRadiusKm)
			|| NewParentRadiusKm <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}

		bool bRestoredAnyCenter = false;
		for (int32 MoonIndex = 0; MoonIndex < Model.MoonsList.Num(); ++MoonIndex)
		{
			TSharedPtr<FMoonData>& MoonData = Model.MoonsList[MoonIndex];
			if (!MoonData.IsValid() || !MoonCenterRadiiKm.IsValidIndex(MoonIndex)
				|| MoonCenterRadiiKm[MoonIndex] <= 0.0)
			{
				continue;
			}

			MoonData->OrbitRadius = FMath::Max(
				MoonCenterRadiiKm[MoonIndex] / NewParentRadiusKm - 1.0, 0.0);
			if (MoonData->MoonModel.IsValid())
			{
				MoonData->MoonModel->OrbitDistance = MoonData->OrbitRadius;
				MoonData->MoonModelData = *MoonData->MoonModel;
			}
			bRestoredAnyCenter = true;
		}
		if (bRestoredAnyCenter)
		{
			// Preserve authored centre distances whenever they are still safe. If a
			// larger parent would intersect a moon, move only that orbit outward.
			UPlanetarySystemGenerator::EnforceSafeMoonOrbitSpacing(Model);
		}

		for (int32 MoonIndex = 0; MoonIndex < Model.MoonsList.Num(); ++MoonIndex)
		{
			const TSharedPtr<FMoonData>& MoonData = Model.MoonsList[MoonIndex];
			if (MoonData.IsValid() && PreviousOrbitRadii.IsValidIndex(MoonIndex)
				&& !FMath::IsNearlyEqual(MoonData->OrbitRadius,
					PreviousOrbitRadii[MoonIndex], 1.0e-12))
			{
				return true;
			}
		}
		return false;
	}

	void ApplyToMoonModelData(
		FMoonModel& Model, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		Model.PlanetType = BodyOverride.PlanetType;
		Model.PlanetHabitability = BodyOverride.PlanetHabitability;
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

	bool ApplyMoonOrbitToParentModel(
		FPlanetModel& ParentModel, const int32 MoonIndex,
		const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		if (!ParentModel.MoonsList.IsValidIndex(MoonIndex)
			|| !ParentModel.MoonsList[MoonIndex].IsValid())
		{
			return false;
		}

		const double ParentRadiusKm = FMath::Max(
			static_cast<double>(ParentModel.RadiusKM),
			static_cast<double>(ParentModel.Radius) * BodyOverrideEarthRadiusKm);
		if (!FMath::IsFinite(ParentRadiusKm) || ParentRadiusKm <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}

		TArray<double> PreviousOrbitRadii;
		PreviousOrbitRadii.Reserve(ParentModel.MoonsList.Num());
		for (const TSharedPtr<FMoonData>& ExistingMoonData : ParentModel.MoonsList)
		{
			PreviousOrbitRadii.Add(
				ExistingMoonData.IsValid() ? ExistingMoonData->OrbitRadius : -1.0);
		}

		TSharedPtr<FMoonData>& MoonData = ParentModel.MoonsList[MoonIndex];
		if (BodyOverride.MoonOrbitRadiusKm > 0.0)
		{
			const double RequestedAltitudeInParentRadii = FMath::Max(
				BodyOverride.MoonOrbitRadiusKm / ParentRadiusKm - 1.0, 0.0);
			MoonData->OrbitRadius = RequestedAltitudeInParentRadii;
			if (MoonData->MoonModel.IsValid())
			{
				MoonData->MoonModel->OrbitDistance = RequestedAltitudeInParentRadii;
				MoonData->MoonModelData = *MoonData->MoonModel;
			}
		}
		UPlanetarySystemGenerator::EnforceSafeMoonOrbitSpacing(ParentModel);
		for (int32 ExistingMoonIndex = 0;
			ExistingMoonIndex < ParentModel.MoonsList.Num(); ++ExistingMoonIndex)
		{
			const TSharedPtr<FMoonData>& ExistingMoonData =
				ParentModel.MoonsList[ExistingMoonIndex];
			if (ExistingMoonData.IsValid()
				&& PreviousOrbitRadii.IsValidIndex(ExistingMoonIndex)
				&& !FMath::IsNearlyEqual(ExistingMoonData->OrbitRadius,
					PreviousOrbitRadii[ExistingMoonIndex], 1.0e-12))
			{
				return true;
			}
		}
		return false;
	}

	void ApplyMoonOrbitLayoutToActors(APlanet& ParentPlanet)
	{
		if (!ParentPlanet.PlanetData.PlanetModel.IsValid())
		{
			return;
		}

		const FPlanetModel& ParentModel = *ParentPlanet.PlanetData.PlanetModel;
		const double ParentRadiusKm = FMath::Max(
			static_cast<double>(ParentModel.RadiusKM), ParentPlanet.RadiusKM);
		for (int32 MoonIndex = 0; MoonIndex < ParentPlanet.Moons.Num(); ++MoonIndex)
		{
			AMoon* Moon = ParentPlanet.Moons[MoonIndex];
			APlanetOrbit* MoonOrbit = ParentPlanet.MoonOrbitsList.IsValidIndex(MoonIndex)
				? ParentPlanet.MoonOrbitsList[MoonIndex] : nullptr;
			if (!IsValid(Moon) || !IsValid(MoonOrbit)
				|| !ParentModel.MoonsList.IsValidIndex(MoonIndex)
				|| !ParentModel.MoonsList[MoonIndex].IsValid())
			{
				continue;
			}

			const double CenterRadiusCm = ParentRadiusKm
				* (1.0 + ParentModel.MoonsList[MoonIndex]->OrbitRadius) * 100000.0;
			const FTransform OrbitTransform = MoonOrbit->GetActorTransform();
			FVector LocalOrbitDirection = FVector::VectorPlaneProject(
				OrbitTransform.InverseTransformPosition(Moon->GetActorLocation()),
				FVector::UpVector).GetSafeNormal();
			if (LocalOrbitDirection.IsNearlyZero())
			{
				LocalOrbitDirection = FVector::YAxisVector;
			}
			// CenterRadiusCm is authored in the orbit actor's physical local space.
			// TransformPosition composes the preview root's uniform normalization; adding
			// the raw centimetres in world space made a surface-only edit fling moons away.
			const FVector TargetWorldLocation = OrbitTransform.TransformPosition(
				LocalOrbitDirection * CenterRadiusCm);
			if (!Moon->GetActorLocation().Equals(TargetWorldLocation, 0.001))
			{
				Moon->SetActorLocation(TargetWorldLocation,
					false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	bool ApplyToGenerationModel(
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
			const bool bMoonLayoutChanged =
				ApplyToPlanetModelDataPreservingMoonCenters(Model, BodyOverride);
			Planet->PlanetData.PlanetModelData = Model;
			Planet->PlanetData.PlanetHabitability = Model.PlanetHabitability;
			Planet->PlanetData.PlanetRadiusKM = FMath::RoundToInt(BodyOverride.RadiusKm);
			return bMoonLayoutChanged;
		}
		else if (AMoon* Moon = Cast<AMoon>(&Body))
		{
			if (!Moon->GenerationModel.IsValid())
			{
				Moon->GenerationModel = MakeShared<FMoonModel>();
			}

			FMoonModel& Model = *Moon->GenerationModel;
			ApplyToMoonModelData(Model, BodyOverride);
			Moon->PlanetData.PlanetHabitability = Model.PlanetHabitability;

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
				const bool bMoonLayoutChanged = ApplyMoonOrbitToParentModel(
					ParentModel, MoonIndex, BodyOverride);
				// The sanitizer can move this moon and every moon outside it. Rebuild
				// the complete serial snapshot after it runs; writing the pre-sanitize
				// local Model here restored a stale OrbitDistance on the edited entry.
				ParentModel.MoonsListData = ParentModel.GetMoonsData();
				Moon->ParentPlanet->PlanetData.PlanetModelData = ParentModel;
				return bMoonLayoutChanged;
			}
		}
		return false;
	}

	void ApplyToBody(APlanetaryBody& Body, const FAPSPreviewBodyEditOverride& BodyOverride)
	{
		AAstroGenerator::ApplyPlanetaryBodyRadius(Body, BodyOverride.RadiusKm);
		Body.PlanetType = BodyOverride.PlanetType;
		Body.PlanetHabitability = BodyOverride.PlanetHabitability;
		Body.WorldScapeSeed = BodyOverride.SurfaceSeed;
		Body.SurfaceFeatureScale = BodyOverride.SurfaceFeatureScale;
		Body.SurfaceReliefScale = BodyOverride.SurfaceReliefScale;
		Body.SurfaceLandCoverageScale = BodyOverride.SurfaceLandCoverageScale;
		Body.SurfaceMountainScale = BodyOverride.SurfaceMountainScale;
		Body.SurfaceCraterScale = BodyOverride.SurfaceCraterScale;
		Body.SurfaceRoughnessScale = BodyOverride.SurfaceRoughnessScale;
		Body.AtmosphereHeight = BodyOverride.AtmosphereHeight;
		const bool bMoonLayoutChanged = ApplyToGenerationModel(Body, BodyOverride);
		if (bMoonLayoutChanged)
		{
			if (APlanet* Planet = Cast<APlanet>(&Body); IsValid(Planet))
			{
				ApplyMoonOrbitLayoutToActors(*Planet);
			}
			else if (AMoon* Moon = Cast<AMoon>(&Body);
				IsValid(Moon) && IsValid(Moon->ParentPlanet))
			{
				ApplyMoonOrbitLayoutToActors(*Moon->ParentPlanet);
			}
		}

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
	if (!FMath::IsFinite(RadiusKm) || RadiusKm <= 0.0) return;
	const double NewRadiusKm = RadiusKm;
	const double PreviousRadiusKm = FMath::IsFinite(Body.RadiusKM) && Body.RadiusKM > 0.0
		? Body.RadiusKM : static_cast<double>(Body.PlanetRadiusKM);
	const FVector CurrentScale = Body.GetActorScale3D();
	const bool bRadiusChanged = !FMath::IsNearlyEqual(
		NewRadiusKm, PreviousRadiusKm,
		FMath::Max(PreviousRadiusKm * 1.0e-9, 1.0e-6));

	// Moon-orbit roots are attached to their planet so orbital rotation remains
	// hierarchical. They are not part of the planet's physical radius, however:
	// changing the planet scale must not multiply a moon's size or orbital radius.
	// Preserve the world transforms of both the orbit roots and their bodies while
	// the parent scale changes, then let Unreal recompute their relative transforms.
	TArray<TPair<TWeakObjectPtr<APlanetOrbit>, FTransform>> MoonOrbitTransforms;
	TArray<TPair<TWeakObjectPtr<AMoon>, FTransform>> MoonTransforms;
	if (bRadiusChanged)
	{
		if (APlanet* Planet = Cast<APlanet>(&Body))
		{
			for (APlanetOrbit* MoonOrbit : Planet->MoonOrbitsList)
			{
				if (IsValid(MoonOrbit))
				{
					MoonOrbitTransforms.Emplace(MoonOrbit, MoonOrbit->GetActorTransform());
				}
			}
			for (AMoon* Moon : Planet->Moons)
			{
				if (IsValid(Moon))
				{
					MoonTransforms.Emplace(Moon, Moon->GetActorTransform());
				}
			}
		}
	}
	if (bRadiusChanged && FMath::IsFinite(PreviousRadiusKm)
		&& PreviousRadiusKm > UE_DOUBLE_SMALL_NUMBER
		&& !CurrentScale.ContainsNaN())
	{
		// Scale by the radius ratio instead of assigning an absolute value. Preview
		// hierarchies carry one parent normalization transform; an absolute scale here
		// would discard it and make the edited body explode on return to SYSTEM.
		const double ScaleRatio = FMath::Clamp(
			NewRadiusKm / PreviousRadiusKm, 1.0e-6, 1.0e6);
		Body.SetActorScale3D(CurrentScale * ScaleRatio);

		for (const TPair<TWeakObjectPtr<APlanetOrbit>, FTransform>& Snapshot
			: MoonOrbitTransforms)
		{
			if (APlanetOrbit* MoonOrbit = Snapshot.Key.Get())
			{
				MoonOrbit->SetActorTransform(
					Snapshot.Value, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		for (const TPair<TWeakObjectPtr<AMoon>, FTransform>& Snapshot : MoonTransforms)
		{
			if (AMoon* Moon = Snapshot.Key.Get())
			{
				Moon->SetActorTransform(
					Snapshot.Value, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	Body.SetRadius(static_cast<float>(NewRadiusKm / BodyOverrideEarthRadiusKm));
	Body.RadiusKM = NewRadiusKm;
	Body.PlanetRadiusKM = FMath::RoundToInt(NewRadiusKm);
	Body.AffectionRadiusKM = NewRadiusKm;
}

int32 AAstroGenerator::ApplyPreviewBodyEditOverridesToModels(
	const UGeneratedWorld* InGeneratedWorld, const int32 StarIndex,
	FPlanetarySystemModel& PlanetarySystem, const FString& SystemPrefix)
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
				MakePlanetKey(StarIndex, PlanetIndex, SystemPrefix)))
		{
			FAPSPreviewBodyEditOverride ResolvedOverride = *PlanetOverride;
			ResolvedOverride.SurfaceSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(
				ResolvedOverride.SurfaceSeed, InGeneratedWorld->GenerationSeed,
				MakePlanetKey(StarIndex, PlanetIndex, SystemPrefix));
			ApplyToPlanetModelDataPreservingMoonCenters(PlanetModel, ResolvedOverride);
			PlanetData->PlanetRadiusKM = FMath::RoundToInt(ResolvedOverride.RadiusKm);
			PlanetData->PlanetHabitability = PlanetModel.PlanetHabitability;
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
					MakeMoonKey(StarIndex, PlanetIndex, MoonIndex, SystemPrefix)))
			{
				FAPSPreviewBodyEditOverride ResolvedOverride = *MoonOverride;
				ResolvedOverride.SurfaceSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(
					ResolvedOverride.SurfaceSeed, InGeneratedWorld->GenerationSeed,
					MakeMoonKey(StarIndex, PlanetIndex, MoonIndex, SystemPrefix));
				ApplyToMoonModelData(*MoonData->MoonModel, ResolvedOverride);
				MoonData->MoonModelData = *MoonData->MoonModel;
				ApplyMoonOrbitToParentModel(PlanetModel, MoonIndex, ResolvedOverride);
				++AppliedCount;
			}
			if (PlanetModel.MoonsListData.IsValidIndex(MoonIndex))
			{
				PlanetModel.MoonsListData[MoonIndex] = *MoonData;
			}
		}
		PlanetModel.MoonsListData = PlanetModel.GetMoonsData();
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
	const AStarSystem* OwningSystem = IsValid(ParentStar) ? Cast<AStarSystem>(ParentStar->GetAttachParentActor()) : nullptr;
	const FString SystemPrefix = IsValid(OwningSystem) && OwningSystem != GeneratedHomeStarSystem
		? TEXT("SYS-") + OwningSystem->StableSystemId.ToString(EGuidFormats::Digits) : TEXT("SYS0");
	int32 StarIndex = INDEX_NONE;
	if (IsValid(OwningSystem) && IsValid(ParentStar))
	{
		StarIndex = OwningSystem->GetStars().IndexOfByKey(ParentStar);
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
		return FString::Printf(TEXT("%s/S%d/P%d"), *SystemPrefix, StarIndex, PlanetIndex);
	}

	const int32 MoonIndex = Planet->Moons.IndexOfByKey(Moon);
	return MoonIndex == INDEX_NONE ? FString()
		: FString::Printf(TEXT("%s/S%d/P%d/M%d"), *SystemPrefix, StarIndex, PlanetIndex, MoonIndex);
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

	FAPSPreviewBodyEditOverride BodyOverride = CaptureEditorBuffer(
		*InGeneratedWorld, StableKey);
	BodyOverride.MoonCount = Cast<APlanet>(Body)
		? FMath::Clamp(InGeneratedWorld->MoonsAmount, 0, 10) : INDEX_NONE;
	InGeneratedWorld->SetPreviewBodyEditOverride(StableKey, BodyOverride);
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

	FAPSPreviewBodyEditOverride ResolvedOverride = *BodyOverride;
	ResolvedOverride.SurfaceSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(
		ResolvedOverride.SurfaceSeed, InGeneratedWorld->GenerationSeed, StableBodyKey);
	ApplyToBody(*Body, ResolvedOverride);
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
