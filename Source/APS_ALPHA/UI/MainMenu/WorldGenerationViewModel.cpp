#include "WorldGenerationViewModel.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "WorldGenerationViewModel"

namespace APSCivilizationPilot
{
	constexpr const TCHAR* ProductionClassPath =
		TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter.BP_CustomGravityCharacter_C");

	UClass* LoadProductionClass()
	{
		return LoadClass<ACustomGravityCharacter>(nullptr, ProductionClassPath);
	}
}

void UWorldGenerationViewModel::Initialize(UObject* InWorldContext, UGeneratedWorld* InGeneratedWorld)
{
	WorldContext = InWorldContext;
	if (InGeneratedWorld)
	{
		InGeneratedWorld->PlanetsAmount = FMath::Clamp(InGeneratedWorld->PlanetsAmount, 1, 20);
		InGeneratedWorld->MoonsAmount = FMath::Clamp(InGeneratedWorld->MoonsAmount, 0, 10);
		if (!FMath::IsFinite(InGeneratedWorld->PlanetRadius) || InGeneratedWorld->PlanetRadius <= 0.0)
			InGeneratedWorld->PlanetRadius = 6371.0;
		InGeneratedWorld->HomeStarRadiusOverrideSolar =
			FMath::IsFinite(InGeneratedWorld->HomeStarRadiusOverrideSolar)
			&& InGeneratedWorld->HomeStarRadiusOverrideSolar > 0.0
				? FMath::Clamp(InGeneratedWorld->HomeStarRadiusOverrideSolar, 1.0e-5, 1000.0)
				: 0.0;
		InGeneratedWorld->PlanetSurfaceSeed = FMath::Clamp(InGeneratedWorld->PlanetSurfaceSeed, 0, 999983);
		InGeneratedWorld->SurfaceFeatureScale = FMath::Clamp(InGeneratedWorld->SurfaceFeatureScale, 0.25, 4.0);
		InGeneratedWorld->SurfaceReliefScale = FMath::Clamp(InGeneratedWorld->SurfaceReliefScale, 0.25, 2.5);
		InGeneratedWorld->SurfaceLandCoverageScale = FMath::Clamp(InGeneratedWorld->SurfaceLandCoverageScale, 0.25, 2.0);
		InGeneratedWorld->SurfaceMountainScale = FMath::Clamp(InGeneratedWorld->SurfaceMountainScale, 0.0, 2.0);
		InGeneratedWorld->SurfaceCraterScale = FMath::Clamp(InGeneratedWorld->SurfaceCraterScale, 0.0, 2.0);
		InGeneratedWorld->SurfaceRoughnessScale = FMath::Clamp(InGeneratedWorld->SurfaceRoughnessScale, 0.25, 2.0);
		InGeneratedWorld->AtmosphereHeight = FMath::Clamp(InGeneratedWorld->AtmosphereHeight, 0.0, 2000.0);
		InGeneratedWorld->AtmosphereOpacity = FMath::Clamp(InGeneratedWorld->AtmosphereOpacity, 0.0, 40.0);
		InGeneratedWorld->AtmosphereMultiScattering = FMath::Clamp(
			InGeneratedWorld->AtmosphereMultiScattering, 0.0, 10.0);
		InGeneratedWorld->AtmosphereRayleighScattering = FMath::Clamp(
			InGeneratedWorld->AtmosphereRayleighScattering, 0.0, 64.0);
		InGeneratedWorld->StartPlanetIndex = FMath::Clamp(
			InGeneratedWorld->StartPlanetIndex, 1, InGeneratedWorld->PlanetsAmount);
	}
	UE_MVVM_SET_PROPERTY_VALUE(GeneratedWorld, InGeneratedWorld);
	if (UWorld* World = InWorldContext ? InWorldContext->GetWorld() : nullptr)
	{
		if (UMainGameplayInstance* GameplayInstance = World->GetGameInstance()
			? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr)
		{
			if (!GameplayInstance->SpawnParameters)
			{
				GameplayInstance->SpawnParameters = NewObject<USpawnParameters>(GameplayInstance);
			}
			// Only the production custom-gravity pawn currently satisfies the full
			// generated-world movement/camera contract. Keep experimental pawns out
			// of the committed civilization route until they are explicitly certified.
			if (UClass* ProductionPilot = APSCivilizationPilot::LoadProductionClass())
			{
				GameplayInstance->SpawnParameters->BP_CharacterClass = ProductionPilot;
			}
			UE_MVVM_SET_PROPERTY_VALUE(SpawnParameters, GameplayInstance->SpawnParameters);
		}
	}
	SetPreviewStatus(LOCTEXT("PreviewPending", "PREVIEW PENDING"), false);
}

void UWorldGenerationViewModel::Shutdown()
{
	if (UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(PreviewTimerHandle);
		World->GetTimerManager().ClearTimer(PlanetAppearanceTimerHandle);
		World->GetTimerManager().ClearTimer(PreviewTravelTimerHandle);
	}
	bPendingSurfaceAppearanceRefresh = false;
	WorldContext.Reset();
	PreviewGenerator.Reset();
}

void UWorldGenerationViewModel::SetEnumValue(const UEnum* EnumClass, int32 SelectedValue)
{
	if (!GeneratedWorld || !EnumClass)
	{
		return;
	}

	FString PropertyName = EnumClass->GetName();
	if ((EnumClass == StaticEnum<EStellarType>() || EnumClass == StaticEnum<ESpectralClass>())
		&& SetSelectedStarEnum(EnumClass, SelectedValue)) return;
	if ((EnumClass == StaticEnum<EStarType>() || EnumClass == StaticEnum<EPlanetarySystemType>()
		|| EnumClass == StaticEnum<EOrbitDistributionType>()) && SetSelectedSystemEnum(EnumClass, SelectedValue)) return;
	if (PropertyName.StartsWith(TEXT("E")))
	{
		PropertyName.RightChopInline(1);
	}

	FProperty* Property = FindFProperty<FProperty>(UGeneratedWorld::StaticClass(), *PropertyName);
	if (!Property)
	{
		UE_LOG(LogTemp, Warning, TEXT("No UGeneratedWorld property matches enum %s"), *EnumClass->GetName());
		return;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(GeneratedWorld);
	if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(SelectedValue));
	}
	else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, static_cast<int64>(SelectedValue));
	}
	else
	{
		return;
	}

	if (EnumClass == StaticEnum<EPlanetType>())
	{
		const EPlanetType PlanetType = static_cast<EPlanetType>(SelectedValue);
		const bool bWorldScape = UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(PlanetType);
		UE_LOG(LogTemp, Log,
			TEXT("[APS.PlanetSurface.UI] subtype=%s family=%s worldscape=%s seed=%d"),
			*UEnum::GetValueAsString(PlanetType),
			bWorldScape
				? *UEnum::GetValueAsString(
					UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(PlanetType))
				: TEXT("GasGiant"),
			bWorldScape ? TEXT("true") : TEXT("false"), GeneratedWorld->PlanetSurfaceSeed);
		// Planet type changes are appearance/profile edits. Reusing the selected
		// actor keeps its proportional camera distance and avoids rebuilding the
		// surrounding galaxy, cluster and every stellar hierarchy.
		RefreshPlanetAppearancePreview(true);
		// A type/preset click is a discrete transaction, not a continuously emitted
		// slider sample.  Commit its latest editor buffer before the next generator
		// tick so an already queued edit for this body cannot consume the inactive
		// globe buffer first and make the final type land back on the old buffer.
		// Repeated clicks in one frame still coalesce because the generator has not
		// sampled the shared body state yet.
		if (UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
			World && World->GetTimerManager().IsTimerActive(PlanetAppearanceTimerHandle))
		{
			World->GetTimerManager().ClearTimer(PlanetAppearanceTimerHandle);
			ExecutePlanetAppearancePreviewRefresh();
		}
		return;
	}
	if (EnumClass == StaticEnum<EPlanetHabitability>())
	{
		// Habitability is gameplay metadata. Persist it on the selected body without
		// rebuilding the astronomical hierarchy or changing its visual subtype.
		RefreshPlanetAppearancePreview(false);
		if (UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
			World && World->GetTimerManager().IsTimerActive(PlanetAppearanceTimerHandle))
		{
			World->GetTimerManager().ClearTimer(PlanetAppearanceTimerHandle);
			ExecutePlanetAppearancePreviewRefresh();
		}
		return;
	}
	RequestPreview();
}

void UWorldGenerationViewModel::SetGalaxySize(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	const int32 NewValue = FMath::RoundToInt(FMath::Clamp(Value, 1.0, 100000.0));
	if (GeneratedWorld->GalaxySize != NewValue)
	{
		GeneratedWorld->GalaxySize = NewValue;
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetHomeStarRadiusOverrideSolar(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	const double NewValue = Value <= 0.0
		? 0.0 : FMath::Clamp(Value, 1.0e-5, 1000.0);
	if (!FMath::IsNearlyEqual(
		GeneratedWorld->HomeStarRadiusOverrideSolar, NewValue, 1.0e-8))
	{
		GeneratedWorld->HomeStarRadiusOverrideSolar = NewValue;
		RequestPreview();
	}
}

bool UWorldGenerationViewModel::SetSelectedStarEnum(const UEnum* EnumClass, const int32 SelectedValue)
{
	FString Address;
	FStarModel Current;
	if (!GeneratedWorld || !PreviewGenerator.IsValid()
		|| !PreviewGenerator->GetPreviewStarEditContext(Address, Current)) return false;
	if (!EnumClass->IsValidEnumValue(SelectedValue) || SelectedValue == EnumClass->GetMaxEnumValue()) return true;
	const bool bStellar = EnumClass == StaticEnum<EStellarType>();
	if ((bStellar ? static_cast<int32>(Current.StellarType) : static_cast<int32>(Current.SpectralClass))
		== SelectedValue) return true;
	FAPSPreviewStarEditOverride Edit;
	if (const FAPSPreviewStarEditOverride* Existing = GeneratedWorld->FindPreviewStarEditOverride(Address)) Edit = *Existing;
	else Edit.AutomaticModel = Edit.Model = Current;
	TSharedPtr<FStarModel> NewModel = MakeShared<FStarModel>();
	NewModel->StellarType = bStellar ? static_cast<EStellarType>(SelectedValue) : Current.StellarType;
	NewModel->SpectralClass = bStellar ? Current.SpectralClass : static_cast<ESpectralClass>(SelectedValue);
	UStarGenerator* Stars = NewObject<UStarGenerator>(this);
	Stars->SetGenerationSeed(static_cast<int32>(HashCombine(
		GetTypeHash(GeneratedWorld->GenerationSeed), FCrc::StrCrc32(*Address)) & 0x7fffffffu));
	Stars->GenerateStarModel(NewModel);
	Edit.AutomaticModel = *NewModel;
	if (Edit.RadiusOverrideSolar > 0.0) Stars->ApplyRadiusOverrideSolar(*NewModel, Edit.RadiusOverrideSolar);
	Edit.Model = *NewModel;
	GeneratedWorld->SetPreviewStarEditOverride(Address, Edit);
	RequestPreview();
	return true;
}

void UWorldGenerationViewModel::SetSelectedStarRadiusOverrideSolar(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	FString Address;
	FStarModel Current;
	if (!PreviewGenerator.IsValid() || !PreviewGenerator->GetPreviewStarEditContext(Address, Current))
	{
		// No selected astronomical object exists before the initial preview.
		SetHomeStarRadiusOverrideSolar(Value);
		return;
	}
	FAPSPreviewStarEditOverride Edit;
	if (const FAPSPreviewStarEditOverride* Existing = GeneratedWorld->FindPreviewStarEditOverride(Address)) Edit = *Existing;
	else Edit.AutomaticModel = Edit.Model = Current;
	const double NewValue = Value <= 0.0 ? 0.0 : FMath::Clamp(Value, 1.0e-5, 1000.0);
	if (FMath::IsNearlyEqual(Edit.RadiusOverrideSolar, NewValue, 1.0e-8)) return;
	Edit.RadiusOverrideSolar = NewValue;
	Edit.Model = Edit.AutomaticModel;
	if (NewValue > 0.0)
	{
		UStarGenerator* Stars = NewObject<UStarGenerator>(this);
		if (!Stars->ApplyRadiusOverrideSolar(Edit.Model, NewValue)) return;
	}
	GeneratedWorld->SetPreviewStarEditOverride(Address, Edit);
	RequestPreview();
}

double UWorldGenerationViewModel::GetSelectedStarRadiusOverrideSolar() const
{
	FString Address;
	FStarModel Current;
	if (GeneratedWorld && PreviewGenerator.IsValid()
		&& PreviewGenerator->GetPreviewStarEditContext(Address, Current))
	{
		const FAPSPreviewStarEditOverride* Edit = GeneratedWorld->FindPreviewStarEditOverride(Address);
		return Edit ? Edit->RadiusOverrideSolar : 0.0;
	}
	return GeneratedWorld ? GeneratedWorld->HomeStarRadiusOverrideSolar : 0.0;
}

EStellarType UWorldGenerationViewModel::GetSelectedStellarType() const
{
	FString Address;
	FStarModel Current;
	if (PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewStarEditContext(Address, Current)) return Current.StellarType;
	return GeneratedWorld ? GeneratedWorld->StellarType : EStellarType::MainSequence;
}

ESpectralClass UWorldGenerationViewModel::GetSelectedSpectralClass() const
{
	FString Address;
	FStarModel Current;
	if (PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewStarEditContext(Address, Current)) return Current.SpectralClass;
	return GeneratedWorld ? GeneratedWorld->SpectralClass : ESpectralClass::G;
}

bool UWorldGenerationViewModel::SetSelectedSystemEnum(const UEnum* EnumClass, const int32 SelectedValue)
{
	FString Address;
	FStarSystemModel Current;
	if (!GeneratedWorld || !PreviewGenerator.IsValid()) return false;
	// A ready but unresolved legacy catalog selection is not permission to edit
	// home defaults. Its disabled controls must also be safe for direct callers.
	if (!PreviewGenerator->GetPreviewSystemEditContext(Address, Current)) return bPreviewReady;
	if (!EnumClass->IsValidEnumValue(SelectedValue) || SelectedValue == EnumClass->GetMaxEnumValue()) return true;
	FAPSPreviewSystemEditOverride Edit;
	if (const FAPSPreviewSystemEditOverride* Existing = GeneratedWorld->FindPreviewSystemEditOverride(Address)) Edit = *Existing;
	if (EnumClass == StaticEnum<EStarType>())
	{
		const EStarType Type = static_cast<EStarType>(SelectedValue);
		if (Current.StarSystemType == Type) return true;
		Edit.StarType = Type;
		FRandomStream CountStream(static_cast<int32>(HashCombine(GetTypeHash(Current.GenerationSeed), FCrc::StrCrc32(*Address))));
		Edit.StarCount = Type == EStarType::SingleStar ? 1 : Type == EStarType::DoubleStar ? 2
			: Type == EStarType::TripleStar ? 3 : CountStream.RandRange(4, 6);
		// Changing stellar multiplicity redistributes the SAME total, not a hidden
		// per-star multiplier. Independent stellar streams retain surviving stars.
		Edit.TotalPlanets = Current.PotentialPlanetCount;
		if (GetSelectedSystemPlanetaryType() == EPlanetarySystemType::SinglePlanetSystem
			&& Edit.TotalPlanets > Edit.StarCount)
		{
			Edit.bOverridePlanetaryType = true;
			Edit.PlanetaryType = EPlanetarySystemType::MultiPlanetSystem;
		}
	}
	else if (EnumClass == StaticEnum<EPlanetarySystemType>())
	{
		const EPlanetarySystemType Type = static_cast<EPlanetarySystemType>(SelectedValue);
		if (Type == EPlanetarySystemType::Unknown || GetSelectedSystemPlanetaryType() == Type) return true;
		Edit.bOverridePlanetaryType = true;
		Edit.PlanetaryType = Type;
		Edit.TotalPlanets = Type == EPlanetarySystemType::NoPlanetSystem ? 0
			: Type == EPlanetarySystemType::SinglePlanetSystem ? Current.AmountOfStars
			: FMath::Max(Current.PotentialPlanetCount, Current.AmountOfStars);
	}
	else
	{
		const EOrbitDistributionType Type = static_cast<EOrbitDistributionType>(SelectedValue);
		if (GetSelectedSystemOrbitDistribution() == Type) return true;
		Edit.bOverrideOrbitDistribution = true;
		Edit.OrbitDistribution = Type;
	}
	GeneratedWorld->SetPreviewSystemEditOverride(Address, Edit);
	if (Address == TEXT("SYS0")) GeneratedWorld->StartPlanetIndex = FMath::Clamp(
		GeneratedWorld->StartPlanetIndex, 1, FMath::Max(1, GetHomeStartPlanetCount()));
	RequestPreview();
	return true;
}

EStarType UWorldGenerationViewModel::GetSelectedSystemStarType() const
{
	FString Address;
	FStarSystemModel Current;
	if (PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewSystemEditContext(Address, Current)) return Current.StarSystemType;
	return GeneratedWorld ? GeneratedWorld->StarType : EStarType::SingleStar;
}

EPlanetarySystemType UWorldGenerationViewModel::GetSelectedSystemPlanetaryType() const
{
	FString Address;
	FStarSystemModel Current;
	if (GeneratedWorld && PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewSystemEditContext(Address, Current))
	{
		const FAPSPreviewSystemEditOverride* Edit = GeneratedWorld->FindPreviewSystemEditOverride(Address);
		if (Edit && Edit->bOverridePlanetaryType) return Edit->PlanetaryType;
		const AStarSystem* System = PreviewGenerator->GetContinuousPreviewActiveSystem();
		if (System && IsValid(System->MainStar) && IsValid(System->MainStar->PlanetarySystem))
			return System->MainStar->PlanetarySystem->PlanetarySystemType;
	}
	return GeneratedWorld ? GeneratedWorld->PlanetarySystemType : EPlanetarySystemType::MultiPlanetSystem;
}

EOrbitDistributionType UWorldGenerationViewModel::GetSelectedSystemOrbitDistribution() const
{
	FString Address;
	FStarSystemModel Current;
	if (GeneratedWorld && PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewSystemEditContext(Address, Current))
	{
		const FAPSPreviewSystemEditOverride* Edit = GeneratedWorld->FindPreviewSystemEditOverride(Address);
		if (Edit && Edit->bOverrideOrbitDistribution) return Edit->OrbitDistribution;
		const AStarSystem* System = PreviewGenerator->GetContinuousPreviewActiveSystem();
		if (System && IsValid(System->MainStar) && IsValid(System->MainStar->PlanetarySystem))
			return System->MainStar->PlanetarySystem->OrbitDistributionType;
	}
	return GeneratedWorld ? GeneratedWorld->OrbitDistributionType : EOrbitDistributionType::Uniform;
}

int32 UWorldGenerationViewModel::GetSelectedSystemPlanetCount() const
{
	FString Address;
	FStarSystemModel Current;
	if (PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewSystemEditContext(Address, Current)) return Current.PotentialPlanetCount;
	const EStarType Type = GetSelectedSystemStarType();
	const int32 Count = Type == EStarType::SingleStar ? 1 : Type == EStarType::DoubleStar ? 2 : Type == EStarType::TripleStar ? 3 : 4;
	return GeneratedWorld ? GeneratedWorld->PlanetsAmount * Count : 0;
}

void UWorldGenerationViewModel::SetSelectedSystemPlanetCount(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	FString Address;
	FStarSystemModel Current;
	if (!PreviewGenerator.IsValid() || !PreviewGenerator->GetPreviewSystemEditContext(Address, Current)) return;
	const int32 Total = FMath::RoundToInt(FMath::Clamp(Value, 0.0, 120.0));
	if (Current.PotentialPlanetCount == Total) return;
	FAPSPreviewSystemEditOverride Edit;
	if (const FAPSPreviewSystemEditOverride* Existing = GeneratedWorld->FindPreviewSystemEditOverride(Address)) Edit = *Existing;
	Edit.TotalPlanets = Total;
	const EPlanetarySystemType Type = GetSelectedSystemPlanetaryType();
	if (Total == 0 || Type == EPlanetarySystemType::NoPlanetSystem
		|| (Type == EPlanetarySystemType::SinglePlanetSystem && Total > Current.AmountOfStars))
	{
		Edit.bOverridePlanetaryType = true;
		Edit.PlanetaryType = Total == 0 ? EPlanetarySystemType::NoPlanetSystem : EPlanetarySystemType::MultiPlanetSystem;
	}
	GeneratedWorld->SetPreviewSystemEditOverride(Address, Edit);
	if (Address == TEXT("SYS0")) GeneratedWorld->StartPlanetIndex = FMath::Clamp(
		GeneratedWorld->StartPlanetIndex, 1, FMath::Max(1, GetHomeStartPlanetCount()));
	RequestPreview();
}

int32 UWorldGenerationViewModel::GetHomeStartPlanetCount() const
{
	return PreviewGenerator.IsValid() ? PreviewGenerator->GetPreviewHomePlanetCount()
		: GeneratedWorld ? GeneratedWorld->PlanetsAmount : 0;
}

bool UWorldGenerationViewModel::CanEditSelectedSystem() const
{
	FString Address;
	FStarSystemModel Current;
	return PreviewGenerator.IsValid() && PreviewGenerator->GetPreviewSystemEditContext(Address, Current);
}

void UWorldGenerationViewModel::SetGalaxyStarCount(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	// The persisted property remains int32-compatible for existing saves, while
	// the generated galaxy exposes the value through its virtual int64 catalog.
	// Clamp before conversion: RoundToInt on an unchecked Slate/Blueprint double
	// can assert in IntFitsIn long before a later integer clamp is reached.
	const int32 NewValue = FMath::RoundToInt(FMath::Clamp(Value, 1.0, 1000000000.0));
	if (GeneratedWorld->GalaxyStarCount != NewValue)
	{
		GeneratedWorld->GalaxyStarCount = NewValue;
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetGalaxyStarDensity(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	const double NewValue = FMath::Clamp(Value, 0.01, 1000.0);
	if (!FMath::IsNearlyEqual(GeneratedWorld->GalaxyStarDensity, NewValue))
	{
		GeneratedWorld->GalaxyStarDensity = NewValue;
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetPlanetRadius(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	// Reading/recommitting a generated value is not an authored radius change.
	if (Value == GeneratedWorld->PlanetRadius) return;
	const double NewValue = FMath::Clamp(Value, 1.0, 200000.0);
	if (!FMath::IsNearlyEqual(GeneratedWorld->PlanetRadius, NewValue))
	{
		GeneratedWorld->PlanetRadius = NewValue;
		if (bPreviewReady && PreviewFocus == EAstroPreviewFocus::HomePlanet
			&& IsValid(PreviewGenerator.Get())
			&& IsValid(PreviewGenerator->GetActivePreviewWorldScapeBody()))
		{
			RefreshPlanetAppearancePreview(true);
		}
		else
		{
			RequestPreview();
		}
	}
}

void UWorldGenerationViewModel::SetMoonsAmount(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	const int32 NewValue = FMath::RoundToInt(FMath::Clamp(Value, 0.0, 10.0));
	if (SelectedPreviewBody.IsValid() && SelectedPreviewBody->IsA<AMoon>())
	{
		// A moon cannot own another moon in the current hierarchy model. Keep the
		// selected-moon editor buffer stable instead of rebuilding the home planet.
		GeneratedWorld->MoonsAmount = 0;
		return;
	}
	if (GeneratedWorld->MoonsAmount != NewValue)
	{
		GeneratedWorld->MoonsAmount = NewValue;
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetSelectedMoonOrbitRadiusKm(const double Value)
{
	AMoon* Moon = Cast<AMoon>(SelectedPreviewBody.Get());
	if (!GeneratedWorld || !IsValid(Moon) || !IsValid(Moon->ParentPlanet)
		|| !FMath::IsFinite(Value))
	{
		return;
	}

	const double ParentRadiusKm = FMath::Max(
		Moon->ParentPlanet->RadiusKM,
		static_cast<double>(Moon->ParentPlanet->PlanetRadiusKM));
	const double MoonRadiusKm = FMath::Max(
		Moon->RadiusKM, static_cast<double>(Moon->PlanetRadiusKM));
	const double SurfaceClearanceKm = FMath::Max(
		ParentRadiusKm * 0.45, MoonRadiusKm * 0.75);
	double MinimumCenterRadiusKm = FMath::Max(
		100.0, ParentRadiusKm + MoonRadiusKm + SurfaceClearanceKm);
	const int32 MoonIndex = Moon->ParentPlanet->Moons.IndexOfByKey(Moon);
	if (MoonIndex > 0 && Moon->ParentPlanet->PlanetData.PlanetModel.IsValid())
	{
		const FPlanetModel& ParentModel = *Moon->ParentPlanet->PlanetData.PlanetModel;
		const int32 PreviousMoonIndex = MoonIndex - 1;
		if (ParentModel.MoonsList.IsValidIndex(PreviousMoonIndex)
			&& ParentModel.MoonsList[PreviousMoonIndex].IsValid())
		{
			const TSharedPtr<FMoonData>& PreviousMoonData =
				ParentModel.MoonsList[PreviousMoonIndex];
			const FMoonModel& PreviousMoonModel = PreviousMoonData->MoonModel.IsValid()
				? *PreviousMoonData->MoonModel : PreviousMoonData->MoonModelData;
			const double PreviousMoonRadiusKm = FMath::Max(
				static_cast<double>(PreviousMoonModel.RadiusKM),
				static_cast<double>(PreviousMoonModel.Radius) * 6371.0);
			const double PreviousCenterRadiusKm = ParentRadiusKm
				* FMath::Max(1.0 + PreviousMoonData->OrbitRadius, 1.0);
			const double InterMoonClearanceKm = FMath::Max(
				ParentRadiusKm * 0.28,
				(PreviousMoonRadiusKm + MoonRadiusKm) * 0.50);
			MinimumCenterRadiusKm = FMath::Max(MinimumCenterRadiusKm,
				PreviousCenterRadiusKm + PreviousMoonRadiusKm
					+ MoonRadiusKm + InterMoonClearanceKm);
		}
	}
	const double Clamped = FMath::Clamp(Value, MinimumCenterRadiusKm, 100000000.0);
	if (FMath::IsNearlyEqual(GeneratedWorld->MoonOrbitRadiusKm, Clamped))
	{
		return;
	}

	GeneratedWorld->MoonOrbitRadiusKm = Clamped;
	RefreshPlanetAppearancePreview(false);
}

void UWorldGenerationViewModel::SetPlanetSurfaceSeed(const int32 Value)
{
	if (!GeneratedWorld) return;
	const int32 Clamped = FMath::Clamp(Value, 0, 999983);
	if (GeneratedWorld->PlanetSurfaceSeed == Clamped) return;
	GeneratedWorld->PlanetSurfaceSeed = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetSurfaceFeatureScale(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	const double Clamped = FMath::Clamp(Value, 0.25, 4.0);
	if (FMath::IsNearlyEqual(GeneratedWorld->SurfaceFeatureScale, Clamped)) return;
	GeneratedWorld->SurfaceFeatureScale = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetSurfaceReliefScale(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	const double Clamped = FMath::Clamp(Value, 0.25, 2.5);
	if (FMath::IsNearlyEqual(GeneratedWorld->SurfaceReliefScale, Clamped)) return;
	GeneratedWorld->SurfaceReliefScale = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetSurfaceLandCoverageScale(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	const double Clamped = FMath::Clamp(Value, 0.25, 2.0);
	if (FMath::IsNearlyEqual(GeneratedWorld->SurfaceLandCoverageScale, Clamped)) return;
	GeneratedWorld->SurfaceLandCoverageScale = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetSurfaceMountainScale(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	const double Clamped = FMath::Clamp(Value, 0.0, 2.0);
	if (FMath::IsNearlyEqual(GeneratedWorld->SurfaceMountainScale, Clamped)) return;
	GeneratedWorld->SurfaceMountainScale = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetSurfaceCraterScale(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	const double Clamped = FMath::Clamp(Value, 0.0, 2.0);
	if (FMath::IsNearlyEqual(GeneratedWorld->SurfaceCraterScale, Clamped)) return;
	GeneratedWorld->SurfaceCraterScale = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetSurfaceRoughnessScale(const double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;
	const double Clamped = FMath::Clamp(Value, 0.25, 2.0);
	if (FMath::IsNearlyEqual(GeneratedWorld->SurfaceRoughnessScale, Clamped)) return;
	GeneratedWorld->SurfaceRoughnessScale = Clamped;
	RefreshPlanetAppearancePreview(true);
}

void UWorldGenerationViewModel::SetPlanetsAmount(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value))
	{
		return;
	}

	const int32 NewValue = FMath::RoundToInt(FMath::Clamp(Value, 1.0, 20.0));
	if (GeneratedWorld->PlanetsAmount != NewValue)
	{
		GeneratedWorld->PlanetsAmount = NewValue;
		GeneratedWorld->StartPlanetIndex = FMath::Clamp(
			GeneratedWorld->StartPlanetIndex, 1, GeneratedWorld->PlanetsAmount);
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetStartPlanetIndex(double Value)
{
	if (!GeneratedWorld || !FMath::IsFinite(Value)) return;

	const int32 MaxPlanetIndex = FMath::Max(1, GetHomeStartPlanetCount());
	const int32 NewIndex = FMath::RoundToInt(FMath::Clamp(
		Value, 1.0, static_cast<double>(MaxPlanetIndex)));
	if (GeneratedWorld->StartPlanetIndex != NewIndex)
	{
		GeneratedWorld->StartPlanetIndex = NewIndex;
		RequestPreview();
	}
}

void UWorldGenerationViewModel::PreserveSelectedPreviewBodyEdit(const bool bFlushPendingActor)
{
	AAstroGenerator* Generator = PreviewGenerator.Get();
	if (!IsValid(Generator) || !GeneratedWorld)
	{
		return;
	}

	// The panel owns one shared buffer, while the hierarchy owns many bodies. Only
	// associate it with an explicit active/selected body. SaveSelected's HomePlanet
	// fallback is useful for first entry, but unsafe after B -> GALAXY: at that point
	// the still-B buffer must never overwrite HomePlanet's retained record.
	APlanetaryBody* PreviousBody = Generator->GetActivePreviewWorldScapeBody();
	if (!IsValid(PreviousBody))
	{
		PreviousBody = Cast<APlanetaryBody>(SelectedPreviewBody.Get());
	}
	if (!IsValid(PreviousBody))
	{
		return;
	}
	Generator->SavePreviewBodyEditOverride(GeneratedWorld, PreviousBody);
	if (!bFlushPendingActor)
	{
		return;
	}

	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	const bool bTimerActive = World
		&& World->GetTimerManager().IsTimerActive(PlanetAppearanceTimerHandle);
	if (bPendingSurfaceAppearanceRefresh || bTimerActive)
	{
		AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(
			GeneratedWorld, Generator->GetPreviewBodyStableKey(PreviousBody), PreviousBody);
	}
	if (World)
	{
		World->GetTimerManager().ClearTimer(PlanetAppearanceTimerHandle);
	}
	bPendingSurfaceAppearanceRefresh = false;
}

void UWorldGenerationViewModel::RequestPreview()
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World || !GeneratedWorld)
	{
		return;
	}
	if (bSkipBodyOverrideSnapshotOnce)
	{
		bSkipBodyOverrideSnapshotOnce = false;
	}
	else
	{
		PreserveSelectedPreviewBodyEdit(true);
	}
	// A structural rebuild already consumes the latest surface and atmosphere
	// values, so a pending local shell refresh would only touch the soon-to-be
	// replaced actor and can make a slider drag hitch twice.
	World->GetTimerManager().ClearTimer(PlanetAppearanceTimerHandle);
	bPendingSurfaceAppearanceRefresh = false;

	// A parameter drag can enqueue many debounced rebuilds. Remember that a
	// useful live camera already existed before PreviewStatus temporarily marks
	// the scene as updating, so a local edit does not recenter orbit/zoom.
	if (!bForceRefocusOnNextPreview)
	{
		bPreserveCameraOnNextPreview = bPreserveCameraOnNextPreview || bPreviewReady;
	}
	else
	{
		bPreserveCameraOnNextPreview = false;
	}
	UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
	SetPreviewStatus(LOCTEXT("PreviewUpdating", "UPDATING LIVE SCENE"), false);
	World->GetTimerManager().SetTimer(
		PreviewTimerHandle, this, &UWorldGenerationViewModel::ExecutePreview, 0.45f, false);
}

void UWorldGenerationViewModel::RefreshPlanetAppearancePreview(const bool bRegenerateSurface)
{
	if (!GeneratedWorld)
	{
		return;
	}
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World || !bPreviewReady || !IsValid(PreviewGenerator.Get()))
	{
		RequestPreview();
		return;
	}
	// Persist the exact selected-body editor buffer before debounce. A structural
	// control may cancel this timer and rebuild the hierarchy before the actor ever
	// receives the value, but its stable path must still retain the user's edit.
	PreviewGenerator->SaveSelectedPreviewBodyEditOverride(GeneratedWorld);

	// SSpinBox emits every intermediate drag value. WorldScape cannot safely and
	// usefully rebuild a profile for all of them, so coalesce the burst while still
	// keeping the preview perceptually live. Atmosphere-only changes share the same
	// short window and never trigger a hierarchy rebuild or camera refocus.
	bPendingSurfaceAppearanceRefresh = bPendingSurfaceAppearanceRefresh || bRegenerateSurface;
	World->GetTimerManager().SetTimer(
		PlanetAppearanceTimerHandle, this,
		&UWorldGenerationViewModel::ExecutePlanetAppearancePreviewRefresh, 0.12f, false);
}

void UWorldGenerationViewModel::ExecutePlanetAppearancePreviewRefresh()
{
	const bool bRegenerateSurface = bPendingSurfaceAppearanceRefresh;
	bPendingSurfaceAppearanceRefresh = false;
	AAstroGenerator* Generator = PreviewGenerator.Get();
	if (GeneratedWorld && bPreviewReady && IsValid(Generator)
		&& Generator->RefreshPreviewPlanetAppearance(GeneratedWorld, bRegenerateSurface))
	{
		UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
		return;
	}
	RequestPreview();
}

void UWorldGenerationViewModel::CancelPendingPreview()
{
	if (UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(PreviewTimerHandle);
		World->GetTimerManager().ClearTimer(PlanetAppearanceTimerHandle);
	}
	bPendingSurfaceAppearanceRefresh = false;
}

void UWorldGenerationViewModel::RegeneratePreviewVariant()
{
	if (GeneratedWorld)
	{
		GeneratedWorld->ClearPreviewBodyEditOverrides();
		GeneratedWorld->ClearPreviewStarEditOverrides();
		GeneratedWorld->ClearPreviewSystemEditOverrides();
		bSkipBodyOverrideSnapshotOnce = true;
	}
	if (AAstroGenerator* Generator = FindOrCreatePreviewGenerator())
	{
		Generator->AdvancePreviewGenerationSeed();
	}
	bForceRefocusOnNextPreview = true;
	RequestPreview();
}

void UWorldGenerationViewModel::SetPreviewFocus(EAstroPreviewFocus NewFocus)
{
	// Buttons and hierarchy navigation may outlive the actors for one debounce
	// frame. Never move a ready preview to a scope that this generation level did
	// not create; before the first build the requested focus is still allowed.
	if (bPreviewReady && !IsPreviewFocusAvailable(NewFocus))
	{
		return;
	}
	if (bPreviewReady && NewFocus == EAstroPreviewFocus::HomeSystem
		&& PreviewFocus == NewFocus && IsPreviewingClusterSystemProxy())
	{
		// SYSTEM is the current lightweight cluster record. Re-running the root
		// focus command would clear that record before a materialized replacement
		// exists and briefly frame the generator fallback.
		return;
	}
	const bool bSelectionResets = NewFocus != PreviewFocus || SelectedPreviewBody.IsValid();
	PreserveSelectedPreviewBodyEdit(bSelectionResets);
	PreviewFocus = NewFocus;
	SelectedPreviewBody.Reset();
	// Before the first live model is ready an existing level generator may still
	// contain its old background world. Remember the desired focus, but do not
	// steer the camera toward an actor that RequestPreview is about to destroy.
	if (!bPreviewReady)
	{
		return;
	}
	if (AAstroGenerator* Generator = FindOrCreatePreviewGenerator())
	{
		APlayerController* PC = WorldContext.IsValid() && WorldContext->GetWorld()
			? WorldContext->GetWorld()->GetFirstPlayerController() : nullptr;
		Generator->FocusPreviewTarget(NewFocus, PC);
		if (Generator->UsesContinuousPreviewFrame())
		{
			SelectedPreviewBody = Generator->GetSelectedPreviewBodyActor();
			if (APlanetaryBody* Body = Cast<APlanetaryBody>(SelectedPreviewBody.Get()))
				HydratePreviewBodyEditorBuffer(Body);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Focus=%d generator=%s"),
			static_cast<int32>(NewFocus), *GetNameSafe(Generator));
	}
}

void UWorldGenerationViewModel::SetGenerationRoute(EAPSGenerationRoute NewRoute)
{
	if (!GeneratedWorld)
	{
		GenerationRoute = NewRoute;
		return;
	}

	const EAPSGenerationRoute PreviousRoute = GenerationRoute;
	GenerationRoute = NewRoute;
	bForceRefocusOnNextPreview = true;
	if (NewRoute == EAPSGenerationRoute::Planet)
	{
		GeneratedWorld->AstroGenerationLevel = EAstroGenerationLevel::SinglePlanet;
		GeneratedWorld->bGenerateHomeSystem = false;
		GeneratedWorld->bStartWithHomePlanet = true;
		GeneratedWorld->PlanetsAmount = 1;
		GeneratedWorld->StartPlanetIndex = 1;
	}
	else
	{
		// Leaving the dedicated planet route restores a complete astronomical
		// hierarchy. The user may still choose another generation level explicitly.
		if (PreviousRoute == EAPSGenerationRoute::Planet
			|| GeneratedWorld->AstroGenerationLevel == EAstroGenerationLevel::SinglePlanet)
		{
			GeneratedWorld->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
		}
		GeneratedWorld->bGenerateHomeSystem = true;
		GeneratedWorld->bStartWithHomePlanet = true;
		GeneratedWorld->PlanetsAmount = FMath::Max(1, GeneratedWorld->PlanetsAmount);
		GeneratedWorld->StartPlanetIndex = FMath::Clamp(
			GeneratedWorld->StartPlanetIndex, 1, GeneratedWorld->PlanetsAmount);
	}
	RequestPreview();
}

void UWorldGenerationViewModel::OrbitPreview(FVector2D ScreenDelta)
{
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->OrbitPreviewCamera(ScreenDelta);
	}
}

void UWorldGenerationViewModel::BeginPreviewOrbit()
{
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->BeginPreviewCameraOrbit();
	}
}

void UWorldGenerationViewModel::EndPreviewOrbit()
{
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->EndPreviewCameraOrbit();
	}
}

void UWorldGenerationViewModel::ZoomPreview(float WheelDelta)
{
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->ZoomPreviewCamera(WheelDelta);
	}
}

bool UWorldGenerationViewModel::FocusPreviewUnderCursor()
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	return Controller && Controller->GetMousePosition(MouseX, MouseY)
		&& FocusPreviewAtScreenPosition(FVector2D(MouseX, MouseY));
}

bool UWorldGenerationViewModel::FocusPreviewAtScreenPosition(const FVector2D& ScreenPosition)
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PlayerController)
	{
		return false;
	}

	const double MouseX = ScreenPosition.X;
	const double MouseY = ScreenPosition.Y;
	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection))
	{
		return false;
	}
	if (AAstroGenerator* ContinuousGenerator = PreviewGenerator.Get();
		ContinuousGenerator && ContinuousGenerator->UsesContinuousPreviewFrame())
	{
		// Presentation meshes deliberately have no physical collision. Picking their
		// old actor-space volumes would select invisible zones or a background system.
		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
		const FVector CameraRight = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
		AActor* PickedBody = nullptr;
		double BestDepth = TNumericLimits<double>::Max();
		for (TActorIterator<ACelestialBody> It(World); It; ++It)
		{
			AActor* Candidate = *It;
			if ((!Candidate->IsA<AStar>() && !Candidate->IsA<APlanetaryBody>())
				|| !Candidate->IsAttachedTo(ContinuousGenerator)) continue;
			FVector Center;
			double Radius = 0.0;
			if (!ContinuousGenerator->GetPreviewPresentationLocation(Candidate, Center)
				|| !ContinuousGenerator->GetPreviewPresentationRadius(Candidate, Radius)) continue;
			FVector2D ScreenCenter, ScreenLimb;
			if (!PlayerController->ProjectWorldLocationToScreen(Center, ScreenCenter, true)
				|| !PlayerController->ProjectWorldLocationToScreen(Center + CameraRight * Radius, ScreenLimb, true)) continue;
			const double PickRadius = FMath::Max(8.0, FVector2D::Distance(ScreenCenter, ScreenLimb) * 1.05);
			if (FVector2D::DistSquared(FVector2D(MouseX, MouseY), ScreenCenter) > PickRadius * PickRadius) continue;
			const double Depth = FVector::DotProduct(Center - RayOrigin, RayDirection) - Radius;
			if (Depth < BestDepth)
			{
				BestDepth = Depth;
				PickedBody = Candidate;
			}
		}
		if (PickedBody) return FocusPreviewBody(PickedBody);
		PreserveSelectedPreviewBodyEdit(true);
		if (ContinuousGenerator->FocusPreviewClusterSystemAtScreenPosition(PlayerController, FVector2D(MouseX, MouseY)))
		{
			PreviewFocus = EAstroPreviewFocus::HomeSystem;
			SelectedPreviewBody.Reset();
			UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
			return true;
		}
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstroPreviewPick), true);
	QueryParams.bTraceComplex = false;
	const bool bBlockingHit = World->LineTraceSingleByChannel(
		Hit, RayOrigin, RayOrigin + RayDirection * 1.0e15, ECC_Visibility, QueryParams);
	if (bBlockingHit)
	{
		for (AActor* Candidate = Hit.GetActor(); IsValid(Candidate); Candidate = Candidate->GetAttachParentActor())
		{
			if (Candidate->IsA<APlanet>() || Candidate->IsA<AMoon>() || Candidate->IsA<AStar>())
			{
				return FocusPreviewBody(Candidate);
			}
			if (Candidate->IsA<AStarSystem>()) { SetPreviewFocus(EAstroPreviewFocus::HomeSystem); return true; }
			if (Candidate->IsA<AGalaxy>()) { SetPreviewFocus(EAstroPreviewFocus::Galaxy); return true; }
			if (Candidate->IsA<AStarCluster>()) { SetPreviewFocus(EAstroPreviewFocus::StarCluster); return true; }
		}
		return false;
	}

	if (AAstroGenerator* Generator = PreviewGenerator.Get();
		Generator && Generator->FocusPreviewClusterSystemAtScreenPosition(
			PlayerController, FVector2D(MouseX, MouseY)))
	{
		PreviewFocus = EAstroPreviewFocus::HomeSystem;
		SelectedPreviewBody.Reset();
		UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
		return true;
	}
	return false;
}

void UWorldGenerationViewModel::GetPreviewBodyEntries(TArray<FAPSPreviewBodyEntry>& OutEntries) const
{
	OutEntries.Reset();
	if (const AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->GetPreviewBodyEntries(OutEntries);
	}
}

bool UWorldGenerationViewModel::GetPreviewPresentationLocation(
	const AActor* Actor, FVector& OutLocation) const
{
	if (const AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		return Generator->GetPreviewPresentationLocation(Actor, OutLocation);
	}
	return false;
}

bool UWorldGenerationViewModel::GetPreviewFocusSphere(FVector& OutCenter, double& OutRadius) const
{
	if (const AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		return Generator->GetPreviewFocusSphere(PreviewFocus, OutCenter, OutRadius);
	}
	return false;
}

bool UWorldGenerationViewModel::GetPreviewFocusSphere(
	EAstroPreviewFocus Focus, FVector& OutCenter, double& OutRadius) const
{
	if (const AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		return Generator->GetPreviewFocusSphere(Focus, OutCenter, OutRadius);
	}
	return false;
}

bool UWorldGenerationViewModel::IsPreviewingClusterSystemProxy() const
{
	const AAstroGenerator* Generator = PreviewGenerator.Get();
	return Generator && Generator->HasSelectedPreviewClusterSystem()
		&& !Generator->UsesContinuousPreviewFrame();
}

bool UWorldGenerationViewModel::IsPreviewFocusAvailable(const EAstroPreviewFocus Focus) const
{
	const AAstroGenerator* Generator = PreviewGenerator.Get();
	return Generator && Generator->IsPreviewFocusAvailable(Focus);
}

void UWorldGenerationViewModel::HydratePreviewBodyEditorBuffer(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !GeneratedWorld)
	{
		return;
	}

	GeneratedWorld->PlanetType = Body->PlanetType;
	GeneratedWorld->PlanetHabitability = Body->PlanetHabitability;
	// The generated physical radius is authoritative, including sub-kilometre
	// precision and gas giants larger than the old terrestrial authoring range.
	GeneratedWorld->PlanetRadius = FMath::IsFinite(Body->RadiusKM) && Body->RadiusKM > 0.0
		? Body->RadiusKM : FMath::Max(static_cast<double>(Body->PlanetRadiusKM), 1.0);
	GeneratedWorld->MoonOrbitRadiusKm = 0.0;
	if (const AMoon* Moon = Cast<AMoon>(Body); IsValid(Moon) && IsValid(Moon->ParentPlanet))
	{
		const APlanet* ParentPlanet = Moon->ParentPlanet;
		const int32 MoonIndex = ParentPlanet->Moons.IndexOfByKey(Moon);
		if (ParentPlanet->PlanetData.PlanetModel.IsValid()
			&& ParentPlanet->PlanetData.PlanetModel->MoonsList.IsValidIndex(MoonIndex)
			&& ParentPlanet->PlanetData.PlanetModel->MoonsList[MoonIndex].IsValid())
		{
			const double ParentRadiusKm = FMath::Max(
				static_cast<double>(ParentPlanet->PlanetData.PlanetModel->RadiusKM),
				ParentPlanet->RadiusKM);
			GeneratedWorld->MoonOrbitRadiusKm = ParentRadiusKm *
				(1.0 + ParentPlanet->PlanetData.PlanetModel->MoonsList[MoonIndex]->OrbitRadius);
		}
		else
		{
			GeneratedWorld->MoonOrbitRadiusKm = FVector::Distance(
				Moon->GetActorLocation(), ParentPlanet->GetActorLocation()) / 100000.0;
		}
	}
	GeneratedWorld->PlanetSurfaceSeed = FMath::Clamp(Body->WorldScapeSeed, 0, 999983);
	GeneratedWorld->SurfaceFeatureScale = FMath::Clamp(Body->SurfaceFeatureScale, 0.25, 4.0);
	GeneratedWorld->SurfaceReliefScale = FMath::Clamp(Body->SurfaceReliefScale, 0.25, 2.5);
	GeneratedWorld->SurfaceLandCoverageScale = FMath::Clamp(
		Body->SurfaceLandCoverageScale, 0.25, 2.0);
	GeneratedWorld->SurfaceMountainScale = FMath::Clamp(Body->SurfaceMountainScale, 0.0, 2.0);
	GeneratedWorld->SurfaceCraterScale = FMath::Clamp(Body->SurfaceCraterScale, 0.0, 2.0);
	GeneratedWorld->SurfaceRoughnessScale = FMath::Clamp(Body->SurfaceRoughnessScale, 0.25, 2.0);
	GeneratedWorld->AtmosphereHeight = FMath::Clamp(Body->AtmosphereHeight, 0.0, 2000.0);
	// Bodies without a materialized AtmoScape must start from their own defaults,
	// never from whichever planet happened to be selected immediately before them.
	GeneratedWorld->AtmosphereOpacity = 12.0;
	GeneratedWorld->AtmosphereMultiScattering = 5.0;
	GeneratedWorld->AtmosphereRayleighScattering = 8.0;
	GeneratedWorld->AtmosphereColor = FLinearColor(3.8f, 13.5f, 33.0f, 0.0f);
	GeneratedWorld->MoonsAmount = Cast<APlanet>(Body)
		? FMath::Clamp(CastChecked<APlanet>(Body)->AmountOfMoons, 0, 10) : 0;
	if (IsValid(Body->PlanetaryEnvironmentGenerator)
		&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
	{
		const AAtmoScape* Atmosphere = Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
		GeneratedWorld->AtmosphereHeight = FMath::Clamp(
			static_cast<double>(Atmosphere->AtmosphereHeight), 0.0, 2000.0);
		GeneratedWorld->AtmosphereOpacity = FMath::Clamp(
			static_cast<double>(Atmosphere->AtmosphereOpacity), 0.0, 40.0);
		GeneratedWorld->AtmosphereMultiScattering = FMath::Clamp(
			static_cast<double>(Atmosphere->MultiScatering), 0.0, 10.0);
		GeneratedWorld->AtmosphereRayleighScattering = FMath::Clamp(
			static_cast<double>(Atmosphere->RayleighHeight), 0.0, 64.0);
		GeneratedWorld->AtmosphereColor = Atmosphere->RayleighScattering;
	}

	// A materialized actor already owns retained terrain fields, while the shared
	// editor buffer also contains atmosphere controls. A complete saved snapshot,
	// when present, is authoritative over generated defaults.
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->LoadPreviewBodyEditOverride(GeneratedWorld, Body);
	}
}

bool UWorldGenerationViewModel::IsSelectedPreviewBodyMoon() const
{
	return IsValid(Cast<AMoon>(SelectedPreviewBody.Get()));
}

bool UWorldGenerationViewModel::FocusPreviewBody(const TWeakObjectPtr<AActor>& BodyActor)
{
	AAstroGenerator* Generator = PreviewGenerator.Get();
	AActor* Actor = BodyActor.Get();
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APlanetaryBody* PreviousBody = Generator
		? Generator->GetActivePreviewWorldScapeBody() : nullptr;
	if (!IsValid(PreviousBody))
	{
		PreviousBody = Cast<APlanetaryBody>(SelectedPreviewBody.Get());
	}
	const bool bSelectionChanges = IsValid(PreviousBody) && PreviousBody != Actor;
	const bool bPendingPreviousSurface = bPendingSurfaceAppearanceRefresh;
	const bool bPendingPreviousAppearance = bSelectionChanges && World
		&& (bPendingSurfaceAppearanceRefresh
			|| World->GetTimerManager().IsTimerActive(PlanetAppearanceTimerHandle));
	// Snapshot A on every selection. Only a real A->B switch drains/cancels A's
	// debounce; re-clicking A preserves its expected in-place WorldScape refresh.
	if (IsValid(PreviousBody))
	{
		PreserveSelectedPreviewBodyEdit(bSelectionChanges);
	}
	else if (Generator && GeneratedWorld
		&& GeneratedWorld->GetPreviewBodyEditOverrideCount() == 0)
	{
		// First hierarchy-body selection starts from the generated HomePlanet buffer;
		// capture that one deliberate fallback before hydrating the clicked body.
		Generator->SaveSelectedPreviewBodyEditOverride(GeneratedWorld);
	}
	if (bPendingPreviousAppearance && Generator && GeneratedWorld
		&& Generator->GetActivePreviewWorldScapeBody() == PreviousBody)
	{
		// PreserveSelected applied the final coalesced A values to A's actor and
		// cancelled its timer. Queue one atomic replacement for A before B becomes the
		// editor target; B's hydration can no longer leak into that deferred rebuild,
		// while A's last complete proxy remains visible until its replacement commits.
		Generator->RefreshPreviewPlanetAppearance(
			GeneratedWorld, bPendingPreviousSurface);
	}
	APlanetaryBody* NewBody = Cast<APlanetaryBody>(Actor);
	if (IsValid(NewBody) && GeneratedWorld)
	{
		// Hydrate B before asking the generator to make B active. The surface build is
		// deferred to the next generator tick, so this ordering publishes one atomic
		// A -> B editor state instead of briefly exposing A's sliders on B's globe.
		HydratePreviewBodyEditorBuffer(NewBody);
	}
	if (!Generator || !Actor || !Generator->FocusPreviewBodyActor(Actor, PlayerController))
	{
		if (IsValid(PreviousBody) && GeneratedWorld)
		{
			HydratePreviewBodyEditorBuffer(PreviousBody);
		}
		return false;
	}

	SelectedPreviewBody = Actor;
	PreviewFocus = Actor->IsA<AStar>() ? EAstroPreviewFocus::HomeStar : EAstroPreviewFocus::HomePlanet;
	if (IsValid(NewBody) && GeneratedWorld)
	{
		UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
	}
	return true;
}

bool UWorldGenerationViewModel::FocusPreviewClusterSystem(int32 InstanceIndex)
{
	AAstroGenerator* Generator = PreviewGenerator.Get();
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	PreserveSelectedPreviewBodyEdit(true);
	if (!Generator || !Generator->FocusPreviewClusterSystem(InstanceIndex, PlayerController))
	{
		return false;
	}

	PreviewFocus = EAstroPreviewFocus::HomeSystem;
	SelectedPreviewBody.Reset();
	UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
	return true;
}

FText UWorldGenerationViewModel::GetPreviewScopeSummary() const
{
	if (!GeneratedWorld)
	{
		return LOCTEXT("NoPreviewModel", "NO MODEL");
	}

	const AAstroGenerator* Generator = PreviewGenerator.Get();
	const auto EnumText = [](const auto Value)
	{
		return UEnum::GetDisplayValueAsText(Value).ToString().ToUpper();
	};

	switch (PreviewFocus)
	{
	case EAstroPreviewFocus::Galaxy:
		return FText::FromString(FString::Printf(
			TEXT("GALAXY TYPE  %s\nCLASS  %s\nMODELED STARS  %lld\nRENDERED SAMPLE  %d\nSIZE  %d  /  DENSITY  %.2f"),
			*EnumText(GeneratedWorld->GalaxyType), *EnumText(GeneratedWorld->GalaxyClass),
			Generator ? Generator->GetPreviewGalaxyModeledStarCount()
				: static_cast<int64>(GeneratedWorld->GalaxyStarCount),
			Generator ? Generator->GetPreviewGalaxyRenderedStarCount() : 0,
			GeneratedWorld->GalaxySize, GeneratedWorld->GalaxyStarDensity));

	case EAstroPreviewFocus::StarCluster:
		return FText::FromString(FString::Printf(
			TEXT("FORMATION  %s\nSIZE  %s\nPOPULATION  %s\nCOMPOSITION  %s\nMODELED SYSTEMS  %d  /  RENDERED STARS  %d"),
			*EnumText(GeneratedWorld->StarClusterType), *EnumText(GeneratedWorld->StarClusterSize),
			*EnumText(GeneratedWorld->StarClusterPopulation), *EnumText(GeneratedWorld->StarClusterComposition),
			Generator ? Generator->GetPreviewClusterModeledSystemCount() : 0,
			Generator ? Generator->GetPreviewClusterRenderedStarCount() : 0));

	case EAstroPreviewFocus::HomeSystem:
	{
		const AStarSystem* LiveSystem = Generator ? Generator->GetContinuousPreviewActiveSystem() : nullptr;
		if (IsValid(LiveSystem))
		{
			if (!bPreviewReady) return LOCTEXT("SystemUpdating", "UPDATING SELECTED SYSTEM...");
			int32 PlanetCount = 0;
			for (const AStar* Star : LiveSystem->GetStars())
				if (IsValid(Star) && IsValid(Star->PlanetarySystem)) PlanetCount += Star->PlanetarySystem->PlanetsActorsList.Num();
			const int32 Stars = LiveSystem->GetStars().Num();
			const EStarType Type = Stars == 1 ? EStarType::SingleStar : Stars == 2 ? EStarType::DoubleStar
				: Stars == 3 ? EStarType::TripleStar : EStarType::MultipleStar;
			const APlanetarySystem* Family = IsValid(LiveSystem->MainStar) ? LiveSystem->MainStar->PlanetarySystem : nullptr;
			const FString Context = LiveSystem->StableSystemId == GeneratedWorld->CanonicalStellarDataset.HomeStableId
				? FString::Printf(TEXT("HOME START PLANET  %d"), GeneratedWorld->StartPlanetIndex)
				: TEXT("CLUSTER SYSTEM  ") + LiveSystem->StableSystemId.ToString(EGuidFormats::Short);
			return FText::FromString(FString::Printf(
				TEXT("SYSTEM TYPE  %s\nORBIT DISTRIBUTION  %s\nSTARS  %d  /  TOTAL PLANETS  %d\n%s\nSTATE  LIVE PHYSICAL HIERARCHY"),
				*EnumText(Type), Family ? *EnumText(Family->OrbitDistributionType) : TEXT("--"), Stars, PlanetCount, *Context));
		}
		FString SelectedSystemId;
		int32 SelectedSystemStars = 0;
		int32 SelectedSystemPlanets = 0;
		if (Generator && Generator->GetSelectedPreviewClusterSystemSummary(
			SelectedSystemId, SelectedSystemStars, SelectedSystemPlanets))
		{
			if (Generator->UsesContinuousPreviewFrame())
			{
				const AStarSystem* System = Generator->GetContinuousPreviewActiveSystem();
				if (IsValid(System))
				{
					int32 PlanetCount = 0;
					for (const AStar* Star : System->GetStars())
						if (IsValid(Star) && IsValid(Star->PlanetarySystem))
							PlanetCount += Star->PlanetarySystem->PlanetsActorsList.Num();
					return FText::FromString(FString::Printf(
						TEXT("CLUSTER SYSTEM  %s\nSTARS  %d\nPLANETS  %d\nSTATE  LIVE PHYSICAL HIERARCHY"),
						*SelectedSystemId, System->GetStars().Num(), PlanetCount));
				}
			}
			return FText::FromString(FString::Printf(
				TEXT("CLUSTER SYSTEM  %s\nSTARS  %d\nPOTENTIAL PLANETS  %d\nSTATE  LIGHTWEIGHT FULL-SCALE RECORD"),
				*SelectedSystemId, SelectedSystemStars, SelectedSystemPlanets));
		}
		int32 StarCount = 1;
		switch (GeneratedWorld->StarType)
		{
		case EStarType::DoubleStar: StarCount = 2; break;
		case EStarType::TripleStar: StarCount = 3; break;
		case EStarType::MultipleStar: StarCount = FMath::Max(4, GeneratedWorld->StarsAmount); break;
		default: break;
		}
		return FText::FromString(FString::Printf(
			TEXT("SYSTEM TYPE  %s\nORBIT DISTRIBUTION  %s\nSTARS  %d  /  PLANETS PER STAR  %d  /  TOTAL  %d\nHOME MOONS  %d\nSTART PLANET  %d"),
			*EnumText(GeneratedWorld->StarType), *EnumText(GeneratedWorld->OrbitDistributionType),
			StarCount, GeneratedWorld->PlanetsAmount, StarCount * GeneratedWorld->PlanetsAmount,
			GeneratedWorld->MoonsAmount, GeneratedWorld->StartPlanetIndex));
	}

	case EAstroPreviewFocus::HomeStar:
		if (const AStar* Star = Generator ? Cast<AStar>(Generator->GetSelectedPreviewBodyActor()) : nullptr)
		{
			return FText::FromString(FString::Printf(
				TEXT("STELLAR TYPE  %s\nSPECTRAL CLASS  %s\nRADIUS  %.3f KM\nPLANETS  %d\nSAFE ORBIT CLEARANCE  LIVE"),
				*EnumText(Star->StellarClass), *EnumText(Star->SpectralClass), Star->RadiusKM,
				IsValid(Star->PlanetarySystem) ? Star->PlanetarySystem->PlanetsActorsList.Num() : 0));
		}
		return FText::FromString(FString::Printf(
			TEXT("STELLAR TYPE  %s\nSPECTRAL CLASS  %s\nSYSTEM PLANETS  %d\nSAFE ORBIT CLEARANCE  LIVE"),
			*EnumText(GeneratedWorld->StellarType), *EnumText(GeneratedWorld->SpectralClass),
			GeneratedWorld->PlanetsAmount));

	case EAstroPreviewFocus::HomePlanet:
	{
		const APlanetaryBody* SurfaceBody = Generator
			? Generator->GetActivePreviewWorldScapeBody() : nullptr;
		const TCHAR* SurfaceStatus = !IsValid(SurfaceBody)
			? TEXT("UNAVAILABLE")
			: SurfaceBody->bWorldScapeSurfaceReady ? TEXT("READY") : TEXT("GENERATING");
		return FText::FromString(FString::Printf(
			TEXT("PLANET TYPE  %s\nHABITABILITY  %s\nRADIUS  %.0f KM\nMOONS  %d\nWORLDSCAPE SURFACE  %s\nFULL-SCALE DATA  %s"),
			*EnumText(GeneratedWorld->PlanetType), *EnumText(GeneratedWorld->PlanetHabitability),
			GeneratedWorld->PlanetRadius, GeneratedWorld->MoonsAmount,
			SurfaceStatus,
			GeneratedWorld->bGenerateFullScaledWorld ? TEXT("ON") : TEXT("OFF")));
	}

	case EAstroPreviewFocus::Overview:
	default:
	{
		const AStarSystem* Home = Generator ? Generator->GetPreviewHomeSystem() : nullptr;
		int32 HomePlanetCount = 0;
		if (IsValid(Home))
			for (const AStar* Star : Home->GetStars())
				if (IsValid(Star) && IsValid(Star->PlanetarySystem)) HomePlanetCount += Star->PlanetarySystem->PlanetsActorsList.Num();
		return FText::FromString(FString::Printf(
			TEXT("GALAXY STARS  %d\nCLUSTER  %s / %s\nHOME SYSTEM PLANETS  %d\nHOME START PLANET  %d\nFULL SCALE  %s"),
			GeneratedWorld->GalaxyStarCount, *EnumText(GeneratedWorld->StarClusterSize),
			*EnumText(GeneratedWorld->StarClusterType), HomePlanetCount,
			GeneratedWorld->StartPlanetIndex, GeneratedWorld->bGenerateFullScaledWorld ? TEXT("ON") : TEXT("OFF")));
	}
	}
}

FText UWorldGenerationViewModel::GetPreviewHierarchyTitle() const
{
	switch (PreviewFocus)
	{
	case EAstroPreviewFocus::Galaxy: return LOCTEXT("GalaxyModelTitle", "GALAXY MODEL");
	case EAstroPreviewFocus::StarCluster: return LOCTEXT("ClusterSystemsTitle", "CLUSTER STAR SYSTEMS");
	case EAstroPreviewFocus::HomeSystem: return LOCTEXT("SystemBodiesTitle", "SYSTEM BODIES");
	case EAstroPreviewFocus::HomeStar: return LOCTEXT("SelectedStarTitle", "SELECTED STAR");
	case EAstroPreviewFocus::HomePlanet: return LOCTEXT("PlanetSatellitesTitle", "PLANET & SATELLITES");
	case EAstroPreviewFocus::Overview: return LOCTEXT("WorldHierarchyTitle", "WORLD HIERARCHY");
	default: return LOCTEXT("HierarchyTitle", "ASTRONOMICAL HIERARCHY");
	}
}

void UWorldGenerationViewModel::SetSpawnClass(EAPSStartAssetSlot Slot, UClass* NewClass)
{
	if (!SpawnParameters || !NewClass)
	{
		return;
	}

	switch (Slot)
	{
	case EAPSStartAssetSlot::Character:
		if (NewClass->IsChildOf(APawn::StaticClass())) SpawnParameters->BP_CharacterClass = NewClass;
		break;
	case EAPSStartAssetSlot::Spaceship:
		if (NewClass->IsChildOf(ASpaceship::StaticClass())) SpawnParameters->BP_HomeSpaceship = NewClass;
		break;
	case EAPSStartAssetSlot::SpaceStation:
		if (NewClass->IsChildOf(ASpaceStation::StaticClass())) SpawnParameters->BP_HomeSpaceStation = NewClass;
		break;
	case EAPSStartAssetSlot::Headquarters:
		if (NewClass->IsChildOf(ASpaceHeadquarters::StaticClass())) SpawnParameters->BP_HomeSpaceHeadquarters = NewClass;
		break;
	case EAPSStartAssetSlot::Shipyard:
		if (NewClass->IsChildOf(ASpaceShipyard::StaticClass())) SpawnParameters->BP_HomeSpaceShipyard = NewClass;
		break;
	}
}

void UWorldGenerationViewModel::SetCharacterSpawnPlace(int32 Value)
{
	if (SpawnParameters)
	{
		const int32 MaxValue = StaticEnum<ECharSpawnPlace>()->NumEnums() - 2;
		SpawnParameters->CharacterSpawnPlace = static_cast<ECharSpawnPlace>(FMath::Clamp(Value, 0, MaxValue));
		if (GeneratedWorld
			&& (SpawnParameters->CharacterSpawnPlace == ECharSpawnPlace::MoonOrbit
				|| SpawnParameters->CharacterSpawnPlace == ECharSpawnPlace::MoonSurface))
		{
			// A lunar start must commit a hierarchy that actually contains a moon.
			// Keep the user's larger value, only repairing the impossible zero case.
			GeneratedWorld->MoonsAmount = FMath::Max(1, GeneratedWorld->MoonsAmount);
		}
	}
}

void UWorldGenerationViewModel::SetStationOrbitHeight(int32 Value)
{
	if (SpawnParameters)
	{
		const int32 MaxValue = StaticEnum<EOrbitHeight>()->NumEnums() - 2;
		SpawnParameters->HomeStationOrbitHeight = static_cast<EOrbitHeight>(FMath::Clamp(Value, 0, MaxValue));
	}
}

void UWorldGenerationViewModel::ExecutePreview()
{
	AAstroGenerator* Generator = FindOrCreatePreviewGenerator();
	if (!Generator)
	{
		SetPreviewStatus(LOCTEXT("PreviewGeneratorMissing", "PREVIEW GENERATOR UNAVAILABLE"), false);
		return;
	}

	// Set the requested hierarchy scope before generation.  On first launch the
	// generator's historical PLANET default otherwise starts WorldScape work even
	// when Landing/Choose Path explicitly asked for a Galaxy background.
	const bool bGenerated = Generator->RegeneratePreview(GeneratedWorld, PreviewFocus);
	if (bGenerated && Generator->UsesContinuousPreviewFrame()) PreviewFocus = Generator->GetCurrentPreviewFocus();
	if (bGenerated && !bPreserveCameraOnNextPreview && !Generator->UsesContinuousPreviewFrame())
	{
		// RegeneratePreview resolves an object selection by hierarchy indices. Use
		// that new actor for a forced refocus as well; FocusPreviewTarget intentionally
		// means "root button" and would otherwise replace Star B/Planet N with A/01.
		AActor* RestoredBody = Generator->GetSelectedPreviewBodyActor();
		if ((PreviewFocus == EAstroPreviewFocus::HomeStar
			|| PreviewFocus == EAstroPreviewFocus::HomePlanet) && IsValid(RestoredBody))
		{
			Generator->FocusPreviewBodyActor(RestoredBody);
		}
		else
		{
			Generator->FocusPreviewTarget(PreviewFocus);
		}
	}
	if (bGenerated && (PreviewFocus == EAstroPreviewFocus::HomeStar
		|| PreviewFocus == EAstroPreviewFocus::HomePlanet))
	{
		SelectedPreviewBody = Generator->GetSelectedPreviewBodyActor();
		if (APlanetaryBody* RestoredBody = Cast<APlanetaryBody>(SelectedPreviewBody.Get()))
		{
			// Structural edits can remove the previously selected planet/moon. The
			// generator then selects the nearest valid fallback; hydrate from that new
			// body before the next surface slider can write the removed body's buffer.
			HydratePreviewBodyEditorBuffer(RestoredBody);
		}
	}
	else if (bGenerated)
	{
		SelectedPreviewBody.Reset();
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Preview generated=%s revision=%d focus=%d level=%d planets=%d moons=%d radius=%.0f"),
		bGenerated ? TEXT("true") : TEXT("false"), PreviewRevision, static_cast<int32>(PreviewFocus),
		GeneratedWorld ? static_cast<int32>(GeneratedWorld->AstroGenerationLevel) : -1,
		GeneratedWorld ? GeneratedWorld->PlanetsAmount : 0, GeneratedWorld ? GeneratedWorld->MoonsAmount : 0,
		GeneratedWorld ? GeneratedWorld->PlanetRadius : 0.0);
	// Keep the failure text accurate for missing targets and rejected projection data.
	SetPreviewStatus(
		bGenerated ? LOCTEXT("PreviewReady", "LIVE FULL-SCALE PREVIEW") : LOCTEXT("PreviewFailed", "PREVIEW GENERATION INCOMPLETE"),
		bGenerated);
	bPreserveCameraOnNextPreview = false;
	bForceRefocusOnNextPreview = false;
}

AAstroGenerator* UWorldGenerationViewModel::FindOrCreatePreviewGenerator()
{
	if (PreviewGenerator.IsValid())
	{
		PreviewGenerator->WarmPreviewMaterialAssets();
		return PreviewGenerator.Get();
	}

	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	// A menu preview must never take ownership of a generator authored into a
	// gameplay map. Reusing the first AAstroGenerator used to let preview state
	// overwrite the legacy SinglePlay references. Only reuse an actor that this
	// view model explicitly created for preview work.
	for (TActorIterator<AAstroGenerator> It(World); It; ++It)
	{
		AAstroGenerator* ExistingGenerator = *It;
		if (IsValid(ExistingGenerator) && ExistingGenerator->ActorHasTag(TEXT("WorldGenerationPreview")))
		{
			InitializeSpawnDefaultsFromGenerator(ExistingGenerator);
			ExistingGenerator->WarmPreviewMaterialAssets();
			PreviewGenerator = ExistingGenerator;
			return ExistingGenerator;
		}
	}

	UClass* GeneratorClass = LoadClass<AAstroGenerator>(
		nullptr, TEXT("/Game/APS/APS_ALPHA/Core/BP_AstroGenerator.BP_AstroGenerator_C"));
	if (!GeneratorClass)
	{
		GeneratorClass = LoadClass<AAstroGenerator>(
			nullptr, TEXT("/Game/APS/Core/BP_AstroGenerator.BP_AstroGenerator_C"));
	}
	if (!GeneratorClass)
	{
		return nullptr;
	}

	const FTransform PreviewTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	AAstroGenerator* NewGenerator = World->SpawnActorDeferred<AAstroGenerator>(
		GeneratorClass, PreviewTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (NewGenerator)
	{
		// Prevent Blueprint defaults from running the normal gameplay BeginPlay
		// generation before the preview model has been supplied. The authored
		// integration flags belong only to the placed SinglePlay generator; carrying
		// them into this transient actor prevents it from assigning HomePlanet.
		NewGenerator->bAutoGeneration = false;
		NewGenerator->bIntegrateStartPlanet = false;
		NewGenerator->WSR_StartHomePlanet = nullptr;
		NewGenerator->Tags.AddUnique(TEXT("WorldGenerationPreview"));
		UGameplayStatics::FinishSpawningActor(NewGenerator, PreviewTransform);
		// This first bounded preview request is the page warm-up. Resolve orbital
		// materials before any later PLANET/body selection can enter globe commit.
		NewGenerator->WarmPreviewMaterialAssets();
		InitializeSpawnDefaultsFromGenerator(NewGenerator);
		PreviewGenerator = NewGenerator;
	}
	return NewGenerator;
}

void UWorldGenerationViewModel::InitializeSpawnDefaultsFromGenerator(AAstroGenerator* Generator)
{
	if (!Generator || !SpawnParameters)
	{
		return;
	}

	if (UClass* ProductionPilot = APSCivilizationPilot::LoadProductionClass())
	{
		SpawnParameters->BP_CharacterClass = ProductionPilot;
	}
	else if (!SpawnParameters->BP_CharacterClass)
	{
		SpawnParameters->BP_CharacterClass = Generator->BP_CharacterClass;
	}
	if (!SpawnParameters->BP_HomeSpaceship) SpawnParameters->BP_HomeSpaceship = Generator->BP_HomeSpaceship;
	if (!SpawnParameters->BP_HomeSpaceStation) SpawnParameters->BP_HomeSpaceStation = Generator->BP_HomeSpaceStation;
	if (!SpawnParameters->BP_HomeSpaceHeadquarters) SpawnParameters->BP_HomeSpaceHeadquarters = Generator->BP_HomeSpaceHeadquarters;
	if (!SpawnParameters->BP_HomeSpaceShipyard) SpawnParameters->BP_HomeSpaceShipyard = Generator->BP_HomeSpaceShipyard;
}

void UWorldGenerationViewModel::CommitAndOpenLevel(FName LevelName)
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World || !GeneratedWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.WorldGeneration] Commit rejected: world context or generated model is missing"));
		return;
	}
	// Flush the final selected body's debounced UI buffer before duplicating the
	// model into GameInstance. Otherwise the last slider movement exists only in
	// the menu actor and disappears during travel.
	PreserveSelectedPreviewBodyEdit(true);
	AAstroGenerator* ReadyPreviewGenerator = PreviewGenerator.Get();
	if (!bPreviewReady || !IsValid(ReadyPreviewGenerator))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Commit rejected: preview hierarchy is not ready"));
		return;
	}
	const FAPSCanonicalStellarProjectionDescriptor& Projection =
		ReadyPreviewGenerator->GetCanonicalStellarProjectionDescriptor();
	const FAPSCanonicalStellarDataset& Dataset = GeneratedWorld->CanonicalStellarDataset;
	const bool bCanonicalStatePresent = Dataset.bFinalized
		|| Projection.CanonicalDatasetHash != 0u;
	const bool bCanonicalReady = !bCanonicalStatePresent
		|| (Projection.bFinalized && Projection.bProjectionValid
			&& Projection.bMappingsComplete && Projection.bUnitRoots
			&& Projection.bBoundsValid
			&& Dataset.IsUsable(Projection.CanonicalDatasetInputHash)
			&& Dataset.DatasetHash == Projection.CanonicalDatasetHash);
	if (!bCanonicalReady)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Commit rejected: canonical preview dataset/projection is incomplete"));
		return;
	}
	if (GenerationRoute == EAPSGenerationRoute::Civilization)
	{
		if (SpawnParameters)
		{
			SpawnParameters->SanitizeForGeneration();
			if (UClass* ProductionPilot = APSCivilizationPilot::LoadProductionClass())
			{
				SpawnParameters->BP_CharacterClass = ProductionPilot;
			}
		}
		const auto IsSpawnableClass = [](const UClass* Class)
		{
			return Class && !Class->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_NotPlaceable);
		};
		const bool bClassesReady = SpawnParameters
			&& SpawnParameters->BP_CharacterClass.Get() == APSCivilizationPilot::LoadProductionClass()
			&& IsSpawnableClass(SpawnParameters->BP_CharacterClass)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceship)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceStation)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceHeadquarters)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceShipyard);
		const bool bHierarchyReady = GeneratedWorld->bGenerateHomeSystem
			&& GeneratedWorld->bStartWithHomePlanet
			&& GetHomeStartPlanetCount() > 0
			&& GeneratedWorld->StartPlanetIndex > 0
			&& GeneratedWorld->StartPlanetIndex <= GetHomeStartPlanetCount();
		if (!bClassesReady || !bHierarchyReady)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Civilization] Commit rejected: classes=%s hierarchy=%s planets=%d start=%d"),
				bClassesReady ? TEXT("OK") : TEXT("INCOMPLETE"),
				bHierarchyReady ? TEXT("OK") : TEXT("INCOMPLETE"),
				GeneratedWorld->PlanetsAmount, GeneratedWorld->StartPlanetIndex);
			return;
		}
	}

	bool bHandoffCommitted = false;
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UMainGameplayInstance* GameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			// This is a newly generated world, not a request to restore the last
			// selected save slot. Explicitly reset both modes before travel.
			GameplayInstance->bIsLoadingMode = false;
			GameplayInstance->bUseAuthoredSinglePlayWorld = false;
			GameplayInstance->bSpawnGeneratedCivilization =
				GenerationRoute == EAPSGenerationRoute::Civilization;
			GameplayInstance->SaveSlotName.Reset();
			// The editable model is owned by the menu controller. Duplicate it into
			// the GameInstance subsystem before travel so it survives destruction of
			// the menu world and remains the exact model consumed by gameplay.
			GameplayInstance->NewGeneratedWorld = DuplicateObject<UGeneratedWorld>(GeneratedWorld, GameplayInstance);
			if (!GameplayInstance->NewGeneratedWorld)
			{
				UE_LOG(LogTemp, Error, TEXT("[APS.WorldGeneration] Commit rejected: failed to persist generated model"));
				return;
			}
			if (GenerationRoute == EAPSGenerationRoute::Civilization
				&& !GameplayInstance->CurrentCivilization)
			{
				GameplayInstance->CurrentCivilization = NewObject<UCivilization>(GameplayInstance);
			}
			if (GenerationRoute == EAPSGenerationRoute::Civilization
				&& !GameplayInstance->CurrentCivilization)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.Civilization] Commit rejected: failed to create runtime civilization"));
				GameplayInstance->NewGeneratedWorld = nullptr;
				GameplayInstance->bSpawnGeneratedCivilization = false;
				return;
			}
			if (GenerationRoute == EAPSGenerationRoute::Civilization
				&& GameplayInstance->CurrentCivilization && SpawnParameters)
			{
				// Freeze an independent GameInstance-owned snapshot across OpenLevel,
				// just like the generated astronomical model. The editable menu model
				// must not keep mutating the committed gameplay session during travel.
				GameplayInstance->SpawnParameters = DuplicateObject<USpawnParameters>(
					SpawnParameters, GameplayInstance);
				if (!GameplayInstance->SpawnParameters)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[APS.Civilization] Commit rejected: failed to persist spawn parameters"));
					GameplayInstance->CurrentCivilization = nullptr;
					GameplayInstance->bSpawnGeneratedCivilization = false;
					return;
				}
				GameplayInstance->CurrentCivilization->InitializeFromSpawnParameters(
					GameplayInstance->SpawnParameters);
				UE_LOG(LogTemp, Log, TEXT("[APS.Civilization] Committed '%s': population=%d tech=%d fleet=%d"),
					*GameplayInstance->CurrentCivilization->Name,
					GameplayInstance->CurrentCivilization->Population,
					GameplayInstance->CurrentCivilization->TechnologyLevel,
					GameplayInstance->CurrentCivilization->FleetSize);
			}
			else
			{
				// Space/planet creation commits the astronomical model only. Do not
				// leak an earlier civilization or its infrastructure into this route.
				GameplayInstance->CurrentCivilization = nullptr;
				GameplayInstance->SpawnParameters = nullptr;
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Committed route=%d without civilization spawn data"),
					static_cast<int32>(GenerationRoute));
			}
			bHandoffCommitted = true;
		}
	}
	if (!bHandoffCommitted)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Commit rejected: MainGameplayInstance subsystem is unavailable"));
		return;
	}

	// L_WorldGeneration already authors BP_GravityGameModeBase, whose native parent
	// performs the generated-model and selected-pawn handoff. The former URL option
	// forced an unrelated Engine.GameModeBase asset and bypassed that entire path.
	PendingTravelLevelName = LevelName;
	PreviewTravelDrainAttempts = 0;
	SetPreviewStatus(LOCTEXT("PreviewTravelDrain", "PREPARING GENERATED WORLD"), false);
	TryOpenCommittedLevelAfterPreviewDrain();
}

void UWorldGenerationViewModel::TryOpenCommittedLevelAfterPreviewDrain()
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World || PendingTravelLevelName.IsNone())
	{
		return;
	}
	if (AAstroGenerator* Generator = PreviewGenerator.Get();
		Generator && !Generator->PreparePreviewForTravel())
	{
		constexpr int32 MaxDrainAttempts = 400; // 20 seconds at 20 Hz.
		if (++PreviewTravelDrainAttempts >= MaxDrainAttempts)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldGeneration] Travel cancelled: preview WorldScape workers did not drain safely"));
			PendingTravelLevelName = NAME_None;
			SetPreviewStatus(LOCTEXT("PreviewTravelDrainFailed", "WORLDSCAPE STILL BUSY - TRY AGAIN"), true);
			return;
		}
		World->GetTimerManager().SetTimer(
			PreviewTravelTimerHandle, this,
			&UWorldGenerationViewModel::TryOpenCommittedLevelAfterPreviewDrain, 0.05f, false);
		return;
	}

	const FName LevelToOpen = PendingTravelLevelName;
	PendingTravelLevelName = NAME_None;
	UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Opening %s with its authored gravity GameMode"),
		*LevelToOpen.ToString());
	UGameplayStatics::OpenLevel(World, LevelToOpen, true);
}

void UWorldGenerationViewModel::SetPreviewStatus(const FText& Status, bool bReady)
{
	UE_MVVM_SET_PROPERTY_VALUE(PreviewStatus, Status);
	UE_MVVM_SET_PROPERTY_VALUE(bPreviewReady, bReady);
}

#undef LOCTEXT_NAMESPACE
