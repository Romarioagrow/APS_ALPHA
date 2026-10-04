#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"

/**
 * Rio 03.10: galaxy subclasses per type ("the subclasses are the same for every galaxy type;
 * distribute them logically and make the subclass actually change the result").
 *
 * Contract (docs: Docs/Design/GALAXY_GENERATION_V2.md):
 * - Every (type, class) pair that existed before 03.10 has NO parametric profile and keeps
 *   resolving through the historic FGalaxyCatalogDescriptor::ResolveStar code, so saved worlds
 *   re-resolve identical galaxies.
 * - Each type offers its own subclass list (GetSubclasses). The type default reproduces the
 *   look that type had before: E0, S0, Irr are historic values; Sb, SBb and the warped
 *   peculiar disk are new values routed to the historic two-arm / warp code (Form Historic).
 * - Every other new subclass is a parametric profile sampled here: a pure function of
 *   (seed, catalogue index), thread-safe, O(1) per star, no global RNG.
 */
namespace APSGalaxyMorphology
{
	/** How a subclass places its stars. Historic routes to the pre-03.10 code. */
	enum class EForm : uint8
	{
		Historic,
		Disk,
		Spheroid,
		Irregular,
		Starburst,
		Ring,
		Interacting,
		TidalTails,
		PolarRing
	};

	/**
	 * Shape parameters of one subclass. Lengths are fractions of the catalogue radius.
	 * Population shares are consumed in declaration order by the historic population
	 * selector of the record hash (halo, bulge, bar, stub), the rest is the main body.
	 */
	struct FProfile
	{
		EGalaxyType Type{EGalaxyType::Unknown};
		EGalaxyClass Class{EGalaxyClass::Unknown};
		EForm Form{EForm::Historic};
		/** Historic: the pre-03.10 class whose output this subclass reproduces exactly. */
		EGalaxyClass HistoricClass{EGalaxyClass::Unknown};

		double Halo{0.0};
		double Bulge{0.0};
		double Bar{0.0};
		double Stub{0.0};

		double HaloExtent{0.96};
		double HaloFlattening{0.85};
		double BulgeRadius{0.1};
		double BulgeExponent{0.5};
		double BulgeFlattening{0.8};
		double BarLength{0.3};
		double BarWidth{0.06};
		double BarAngle{0.0};
		double OffsetX{0.0};
		double OffsetY{0.0};

		// Disk: radius = Extent * U^Exponent with a soft rim; Archimedean arms as the historic spiral.
		double DiskExtent{1.0};
		double DiskExponent{0.5};
		double DiskThickness{0.025};
		double ThickShare{0.15};
		double ThickScale{3.0};
		int32 Arms{0};
		double ArmStart{0.0};
		double ArmTurns{1.0};
		double ArmJitter{0.2};
		double ArmShare{1.0};
		/** Share of arm stars forced onto arm 0 (Magellanic one-armed asymmetry). */
		double ArmLead{0.0};
		/** Share of arm stars gathered into star-forming knots. */
		double ArmClumps{0.0};
		int32 KnotsPerArm{10};
		double InnerRing{0.0};

		// Spheroid: core + envelope, then axis ratios.
		double Core{0.0};
		double CoreRadius{0.1};
		double CoreExponent{1.0};
		double EnvelopeInner{0.0};
		double EnvelopeOuter{0.8};
		double EnvelopeExponent{0.6};
		double AxisY{1.0};
		double AxisZ{1.0};
		double Nucleus{0.0};

		// Irregular: soft lobes, an optional bar and one stubby arm.
		int32 Lobes{3};
		double LobeSpread{0.3};
		double LobeRadiusMin{0.15};
		double LobeRadiusMax{0.28};
		double StubTurns{0.3};
		double StubReach{0.35};
		double Clumps{0.0};

		/** Population age of the main body: -1 old (red) ... +1 young (hot). */
		double Age{0.0};

		bool IsHistoric() const { return Form == EForm::Historic; }
	};

	struct FSample
	{
		/** Galaxy-local catalogue units (already multiplied by the catalogue radius). */
		FVector Position{FVector::ZeroVector};
		/** -1 old ... +1 young; folded into the spectral class by ApplyPopulationAge. */
		float Age{0.0f};
	};

	/** Subclasses the menu offers for a type, in Hubble order. Empty for Unknown. */
	APS_ALPHA_API TConstArrayView<EGalaxyClass> GetSubclasses(EGalaxyType Type);
	/** The subclass that reproduces the type's pre-03.10 look. */
	APS_ALPHA_API EGalaxyClass GetDefaultSubclass(EGalaxyType Type);
	APS_ALPHA_API bool IsSubclassOf(EGalaxyType Type, EGalaxyClass Class);
	/** Keeps a class offered for the type, otherwise returns the type default (menu type change). */
	APS_ALPHA_API EGalaxyClass CoerceSubclass(EGalaxyType Type, EGalaxyClass Class);
	/** Parametric or historic-routing profile; nullptr: resolve with the historic code as is. */
	APS_ALPHA_API const FProfile* FindProfile(EGalaxyType Type, EGalaxyClass Class);
	/** All profiles (tests, documentation). */
	APS_ALPHA_API TConstArrayView<FProfile> GetProfiles();
	/** One star of a parametric profile. RecordHash supplies the historic population selector. */
	APS_ALPHA_API FSample Sample(const FProfile& Profile, int32 Seed, int64 CatalogIndex,
		uint64 RecordHash, double GalaxyRadius);
	/** Young bodies gain hot O/B/A stars, old bodies lose them. Uses its own stream. */
	APS_ALPHA_API ESpectralClass ApplyPopulationAge(ESpectralClass Base, float Age, uint64 RecordHash);
	/** One-line English summary for menu tooltips. */
	APS_ALPHA_API const TCHAR* GetSubclassSummary(EGalaxyClass Class);

	/** Historic render budgets: the accepted look is defined at these counts. */
	inline constexpr int32 PreviewReferenceBudget = 1800;
	inline constexpr int32 GameplayReferenceBudget = 25000;
	/**
	 * STARS slider ceiling for the current renderer (Rio 03.10). The menu's continuous frame re-projects every galaxy
	 * point on the CPU while the camera moves (~0.28 us per point measured), so 50k costs ~10-14 ms per moving frame;
	 * static frames are free. The design target of 1,000,000 needs the GPU star layer (GALAXY_GENERATION_V2.md).
	 */
	inline constexpr int32 MaxPlacedStars = 50000;
	/**
	 * Smallest GALAXY SIZE the menu offers (Rio 03.10: "SIZE 1: the galaxy as if collapsed"). Positions scale with SIZE
	 * (x 50,000 units of 1e9 cm), star radii are physical: at SIZE 1 the whole galaxy is ~720 solar radii across its
	 * radius, so stars and the 16%-scaled home cluster pile into one another. At 50 the mean spacing of 50k disk stars
	 * is still ~800 solar radii. Old saves with smaller sizes still load unchanged; only the input is clamped.
	 */
	inline constexpr int32 MinGalaxySize = 50;

	/**
	 * Rio 03.10 (up to 1M placed stars, "more volumetric"): above the reference budget each
	 * star gets smaller and dimmer so the total light grows only as N^0.2 instead of N.
	 * Identity at or below the reference, so historic budgets render exactly as before.
	 */
	struct FDensityCompensation
	{
		double EmissionScale{1.0};
		double RadiusScale{1.0};
	};
	APS_ALPHA_API FDensityCompensation GetDensityCompensation(int64 RenderedCount, int64 ReferenceCount);

	/**
	 * Rio 03.10 ("the galaxy's star sizes and spectral classes, the same as for the cluster"): the cluster's
	 * POPULATION (EStarClusterPopulation) and COMPOSITION (EStarClusterComposition) presets applied to one galaxy
	 * record, from a stream of its own, after every historic draw. Population 0 (All Sequences) and composition 0
	 * (All Spectral) change nothing, so older saves resolve identically; aps.Galaxy.StarMix 0 turns the mix off.
	 * Population sets a size factor per drawn stellar type (giants larger, dwarfs smaller; exotic types also take
	 * their own class), composition re-draws the spectral class from the cluster's colour tables.
	 */
	APS_ALPHA_API void ApplyStarMix(uint8 Population, uint8 Composition, uint64 RecordHash,
		ESpectralClass& InOutSpectralClass, float& OutRadiusScale);
	/** Luminosity factor of a size factor at the class temperature (R^2), bounded for the visual pipeline. */
	inline double GetRadiusScaleLuminosity(const float RadiusScale)
	{
		return FMath::Clamp(static_cast<double>(RadiusScale) * RadiusScale, 1.0e-4, 1.0e4);
	}
}
