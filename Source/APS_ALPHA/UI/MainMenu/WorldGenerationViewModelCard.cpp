#include "WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#include "APSGenerationModelCard.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"

#define LOCTEXT_NAMESPACE "WorldGenerationViewModelCard"

namespace APSModelCardPrivate
{
	constexpr double SolarRadiusKm = 695700.0;
	constexpr double EarthRadiusKm = 6371.0;

	FText Number(const double Value, const int32 MaxFraction = 0)
	{
		FNumberFormattingOptions Options;
		Options.SetUseGrouping(true).SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(MaxFraction);
		return APSUINumber::Number(Value, &Options);
	}

	/** About three significant digits: 1.02, 46.6, 1,234. */
	FText Readable(const double Value)
	{
		const double Magnitude = FMath::Abs(Value);
		return Number(Value, Magnitude < 10.0 ? 2 : Magnitude < 100.0 ? 1 : 0);
	}

	/** Counts in the billions and millions read as 1.2 BILLION; smaller ones in full. */
	void Count(const double Value, FText& OutValue, FText& OutUnit, const FText& Noun)
	{
		if (Value >= 1.0e9)
		{
			OutValue = Number(Value / 1.0e9, 1);
			OutUnit = FText::Format(LOCTEXT("Billion", "BILLION {0}"), Noun);
		}
		else if (Value >= 1.0e6)
		{
			OutValue = Number(Value / 1.0e6, 1);
			OutUnit = FText::Format(LOCTEXT("Million", "MILLION {0}"), Noun);
		}
		else
		{
			OutValue = Number(Value);
			OutUnit = Noun;
		}
	}

	template <typename TEnum>
	FText Enum(const TEnum Value)
	{
		return FText::FromString(UEnum::GetDisplayValueAsText(Value).ToString().ToUpper());
	}

	/** "ROCKY", "FROZEN", "GAS GIANT": the " Planet" word does not fit a moon and repeats the card's kind. */
	FText TypeName(const EPlanetType Type)
	{
		FString Name = UEnum::GetDisplayValueAsText(Type).ToString().ToUpper();
		Name.RemoveFromEnd(TEXT(" PLANET"));
		return FText::FromString(Name);
	}

	FAPSModelFact& Add(FAPSModelCard& Card, const EAPSModelGlyph Glyph, const FText& Label, const FText& Value,
		const FText& Unit = FText::GetEmpty(), const FText& Note = FText::GetEmpty(), const bool bAccent = false)
	{
		FAPSModelFact& Fact = Card.Facts.AddDefaulted_GetRef();
		Fact.Glyph = Glyph;
		Fact.Label = Label;
		Fact.Value = Value;
		Fact.Unit = Unit;
		Fact.Note = Note;
		Fact.bAccent = bAccent;
		return Fact;
	}

	FText Upper(const FName Name, const FText& Fallback)
	{
		return Name.IsNone() ? Fallback : FText::FromString(Name.ToString().ToUpper());
	}

	EStarType SystemType(const int32 Stars)
	{
		return Stars <= 1 ? EStarType::SingleStar : Stars == 2 ? EStarType::DoubleStar
			: Stars == 3 ? EStarType::TripleStar : EStarType::MultipleStar;
	}

	int32 PlanetCount(const AStarSystem* System)
	{
		int32 Count = 0;
		if (IsValid(System))
		{
			for (const AStar* Star : System->GetStars())
			{
				Count += IsValid(Star) && IsValid(Star->PlanetarySystem) ? Star->PlanetarySystem->PlanetsActorsList.Num() : 0;
			}
		}
		return Count;
	}
}

void UWorldGenerationViewModel::GetPreviewModelCard(FAPSModelCard& OutCard) const
{
	using namespace APSModelCardPrivate;
	OutCard = FAPSModelCard();
	if (!GeneratedWorld)
	{
		OutCard.Kind = LOCTEXT("NoModelKind", "MODEL");
		OutCard.Title = LOCTEXT("NoModel", "NO MODEL");
		return;
	}
	const AAstroGenerator* Generator = PreviewGenerator.Get();
	const FText GalaxyName = FText::FromString(GeneratedWorld->GetGalaxyName().ToUpper());
	const FText ClusterName = FText::FromString(GeneratedWorld->GetClusterName().ToUpper());

	switch (PreviewFocus)
	{
	case EAstroPreviewFocus::Galaxy:
	{
		OutCard.Glyph = EAPSModelGlyph::Galaxy;
		OutCard.Kind = LOCTEXT("GalaxyKind", "GALAXY");
		OutCard.Title = GalaxyName;
		OutCard.Subtitle = FText::Format(LOCTEXT("GalaxySubtitle", "{0}  /  {1}"),
			Enum(GeneratedWorld->GalaxyType), Enum(GeneratedWorld->GalaxyClass));
		FText Value, Unit;
		Count(static_cast<double>(Generator ? Generator->GetPreviewGalaxyModeledStarCount()
			: static_cast<int64>(GeneratedWorld->GalaxyStarCount)), Value, Unit, LOCTEXT("StarsNoun", "STARS"));
		Add(OutCard, EAPSModelGlyph::Star, LOCTEXT("ModeledStars", "MODELED"), Value, Unit, FText::GetEmpty(), true);
		Add(OutCard, EAPSModelGlyph::Count, LOCTEXT("LiveSample", "LIVE SAMPLE"),
			Number(Generator ? Generator->GetPreviewGalaxyRenderedStarCount() : 0), LOCTEXT("StarsUnit", "STARS"));
		Add(OutCard, EAPSModelGlyph::Galaxy, LOCTEXT("GalaxyType", "TYPE"), Enum(GeneratedWorld->GalaxyType));
		Add(OutCard, EAPSModelGlyph::Type, LOCTEXT("GalaxyClass", "CLASS"), Enum(GeneratedWorld->GalaxyClass));
		Add(OutCard, EAPSModelGlyph::Scale, LOCTEXT("GalaxySize", "SIZE"), Number(GeneratedWorld->GalaxySize));
		Add(OutCard, EAPSModelGlyph::Cluster, LOCTEXT("GalaxyDensity", "DENSITY"), Number(GeneratedWorld->GalaxyStarDensity, 2));
		return;
	}

	case EAstroPreviewFocus::StarCluster:
		OutCard.Glyph = EAPSModelGlyph::Cluster;
		OutCard.Kind = LOCTEXT("ClusterKind", "STAR CLUSTER");
		OutCard.Title = ClusterName;
		OutCard.Subtitle = FText::Format(LOCTEXT("ClusterSubtitle", "IN {0}"), GalaxyName);
		Add(OutCard, EAPSModelGlyph::System, LOCTEXT("ClusterSystems", "SYSTEMS"),
			Number(Generator ? Generator->GetPreviewClusterModeledSystemCount() : 0), LOCTEXT("ModeledUnit", "MODELED"),
			FText::GetEmpty(), true);
		Add(OutCard, EAPSModelGlyph::Star, LOCTEXT("ClusterRendered", "LIVE STARS"),
			Number(Generator ? Generator->GetPreviewClusterRenderedStarCount() : 0));
		Add(OutCard, EAPSModelGlyph::Cluster, LOCTEXT("ClusterFormation", "FORMATION"), Enum(GeneratedWorld->StarClusterType));
		Add(OutCard, EAPSModelGlyph::Scale, LOCTEXT("ClusterSize", "SIZE"), Enum(GeneratedWorld->StarClusterSize));
		Add(OutCard, EAPSModelGlyph::Count, LOCTEXT("ClusterPopulation", "POPULATION"), Enum(GeneratedWorld->StarClusterPopulation));
		Add(OutCard, EAPSModelGlyph::Spectrum, LOCTEXT("ClusterComposition", "COMPOSITION"),
			Enum(GeneratedWorld->StarClusterComposition));
		return;

	case EAstroPreviewFocus::HomeSystem:
	{
		OutCard.Glyph = EAPSModelGlyph::System;
		OutCard.Kind = LOCTEXT("SystemKind", "STAR SYSTEM");
		const AStarSystem* LiveSystem = Generator ? Generator->GetContinuousPreviewActiveSystem() : nullptr;
		FString RecordId;
		int32 RecordStars = 0;
		int32 RecordPlanets = 0;
		// A selected cluster record without a live hierarchy is shown as that record, never as the home system.
		const bool bRecord = !IsValid(LiveSystem) && Generator
			&& Generator->GetSelectedPreviewClusterSystemSummary(RecordId, RecordStars, RecordPlanets);
		if (!IsValid(LiveSystem) && !bRecord && Generator) LiveSystem = Generator->GetPreviewHomeSystem();
		const bool bHome = IsValid(LiveSystem)
			&& (LiveSystem == (Generator ? Generator->GetPreviewHomeSystem() : nullptr)
				|| LiveSystem->StableSystemId == GeneratedWorld->CanonicalStellarDataset.HomeStableId);
		const AStar* MainStar = IsValid(LiveSystem) ? LiveSystem->MainStar : nullptr;
		OutCard.Title = IsValid(MainStar) ? Upper(MainStar->AstroName, LOCTEXT("SystemTitle", "STAR SYSTEM"))
			: bRecord ? FText::FromString(RecordId.ToUpper()) : LOCTEXT("HomeSystemTitle", "HOME SYSTEM");
		OutCard.Subtitle = !bPreviewReady ? LOCTEXT("SystemUpdating", "UPDATING THE SELECTED SYSTEM...")
			: bHome ? LOCTEXT("HomeSystemSubtitle", "HOME SYSTEM  /  LIVE HIERARCHY")
			: bRecord ? LOCTEXT("RecordSubtitle", "CLUSTER RECORD  /  FULL-SCALE DATA")
			: LOCTEXT("ClusterSystemSubtitle", "CLUSTER SYSTEM  /  LIVE HIERARCHY");
		const int32 Stars = IsValid(LiveSystem) ? LiveSystem->GetStars().Num() : RecordStars;
		const int32 Planets = IsValid(LiveSystem) ? PlanetCount(LiveSystem) : RecordPlanets;
		Add(OutCard, EAPSModelGlyph::Star, LOCTEXT("SystemStars", "STARS"), Number(Stars), Enum(SystemType(Stars)));
		Add(OutCard, EAPSModelGlyph::Planet, LOCTEXT("SystemPlanets", "PLANETS"), Number(Planets),
			bRecord ? LOCTEXT("PotentialUnit", "POTENTIAL") : FText::GetEmpty(), FText::GetEmpty(), true);
		const APlanetarySystem* Family = IsValid(MainStar) ? MainStar->PlanetarySystem : nullptr;
		Add(OutCard, EAPSModelGlyph::Orbit, LOCTEXT("SystemOrbits", "ORBITS"),
			IsValid(Family) ? Enum(Family->OrbitDistributionType) : Enum(GeneratedWorld->OrbitDistributionType));
		if (bHome)
		{
			Add(OutCard, EAPSModelGlyph::Home, LOCTEXT("SystemStart", "START PLANET"),
				Number(GeneratedWorld->StartPlanetIndex));
		}
		return;
	}

	case EAstroPreviewFocus::HomeStar:
	{
		OutCard.Glyph = EAPSModelGlyph::Star;
		OutCard.Kind = LOCTEXT("StarKind", "STAR");
		const AStar* Star = Generator ? Cast<AStar>(Generator->GetSelectedPreviewBodyActor()) : nullptr;
		if (!IsValid(Star))
		{
			OutCard.Title = FText::FromName(GeneratedWorld->HomeStarName);
			Add(OutCard, EAPSModelGlyph::Type, LOCTEXT("StarClassRecipe", "CLASS"), Enum(GeneratedWorld->StellarType));
			Add(OutCard, EAPSModelGlyph::Spectrum, LOCTEXT("StarSpectrumRecipe", "SPECTRUM"), Enum(GeneratedWorld->SpectralClass),
				FText::GetEmpty(), FText::GetEmpty(), true);
			return;
		}
		OutCard.Designation = FText::FromString(APSBodyDesignation::Of(Star));
		OutCard.Title = Upper(Star->AstroName, LOCTEXT("StarTitle", "STAR"));
		OutCard.Subtitle = Star->FullSpectralName.IsNone() ? Enum(Star->StellarClass)
			: FText::FromString(Star->FullSpectralName.ToString().ToUpper());
		Add(OutCard, EAPSModelGlyph::Spectrum, LOCTEXT("StarSpectrum", "SPECTRUM"),
			Star->FullSpectralClass.IsNone() ? Enum(Star->SpectralClass) : FText::FromName(Star->FullSpectralClass),
			FText::GetEmpty(), FText::GetEmpty(), true);
		Add(OutCard, EAPSModelGlyph::Type, LOCTEXT("StarClass", "CLASS"), Enum(Star->StellarClass));
		const double RadiusKm = Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM);
		Add(OutCard, EAPSModelGlyph::Radius, LOCTEXT("StarRadius", "RADIUS"), Readable(RadiusKm / SolarRadiusKm),
			LOCTEXT("SolarRadii", "R SUN"), FText::Format(LOCTEXT("StarRadiusKm", "{0} KM"), Number(RadiusKm)));
		if (Star->SurfaceTemperature > 0)
		{
			Add(OutCard, EAPSModelGlyph::Temperature, LOCTEXT("StarTemperature", "SURFACE"),
				Number(Star->SurfaceTemperature), LOCTEXT("Kelvin", "K"));
		}
		if (Star->Luminosity > 0.0f)
		{
			Add(OutCard, EAPSModelGlyph::Light, LOCTEXT("StarLuminosity", "LUMINOSITY"), Readable(Star->Luminosity),
				LOCTEXT("SolarLuminosity", "L SUN"));
		}
		if (Star->Mass > 0.0)
		{
			Add(OutCard, EAPSModelGlyph::Mass, LOCTEXT("StarMass", "MASS"), Readable(Star->Mass), LOCTEXT("SolarMass", "M SUN"));
		}
		Add(OutCard, EAPSModelGlyph::Planet, LOCTEXT("StarPlanets", "PLANETS"),
			Number(IsValid(Star->PlanetarySystem) ? Star->PlanetarySystem->PlanetsActorsList.Num() : 0));
		return;
	}

	case EAstroPreviewFocus::HomePlanet:
	{
		const APlanetaryBody* Body = Cast<APlanetaryBody>(SelectedPreviewBody.Get());
		const AMoon* Moon = Cast<AMoon>(Body);
		const APlanet* Planet = Cast<APlanet>(Body);
		OutCard.Glyph = Moon ? EAPSModelGlyph::Moon : EAPSModelGlyph::Planet;
		OutCard.Kind = Moon ? LOCTEXT("MoonKind", "MOON") : LOCTEXT("PlanetKind", "PLANET");
		if (!IsValid(Body))
		{
			OutCard.Title = FText::FromName(GeneratedWorld->HomePlanetName);
			Add(OutCard, EAPSModelGlyph::Surface, LOCTEXT("PlanetTypeRecipe", "TYPE"), TypeName(GeneratedWorld->PlanetType));
			Add(OutCard, EAPSModelGlyph::Radius, LOCTEXT("PlanetRadiusRecipe", "RADIUS"), Number(GeneratedWorld->PlanetRadius),
				LOCTEXT("KmUnitRecipe", "KM"));
			return;
		}
		OutCard.Designation = FText::FromString(APSBodyDesignation::Of(Body));
		OutCard.Title = Upper(Body->AstroName, OutCard.Kind);
		OutCard.Subtitle = FText::Format(LOCTEXT("PlanetSubtitle", "{0}  /  {1}"),
			TypeName(Body->PlanetType), Enum(Body->PlanetHabitability));
		Add(OutCard, EAPSModelGlyph::Surface, LOCTEXT("PlanetType", "TYPE"), TypeName(Body->PlanetType));
		Add(OutCard, EAPSModelGlyph::Life, LOCTEXT("PlanetHabitability", "HABITABILITY"), Enum(Body->PlanetHabitability),
			FText::GetEmpty(), FText::GetEmpty(), Body->PlanetHabitability != EPlanetHabitability::Uninhabitable);
		Add(OutCard, EAPSModelGlyph::Radius, LOCTEXT("PlanetRadius", "RADIUS"), Number(Body->PlanetRadiusKM),
			LOCTEXT("KmUnit", "KM"), FText::Format(LOCTEXT("EarthRadii", "{0} EARTH RADII"),
				Readable(Body->PlanetRadiusKM / EarthRadiusKm)));
		if (Planet && CanEditSelectedPlanetOrbit())
		{
			Add(OutCard, EAPSModelGlyph::Orbit, LOCTEXT("PlanetOrbit", "ORBIT"), Readable(GetSelectedPlanetOrbitDistanceAu()),
				LOCTEXT("AuUnit", "AU"));
		}
		if (Planet)
		{
			Add(OutCard, EAPSModelGlyph::Moon, LOCTEXT("PlanetMoons", "MOONS"), Number(Planet->Moons.Num()));
		}
		else if (Moon && IsValid(Moon->ParentPlanet))
		{
			Add(OutCard, EAPSModelGlyph::Planet, LOCTEXT("MoonParent", "ORBITS"),
				Upper(Moon->ParentPlanet->AstroName, LOCTEXT("MoonParentFallback", "PLANET")),
				FText::FromString(APSBodyDesignation::Of(Moon->ParentPlanet)));
		}
		Add(OutCard, EAPSModelGlyph::Atmosphere, LOCTEXT("PlanetAtmosphere", "ATMOSPHERE"),
			Body->AtmosphereHeight > 0.0 ? Number(Body->AtmosphereHeight) : LOCTEXT("NoAtmosphere", "NONE"),
			Body->AtmosphereHeight > 0.0 ? LOCTEXT("AtmosphereUnit", "KM HIGH") : FText::GetEmpty());
		const APlanetaryBody* SurfaceBody = Generator ? Generator->GetActivePreviewWorldScapeBody() : nullptr;
		if (SurfaceBody == Body)
		{
			Add(OutCard, EAPSModelGlyph::Scale, LOCTEXT("PlanetSurface", "SURFACE"),
				Body->bWorldScapeSurfaceReady ? LOCTEXT("SurfaceReady", "READY") : LOCTEXT("SurfaceGenerating", "GENERATING"));
		}
		return;
	}

	case EAstroPreviewFocus::Overview:
	default:
	{
		OutCard.Glyph = EAPSModelGlyph::Galaxy;
		OutCard.Kind = LOCTEXT("WorldKind", "WORLD");
		OutCard.Title = GalaxyName;
		OutCard.Subtitle = FText::Format(LOCTEXT("WorldSubtitle", "SEED {0}"), APSUINumber::Number(GeneratedWorld->GenerationSeed,
			&FNumberFormattingOptions::DefaultNoGrouping()));
		FText Value, Unit;
		Count(GeneratedWorld->GalaxyStarCount, Value, Unit, LOCTEXT("OverviewStarsNoun", "STARS"));
		Add(OutCard, EAPSModelGlyph::Galaxy, LOCTEXT("OverviewGalaxy", "GALAXY"), Value, Unit);
		Add(OutCard, EAPSModelGlyph::Cluster, LOCTEXT("OverviewCluster", "CLUSTER"), ClusterName,
			Enum(GeneratedWorld->StarClusterSize));
		Add(OutCard, EAPSModelGlyph::System, LOCTEXT("OverviewHome", "HOME PLANETS"),
			Number(Generator ? PlanetCount(Generator->GetPreviewHomeSystem()) : GeneratedWorld->PlanetsAmount));
		Add(OutCard, EAPSModelGlyph::Home, LOCTEXT("OverviewStart", "START PLANET"), Number(GeneratedWorld->StartPlanetIndex),
			FText::GetEmpty(), FText::GetEmpty(), true);
		Add(OutCard, EAPSModelGlyph::Scale, LOCTEXT("OverviewScale", "FULL SCALE"),
			GeneratedWorld->bGenerateFullScaledWorld ? LOCTEXT("On", "ON") : LOCTEXT("Off", "OFF"));
		return;
	}
	}
}

FString UWorldGenerationViewModel::GetCurrentScopeNameKey() const
{
	switch (PreviewFocus)
	{
	case EAstroPreviewFocus::Galaxy: return UGeneratedWorld::GalaxyNameKey();
	case EAstroPreviewFocus::StarCluster: return UGeneratedWorld::ClusterNameKey();
	default: return GetPreviewObjectStableKey(SelectedPreviewBody.Get());
	}
}

bool UWorldGenerationViewModel::CanRenameCurrentScope() const
{
	return PreviewFocus == EAstroPreviewFocus::Galaxy || PreviewFocus == EAstroPreviewFocus::StarCluster
		? GeneratedWorld != nullptr : CanRenameSelectedPreviewBody();
}

FText UWorldGenerationViewModel::GetCurrentScopeName() const
{
	if (!GeneratedWorld) return FText::GetEmpty();
	switch (PreviewFocus)
	{
	case EAstroPreviewFocus::Galaxy: return FText::FromString(GeneratedWorld->GetGalaxyName().ToUpper());
	case EAstroPreviewFocus::StarCluster: return FText::FromString(GeneratedWorld->GetClusterName().ToUpper());
	default: return FText::FromString(GetSelectedPreviewBodyName().ToString().ToUpper());
	}
}

FText UWorldGenerationViewModel::GetCurrentScopeNameTitle() const
{
	switch (PreviewFocus)
	{
	case EAstroPreviewFocus::Galaxy: return LOCTEXT("GalaxyNameTitle", "GALAXY NAME");
	case EAstroPreviewFocus::StarCluster: return LOCTEXT("ClusterNameTitle", "CLUSTER NAME");
	default:
		if (SelectedPreviewBody.IsValid() && SelectedPreviewBody->IsA<AStar>()) return LOCTEXT("StarNameTitle", "STAR NAME");
		if (SelectedPreviewBody.IsValid() && SelectedPreviewBody->IsA<AMoon>()) return LOCTEXT("MoonNameTitle", "MOON NAME");
		return LOCTEXT("PlanetNameTitle", "PLANET NAME");
	}
}

bool UWorldGenerationViewModel::SetCurrentScopeName(const FText& Name, const FString& ExpectedKey)
{
	if (ExpectedKey.IsEmpty() || ExpectedKey != GetCurrentScopeNameKey()) return false;
	if (PreviewFocus != EAstroPreviewFocus::Galaxy && PreviewFocus != EAstroPreviewFocus::StarCluster)
	{
		return SetSelectedPreviewBodyName(Name, ExpectedKey);
	}
	// Galaxy and cluster names are labels only: no regeneration, the same validation as body names.
	if (!GeneratedWorld || !GeneratedWorld->SetPreviewDisplayNameOverride(ExpectedKey, Name.ToString())) return false;
	UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
	return true;
}

#undef LOCTEXT_NAMESPACE
