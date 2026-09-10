#include "APSWorldScapePlanetNoise.h"

#include "WorldScapeCommon/Public/NoiseMathUtils.h"

#include <cmath>

namespace APSPlanetNoise
{
	constexpr double ContinentalSigma = 0.105;
	constexpr double SqrtTwo = 1.4142135623730950488;
	constexpr double MaterialCoastHeight = 0.08;
	constexpr double MaterialLandHeightRange = 0.78;

	double UniformizeContinentalField(double Continental01)
	{
		// WorldScape's octave-normalized Simplex fBm is centred close to 0.5, not
		// uniformly distributed across 0..1. Mapping its empirical Gaussian-like
		// distribution through a monotonic CDF makes 1-LandCoverage a meaningful
		// area quantile instead of a raw-noise threshold.
		const double Z = (FMath::Clamp(Continental01, 0.0, 1.0) - 0.5)
			/ (ContinentalSigma * SqrtTwo);
		return FMath::Clamp(0.5 * (1.0 + std::erf(Z)), 0.0, 1.0);
	}

	DVector SafePlanetDirection(const DVector& PlanetVector)
	{
		const double Length = FMath::Sqrt(
			PlanetVector.X * PlanetVector.X
			+ PlanetVector.Y * PlanetVector.Y
			+ PlanetVector.Z * PlanetVector.Z);
		return Length > UE_DOUBLE_SMALL_NUMBER
			? PlanetVector / Length
			: DVector(0.0, 0.0, 1.0);
	}

	DVector MakePangeaAxis(int32 Seed)
	{
		const double Phase = static_cast<double>(Seed % 104729) * 0.017453292519943295;
		return SafePlanetDirection(DVector(
			FMath::Sin(Phase * 0.731 + 0.37),
			FMath::Cos(Phase * 1.137 + 1.11),
			FMath::Sin(Phase * 0.493 + 2.03)));
	}

	double NormalizeHeightForMaterial(
		double PhysicalHeightNormalized, const FAPSResolvedPlanetSurfaceProfile& Profile)
	{
		// WorldScape writes HeightNormalize into the red vertex-colour channel and
		// its terrain materials explicitly expect a 0..1 classification value. It
		// must not be the signed displacement used by the mesh. Normalize the
		// authored above-sea span per profile so low-land Ocean worlds still reach
		// the same material layers as dry worlds instead of rendering as an almost
		// uniform black bottom layer.
		// Do not pretend that a sparse-island Ocean body owns at least 25% land.
		// Doing so widens the display span far beyond the height actually available
		// above sea level and compresses every surviving island into the coast/lowest
		// material band.  Eight percent is only a numerical floor for all-water edge
		// cases; normal Ocean (9-13%) and Archipelago profiles now use their authored
		// coverage and can reach the same readable lowland/highland palette layers as
		// dry bodies.  This remains a material-channel mapping only: Data.Height and
		// therefore WorldScape geometry/collision are deliberately unchanged.
		const double CoverageAwareLandSpan = 0.13 * FMath::Max(
			static_cast<double>(Profile.LandCoverage), 0.08);
		// Cryogenic's physically amplified broad bands legitimately exceed the common
		// 0.13 authored land span. A hard linear map consequently pinned most vertices
		// to red=0/1 and made real geometry look like one flat colour. Use a wider
		// display span for that family and a rational shoulder for every family. The
		// shoulder remains monotonic, preserves the exact coast value and never changes
		// Data.Height (the authoritative WorldScape displacement).
		const double AuthoredLandSpan = Profile.Archetype
			== EAPSPlanetSurfaceArchetype::Cryogenic
			? FMath::Max(CoverageAwareLandSpan, 0.22)
			: CoverageAwareLandSpan;
		const double HeightAboveSea = PhysicalHeightNormalized
			- static_cast<double>(Profile.OceanLevel);
		if (HeightAboveSea >= 0.0)
		{
			const double LandRatio = HeightAboveSea / FMath::Max(AuthoredLandSpan, 0.001);
			return MaterialCoastHeight
				+ MaterialLandHeightRange * LandRatio / (1.0 + LandRatio);
		}

		const double BasinRatio = -HeightAboveSea
			/ FMath::Max(AuthoredLandSpan * 0.35, 0.001);
		return MaterialCoastHeight / (1.0 + BasinRatio);
	}
}

void UAPSWorldScapePlanetNoise::Configure(const FAPSResolvedPlanetSurfaceProfile& InProfile)
{
	SurfaceProfile = InProfile;
	bNeedPlanetRefresh = true;
}
FNoiseData UAPSWorldScapePlanetNoise::GetNoise(
	CustomNoise NoiseClass, const DVector& Position, const DVector& PlanetPosition,
	double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld,
	double Latitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	return Evaluate(NoiseClass, Position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, Latitude, NoisePosition);
}

FNoiseData UAPSWorldScapePlanetNoise::GetOceanNoise(
	CustomNoise NoiseClass, const DVector& Position, const DVector& PlanetPosition,
	double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld,
	double Latitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	FNoiseData Data;
	const double SafePlanetScale = FMath::Max(PlanetScale, 1.0);
	// WorldScape supplies Position in root-local ECEF space. PlanetPosition is the
	// root's world-placement metadata; subtracting it again offsets the sample when
	// the root moves and makes ocean and terrain evaluate different directions.
	NoisePosition = (Position / SafePlanetScale) * FMath::Max(NoiseScale, 1.0);
	const double PhysicalOceanHeightNormalized = static_cast<double>(SurfaceProfile.OceanLevel);
	Data.Height = PhysicalOceanHeightNormalized * NoiseIntensity;
	// Keep the ocean displacement signed/physical in Height. HeightNormalize is
	// still a WorldScape material vertex channel and receives a stable 0..1 coast
	// classification even if a liquid graph starts consuming vertex red later.
	Data.HeightNormalize = APSPlanetNoise::MaterialCoastHeight;
	Data.Temperature = SurfaceProfile.Temperature;
	Data.Humidity = SurfaceProfile.Humidity;
	Data.WaterMask = 1.0f;
	return Data;
}

FNoiseData UAPSWorldScapePlanetNoise::SampleResolved(
	CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
	double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
	DVector& NoisePosition) const
{
	return EvaluateProfile(SurfaceProfile, NoiseClass, Position, PlanetPosition,
		NoiseScale, NoiseIntensity, PlanetScale, Latitude, NoisePosition);
}

FNoiseData UAPSWorldScapePlanetNoise::SampleResolvedProfile(
	const FAPSResolvedPlanetSurfaceProfile& SurfaceProfile,
	CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
	double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
	DVector& NoisePosition)
{
	return EvaluateProfile(SurfaceProfile, NoiseClass, Position, PlanetPosition,
		NoiseScale, NoiseIntensity, PlanetScale, Latitude, NoisePosition);
}

FNoiseData UAPSWorldScapePlanetNoise::Evaluate(
	CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
	double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
	DVector& NoisePosition) const
{
	return EvaluateProfile(SurfaceProfile, NoiseClass, Position, PlanetPosition,
		NoiseScale, NoiseIntensity, PlanetScale, Latitude, NoisePosition);
}

FNoiseData UAPSWorldScapePlanetNoise::EvaluateProfile(
	const FAPSResolvedPlanetSurfaceProfile& SurfaceProfile,
	CustomNoise& NoiseClass, const DVector& Position, const DVector& PlanetPosition,
	double NoiseScale, double NoiseIntensity, double PlanetScale, double Latitude,
	DVector& NoisePosition)
{
	FNoiseData Data;
	const double SafePlanetScale = FMath::Max(PlanetScale, 1.0);
	const double EffectiveScale = FMath::Max(NoiseScale, 1.0);
	// WorldScape Position is already root-local ECEF. PlanetPosition describes the
	// actor's world placement and must not be subtracted a second time here.
	const DVector PlanetVector = Position / SafePlanetScale;
	const DVector PlanetDirection = APSPlanetNoise::SafePlanetDirection(PlanetVector);
	NoisePosition = PlanetVector * EffectiveScale;

	const DVector TerrainOffset(
		static_cast<double>(SurfaceProfile.TerrainSeed % 997) * 0.013,
		static_cast<double>(SurfaceProfile.TerrainSeed % 991) * 0.017,
		static_cast<double>(SurfaceProfile.TerrainSeed % 983) * 0.019);
	const DVector BiomeOffset(
		static_cast<double>(SurfaceProfile.BiomeSeed % 977) * 0.011,
		static_cast<double>(SurfaceProfile.BiomeSeed % 971) * 0.014,
		static_cast<double>(SurfaceProfile.BiomeSeed % 967) * 0.016);

	const DVector TerrainPosition = NoisePosition + TerrainOffset;
	const double ContinentalFrequency = FMath::Clamp(
		static_cast<double>(SurfaceProfile.ContinentalFrequencyMultiplier), 0.25, 2.5);
	const double RegionalFrequency = FMath::Clamp(
		static_cast<double>(SurfaceProfile.RegionalFrequencyMultiplier), 0.25, 2.5);
	const double DetailFrequency = FMath::Clamp(
		static_cast<double>(SurfaceProfile.DetailFrequencyMultiplier), 0.25, 2.5);
	const double RidgeFrequency = FMath::Clamp(
		static_cast<double>(SurfaceProfile.RidgeFrequencyMultiplier), 0.25, 2.5);
	const double CellularFrequency = FMath::Clamp(
		static_cast<double>(SurfaceProfile.CellularFrequencyMultiplier), 0.25, 2.5);
	// Continental topology is evaluated in direction space. The former
	// NoiseScale-dependent frequency gave an Ocean world less than one noise cell
	// across its entire globe and made a seed resolve to either all water or all
	// land. Detail still follows NoiseScale, while the macro field remains stable.
	const DVector ContinentalPosition = PlanetDirection * (2.45 * ContinentalFrequency)
		+ TerrainOffset * 0.23;
	const double Continental01 = FMath::Clamp(
		NoiseClass.Fractal(ContinentalPosition, 5, 2.05, 0.52), 0.0, 1.0);
	double ContinentalAreaField = APSPlanetNoise::UniformizeContinentalField(Continental01);
	if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::Pangea))
	{
		// A seeded spherical cap has an exactly uniform area distribution and gives
		// Pangea one coherent primary continent. A restrained fBm warp keeps its
		// coastline organic without fragmenting it into an archipelago.
		const DVector PangeaAxis = APSPlanetNoise::MakePangeaAxis(SurfaceProfile.TerrainSeed);
		const double CapField = FMath::Clamp(
			0.5 * (PlanetDirection.X * PangeaAxis.X
				+ PlanetDirection.Y * PangeaAxis.Y
				+ PlanetDirection.Z * PangeaAxis.Z + 1.0), 0.0, 1.0);
		ContinentalAreaField = FMath::Clamp(
			CapField + (ContinentalAreaField - 0.5) * 0.14, 0.0, 1.0);
	}
	const double Regional = FMath::Clamp(
		NoiseClass.Fractal(TerrainPosition * (0.014 * RegionalFrequency), 5, 2.1, 0.48), 0.0, 1.0) - 0.5;
	const double Detail = FMath::Clamp(
		NoiseClass.Fractal(TerrainPosition * (0.07 * DetailFrequency), 4, 2.2, 0.42), 0.0, 1.0) - 0.5;
	const double MicroDetail = FMath::Clamp(
		NoiseClass.Fractal(TerrainPosition * (0.19 * DetailFrequency), 3, 2.17, 0.44), 0.0, 1.0) - 0.5;
	const double Ridges = FMath::Clamp(
		NoiseClass.FractalRidge(TerrainPosition * (0.011 * RidgeFrequency), 5, 2.0, 0.52), 0.0, 1.0);
	const double Cellular = FMath::Clamp(NoiseClass.CellularNoise(
		TerrainPosition * (0.018 * CellularFrequency), ECellularDistanceType::Euclidean,
		ECellularType::Distance2Sub, 0.86), -1.0, 1.0);

	// NoiseScale is intentionally an orbital-scale control. Even its former
	// "micro" octave still spans many kilometres on an Earth-sized body, so a
	// collision patch around a walking pawn could be mathematically almost flat.
	// Add direction-space bands whose wavelength is expressed in physical
	// centimetres instead. Deriving the unscaled radius from the intensity ratio
	// keeps the exact same angular field in the compressed preview and full-scale
	// gameplay worlds. All bands affect Data.Height; only the three kilometre-scale
	// bands also feed the low-pass material classification below.
	const double ProfileIntensity = FMath::Max(
		FMath::Abs(static_cast<double>(SurfaceProfile.NoiseIntensity)), 1.0);
	const double PresentationRatio = FMath::Clamp(
		FMath::Abs(NoiseIntensity) / ProfileIntensity, 1.0e-9, 1.0);
	const double PhysicalPlanetScale = SafePlanetScale / PresentationRatio;
	// A normalized PLANET-page globe is a presentation of the same resolved body,
	// not a centimetre-for-centimetre gameplay planet. Reconstructing the physical
	// 18 km..60 m bands on that tiny globe turns them into roughly 32 m..11 cm waves,
	// far above the outer clipmap sampling rate. They then alias into the concentric
	// rings and dots visible in the rendered gallery. Keep every physical band exactly
	// as authored for a full-scale root; the scaled orbital root uses the bounded
	// continental/regional silhouette of this same WorldScape surface.
	const double PhysicalDetailWeight = PresentationRatio < 0.999 ? 0.0 : 1.0;
	constexpr double GroundRegionalBaseWavelengthCm = 1800000.0; // 18 km base landform
	constexpr double GroundReliefBaseWavelengthCm = 400000.0;    // 4 km terrain relief
	constexpr double GroundRollingBaseWavelengthCm = 180000.0;   // 1.8 km rolling terrain
	constexpr double GroundMicroBaseWavelengthCm = 90000.0;      // 0.9 km local relief
	constexpr double GroundWalkBaseWavelengthCm = 24000.0;       // 0.24 km walk-scale break-up
	// 60 m is the smallest geometric band retained by the streamed mesh. Shorter
	// wavelengths cannot be Nyquist-filtered because WorldScape's GetNoise contract
	// does not expose the current LOD. Sampling them on successively coarser rings
	// therefore produced the nested square relief visible around the player. The
	// canonical material supplies continuous sub-60 m albedo/roughness/normal detail.
	constexpr double GroundFineBaseWavelengthCm = 6000.0;
	const double GroundRegionalFrequency = FMath::Clamp(
		PhysicalPlanetScale / GroundRegionalBaseWavelengthCm, 0.25, 10000000.0);
	const double GroundReliefFrequency = FMath::Clamp(
		PhysicalPlanetScale / GroundReliefBaseWavelengthCm, 0.5, 10000000.0);
	const double GroundRollingFrequency = FMath::Clamp(
		PhysicalPlanetScale / GroundRollingBaseWavelengthCm, 1.0, 10000000.0);
	const double GroundMicroFrequency = FMath::Clamp(
		PhysicalPlanetScale / GroundMicroBaseWavelengthCm, 1.0, 10000000.0);
	const double GroundWalkFrequency = FMath::Clamp(
		PhysicalPlanetScale / GroundWalkBaseWavelengthCm, 1.0, 10000000.0);
	const double GroundFineFrequency = FMath::Clamp(
		PhysicalPlanetScale / GroundFineBaseWavelengthCm, 1.0, 10000000.0);
	const DVector GroundRegionalPosition = PlanetDirection * GroundRegionalFrequency
		+ TerrainOffset * 1.19;
	const DVector GroundReliefPosition = PlanetDirection * GroundReliefFrequency
		+ TerrainOffset * 1.73;
	const DVector GroundRollingPosition = PlanetDirection * GroundRollingFrequency
		+ TerrainOffset * 2.03;
	const DVector GroundMicroPosition = PlanetDirection * GroundMicroFrequency
		+ TerrainOffset * 2.31;
	const DVector GroundWalkPosition = PlanetDirection * GroundWalkFrequency
		+ TerrainOffset * 2.89;
	const DVector GroundFinePosition = PlanetDirection * GroundFineFrequency
		+ TerrainOffset * 3.17;
	const double GroundRegional = FMath::Clamp(
		NoiseClass.Fractal(GroundRegionalPosition, 2, 2.07, 0.48), 0.0, 1.0) - 0.5;
	const double GroundRelief = FMath::Clamp(
		NoiseClass.Fractal(GroundReliefPosition, 2, 2.03, 0.48), 0.0, 1.0) - 0.5;
	const double GroundRolling = FMath::Clamp(
		NoiseClass.Fractal(GroundRollingPosition, 2, 2.01, 0.46), 0.0, 1.0) - 0.5;
	const double GroundMicro = FMath::Clamp(
		NoiseClass.Fractal(GroundMicroPosition, 2, 2.07, 0.46), 0.0, 1.0) - 0.5;
	const double GroundWalk = FMath::Clamp(
		NoiseClass.Fractal(GroundWalkPosition, 2, 2.03, 0.46), 0.0, 1.0) - 0.5;
	const double GroundFine = FMath::Clamp(
		NoiseClass.Fractal(GroundFinePosition, 2, 2.05, 0.44), 0.0, 1.0) - 0.5;

	// Height is authored relative to the liquid surface. OceanLevel therefore
	// changes sea altitude but can no longer silently turn a terrestrial profile
	// into an all-ocean world.
	const double LandThreshold = 1.0 - FMath::Clamp(
		static_cast<double>(SurfaceProfile.LandCoverage), 0.0, 1.0);
	const double SignedLand = ContinentalAreaField - LandThreshold;
	const double LandMask = FMath::SmoothStep(-0.035, 0.075, SignedLand);
	const double DeepLandMask = FMath::SmoothStep(0.025, 0.18, SignedLand);
	const double MountainSignal = FMath::SmoothStep(0.56, 0.86, Ridges);
	const double MountainMask = MountainSignal * DeepLandMask;
	const double GroundSurfaceMask = FMath::Lerp(0.35, 1.0, LandMask);
	const double GroundRoughness = FMath::Lerp(
		0.62, 1.35, static_cast<double>(SurfaceProfile.Roughness));
	const double GroundRegionalShape = FMath::Lerp(
		0.72, 1.32, static_cast<double>(SurfaceProfile.HillStrength))
		* FMath::Lerp(0.86, 1.18, static_cast<double>(SurfaceProfile.MountainStrength));
	const bool bCryogenicGround =
		SurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Cryogenic;
	const bool bHighMountainGround = SurfaceProfile.HasModifier(
		EAPSPlanetSurfaceModifier::HighMountain);
	const bool bTundraGround = SurfaceProfile.HasModifier(
		EAPSPlanetSurfaceModifier::TundraBands);
	// Tundra shares the broad cryogenic landform family, but its exposed mixed-soil
	// surface must remain traversable at pawn scale. Keep the authored macro shape and
	// subtype identity while taking only the physical geometry bands off the steep
	// shoulder that otherwise appears in a few deterministic seeds.
	const double PhysicalBandWeight = PhysicalDetailWeight * (bTundraGround ? 0.86 : 1.0);
	// Cryogenic bodies used to retain their orbital silhouette while becoming almost
	// planar inside a walking collision tile. Keep the boost in this same authoritative
	// height field: broad and rolling bands make readable hills while the restrained
	// sub-kilometre multiplier avoids replacing terrain with high-frequency ripples.
	const double GroundBroadLandformBoost = bHighMountainGround
		? 2.20 : (bCryogenicGround ? 3.50 : 1.0);
	const double GroundRollingLandformBoost = bHighMountainGround
		? 1.72 : (bCryogenicGround ? 3.90 : 1.0);
	const double GroundLocalLandformBoost = bHighMountainGround
		? 1.08 : (bCryogenicGround ? 2.70 : 1.0);
	// Fine geometry is intentionally not multiplied by the large Cryogenic broad-band
	// boost. It provides a restrained 60 m silhouette cue; finer appearance belongs to
	// the continuous material so every WorldScape LOD shades the same surface.
	const double GroundFineResponse = FMath::Lerp(
		0.72, 1.18, static_cast<double>(SurfaceProfile.Roughness))
		* (bCryogenicGround ? 1.20 : (bHighMountainGround ? 1.12 : 1.0));
	// Distance2Sub is -1 at cell boundaries and approaches 0 toward isolated cell
	// centres. A fourth power made the useful cavity occupy too few samples to move
	// even the first terrain percentile; the softer exponent produces recognisable
	// impact bowls while retaining deterministic cellular centres.
	const double CraterCavity = -FMath::Pow(
		FMath::Clamp(1.0 - FMath::Abs(Cellular), 0.0, 1.0), 1.75);

	double HeightNormalized = static_cast<double>(SurfaceProfile.OceanLevel) + SignedLand * 0.13;
	HeightNormalized += Regional * 0.026 * static_cast<double>(SurfaceProfile.HillStrength) * DeepLandMask;
	HeightNormalized += Detail * 0.008 * DeepLandMask;
	HeightNormalized += MicroDetail * 0.009
		* FMath::Lerp(0.25, 1.25, static_cast<double>(SurfaceProfile.Roughness)) * DeepLandMask;
	HeightNormalized += FMath::Square(MountainSignal) * 0.058
		* static_cast<double>(SurfaceProfile.MountainStrength) * MountainMask;
	// Put most displacement into broad landforms that remain recognisable both from
	// orbit and while walking. High Mountain and Cryogenic profiles receive bounded
	// profile-specific boosts, but still use the same single WorldScape height field
	// as every other solid body.
	// The five macro-through-walk physical bands cover regional silhouette through
	// 240 m walk-scale break-up. Most energy is deliberately in the 1.8-18 km bands so a pawn sees
	// coherent ridges and valleys rather than a displaced but visually flat tile.
	// Their profile boosts stay finite so the strengthened relief remains traversable.
	HeightNormalized += PhysicalBandWeight * GroundRegional * 0.0200 * GroundRegionalShape
		* GroundBroadLandformBoost * GroundSurfaceMask;
	HeightNormalized += PhysicalBandWeight * GroundRelief * 0.0280 * GroundRoughness
		* GroundBroadLandformBoost * GroundSurfaceMask;
	HeightNormalized += PhysicalBandWeight * GroundRolling * 0.0155 * GroundRoughness
		* GroundRollingLandformBoost * GroundSurfaceMask;
	HeightNormalized += PhysicalBandWeight * GroundMicro * 0.0050 * GroundRoughness
		* GroundLocalLandformBoost * GroundSurfaceMask;
	// Keep the authored 240 m band intact and retain only a low-amplitude 60 m
	// geometric cue. The removed 22/14 m bands crossed the sampling limit of outer
	// WorldScape rings and made each LOD footprint visible as a square. Material
	// micro-detail is evaluated per pixel and remains continuous across those rings.
	const double WalkBandCoefficient =
		0.0026 * GroundRoughness * GroundLocalLandformBoost
		* (bCryogenicGround ? 1.70 : 1.0);
	const double FineBandCoefficient = 0.00012 * GroundFineResponse;
	// Frozen ground must not collapse to a visually flat sheet in low continental
	// mask pockets. Preserve the authored coast/ice classification, but keep a small
	// physical relief floor for the local geometry bands. This changes the one
	// authoritative WorldScape height field only; no material displacement or preview
	// shell is introduced.
	const double LocalPhysicalSurfaceMask = bCryogenicGround
		? FMath::Max(GroundSurfaceMask, 0.48)
		: GroundSurfaceMask;
	HeightNormalized += PhysicalBandWeight * GroundWalk * WalkBandCoefficient
		* LocalPhysicalSurfaceMask;
	HeightNormalized += PhysicalBandWeight * GroundFine * FineBandCoefficient
		* LocalPhysicalSurfaceMask;
	// Keep orbital colour classification on a deliberately low-pass terrain field.
	// Feeding physical displacement or preset deformation into vertex R turns real
	// relief into nested palette isolines; at mixed WorldScape LODs those isolines also
	// reveal the square patch topology. Geometry retains every band in Data.Height.
	// Vertex R receives only broad continental/mountain structure plus a restrained
	// share of the 18 km and 4 km fields. The 1.8 km and finer physical bands and all
	// preset-only deformation remain geometry-only.
	const double PalettePhysicalBandGain = PhysicalDetailWeight * (bCryogenicGround
		? 0.24 : (bHighMountainGround ? 0.26 : 0.22));
	const double PaletteMacroHeightNormalized =
		static_cast<double>(SurfaceProfile.OceanLevel) + SignedLand * 0.13
		+ Regional * 0.014 * static_cast<double>(SurfaceProfile.HillStrength) * DeepLandMask
		+ Detail * 0.002 * DeepLandMask
		+ FMath::Square(MountainSignal) * 0.026
			* static_cast<double>(SurfaceProfile.MountainStrength) * MountainMask
		+ GroundRegional * 0.0200 * GroundRegionalShape * PalettePhysicalBandGain
			* GroundBroadLandformBoost * GroundSurfaceMask
		+ GroundRelief * 0.0280 * GroundRoughness * PalettePhysicalBandGain
			* GroundBroadLandformBoost * GroundSurfaceMask;
	// Impact basins do not stop at a sea-level classification. Applying a restrained
	// floor below the land mask keeps submerged craters in the physical terrain and
	// gives the crater control a robust lower-tail response without letting ocean-floor
	// detail dominate the coast silhouette.
	const double CraterSurfaceMask = FMath::Lerp(0.55, 1.0, LandMask);
	HeightNormalized += CraterCavity * 0.042
		* static_cast<double>(SurfaceProfile.CraterStrength) * CraterSurfaceMask;
	if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::Pangea))
	{
		HeightNormalized += Regional * 0.006 * DeepLandMask;
	}
	if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::Archipelago))
	{
		HeightNormalized += Regional * 0.018 * LandMask;
	}
	if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ActiveGeology))
	{
		HeightNormalized += MountainSignal * Regional * 0.014 * DeepLandMask;
	}

	const double PatternStrength = FMath::Clamp(
		static_cast<double>(SurfaceProfile.TerrainPatternStrength), 0.0, 1.0);
	const bool bNeedsPresetPattern =
		SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::Dwarf)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::AbyssalBasins)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ContinentalShelves)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ArchipelagoChains)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ForestCanopy)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::OasisWetlands)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::IceSheets)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::GlacialRifts)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::LavaSeas)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::CalderaFields)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::MetallicPlates)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::CarbonRidges)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ChemicalBands)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::MesaFields)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::AtmosphericErosion)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::AlienTerrain)
		|| SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::VolcanicFissures);
	if (bNeedsPresetPattern && PatternStrength > static_cast<double>(KINDA_SMALL_NUMBER))
	{
		const double Pattern01 = FMath::Clamp(NoiseClass.Fractal(
			TerrainPosition * (0.0064 * FMath::Sqrt(RegionalFrequency)), 4, 2.03, 0.51), 0.0, 1.0);
		const double Pattern = Pattern01 - 0.5;

		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::Dwarf))
		{
			// Broad ancient impact basins deliberately separate a dwarf body from
			// the finer, higher-relief Rocky preset at the same deterministic seed.
			HeightNormalized += CraterCavity * 0.042 * PatternStrength;
			HeightNormalized -= FMath::Abs(Pattern) * 0.014 * PatternStrength;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::AbyssalBasins))
		{
			HeightNormalized -= FMath::Abs(Pattern) * 0.022 * PatternStrength;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ContinentalShelves))
		{
			const double Shelf = FMath::SmoothStep(0.42, 0.61, Pattern01) - 0.5;
			HeightNormalized += Shelf * 0.018 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ArchipelagoChains))
		{
			const double Chain = FMath::Square(FMath::Max(Ridges, 0.0)) - 0.22;
			HeightNormalized += Chain * 0.028 * PatternStrength * LandMask
				+ Regional * 0.010 * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ForestCanopy))
		{
			HeightNormalized += Pattern * 0.010 * PatternStrength * DeepLandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::OasisWetlands))
		{
			const double WetBasin = FMath::SmoothStep(0.66, 0.84, Pattern01);
			HeightNormalized -= WetBasin * 0.024 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::IceSheets))
		{
			const double IceFlow = FMath::Clamp(NoiseClass.BillowFractal(
				TerrainPosition * 0.025, 3, 2.0, 0.47), -1.0, 1.0);
			HeightNormalized += IceFlow * 0.010 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::GlacialRifts))
		{
			HeightNormalized -= FMath::Pow(Ridges, 5.0) * 0.020 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::LavaSeas))
		{
			HeightNormalized += Pattern * 0.022 * PatternStrength * LandMask
				- 0.006 * PatternStrength;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::CalderaFields))
		{
			const double CalderaCavity = -FMath::Pow(
				FMath::Clamp(1.0 - FMath::Abs(Cellular), 0.0, 1.0), 2.4);
			HeightNormalized += CalderaCavity * 0.026 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::MetallicPlates))
		{
			HeightNormalized += Cellular * 0.016 * PatternStrength * DeepLandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::CarbonRidges))
		{
			HeightNormalized += FMath::Pow(Ridges, 3.0) * 0.030 * PatternStrength * DeepLandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ChemicalBands))
		{
			// Latitude-warped bands are continuous in planet space. The former planar
			// X/Y sine exposed cube-face orientation and looked like a technical grid.
			const double Bands = FMath::Sin((PlanetDirection.Z * 4.5
				+ Regional * 0.65 + Detail * 0.08) * UE_PI);
			HeightNormalized += Bands * 0.012 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::MesaFields))
		{
			const double Mesa = static_cast<double>(FMath::RoundToInt(Pattern * 5.0)) / 5.0;
			HeightNormalized += Mesa * 0.024 * PatternStrength * DeepLandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::AtmosphericErosion))
		{
			HeightNormalized -= FMath::Pow(Ridges, 2.0) * 0.014 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::AlienTerrain))
		{
			// A coherent ridge/region mixture stays unfamiliar without producing the
			// axis-aligned lattice that revealed WorldScape patch orientation.
			const double AlienField = (Ridges - 0.5) * 0.62 + Regional * 0.74
				+ Detail * 0.16;
			HeightNormalized += AlienField * 0.020 * PatternStrength * LandMask;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::VolcanicFissures))
		{
			HeightNormalized -= FMath::Pow(Ridges, 6.0) * 0.018 * PatternStrength * LandMask;
		}
	}
	if (SurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Magmatic)
	{
		const double Caldera = NoiseClass.CellularNoise(
			TerrainPosition * 0.009, ECellularDistanceType::EuclideanSq,
			ECellularType::Distance2Sub, 0.92);
		HeightNormalized += Caldera * 0.014 * LandMask;
	}
	if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::SandDunes))
	{
		const double Dunes = FMath::Clamp(
			NoiseClass.BillowFractal(TerrainPosition * 0.11, 3, 2.0, 0.48), -1.0, 1.0);
		HeightNormalized += Dunes * 0.012 * FMath::Max(PatternStrength, 0.2) * DeepLandMask;
	}

	const double PhysicalHeightNormalized = FMath::Clamp(HeightNormalized, -0.35, 0.65);
	Data.Height = PhysicalHeightNormalized * NoiseIntensity;
	// Palette classification is intentionally independent from the high-frequency and
	// preset-specific displacement above. Preset identity is retained by the resolved
	// family palette, climate channels and the physical silhouette, without drawing the
	// displacement field a second time as coloured contour lines.
	const double PaletteHeightNormalized = FMath::Clamp(
		PaletteMacroHeightNormalized, -0.35, 0.65);
	Data.HeightNormalize = APSPlanetNoise::NormalizeHeightForMaterial(
		PaletteHeightNormalized, SurfaceProfile);

	const double ClampedLatitude = FMath::Clamp(Latitude, -1.0, 1.0);
	const double EquatorialWarmth = 1.0 - FMath::Abs(FMath::Asin(ClampedLatitude) / UE_HALF_PI);
	const double LatitudeStrength = FMath::Clamp(
		static_cast<double>(SurfaceProfile.LatitudeClimateStrength), 0.5, 1.75);
	const double ShapedEquatorialWarmth = FMath::Clamp(
		0.5 + (EquatorialWarmth - 0.5) * LatitudeStrength, 0.0, 1.0);
	const double ClimateNoise = FMath::Clamp(
		NoiseClass.Fractal((NoisePosition + BiomeOffset) * 0.0035, 4, 2.0, 0.5), 0.0, 1.0) - 0.5;
	const double HeightCooling = FMath::Clamp(PhysicalHeightNormalized, 0.0, 1.0) * 0.5;
	double TemperatureValue = static_cast<double>(SurfaceProfile.Temperature)
		* (0.42 + ShapedEquatorialWarmth * 0.58) + ClimateNoise * 0.08 - HeightCooling;

	const double HumidityNoise = FMath::Clamp(
		NoiseClass.Fractal((NoisePosition + BiomeOffset) * 0.0065, 5, 2.1, 0.5), 0.0, 1.0) - 0.5;
	const double OceanInfluence = 1.0 - FMath::SmoothStep(-0.02, 0.16, SignedLand);
	double HumidityValue = static_cast<double>(SurfaceProfile.Humidity) + HumidityNoise * 0.16
		+ OceanInfluence * 0.24 - HeightCooling * 0.32;
	double VegetationPatch = 1.0;
	const double ClimatePatchStrength = FMath::Clamp(
		static_cast<double>(SurfaceProfile.ClimatePatchStrength), 0.0, 1.0);
	if (ClimatePatchStrength > static_cast<double>(KINDA_SMALL_NUMBER))
	{
		const double ClimatePatch01 = FMath::Clamp(NoiseClass.Fractal(
			(NoisePosition + BiomeOffset) * 0.0019, 4, 2.0, 0.53), 0.0, 1.0);
		HumidityValue += (ClimatePatch01 - 0.5) * 0.32 * ClimatePatchStrength;
		TemperatureValue += (0.5 - ClimatePatch01) * 0.08 * ClimatePatchStrength;

		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ForestCanopy))
		{
			// Large coherent humidity/temperature patches feed the material's vertex
			// channels, so a forest world reads as biomes even at orbital distance.
			VegetationPatch = FMath::SmoothStep(0.30, 0.72, ClimatePatch01);
			HumidityValue += (VegetationPatch - 0.58) * 0.72 * ClimatePatchStrength;
			TemperatureValue -= VegetationPatch * 0.035;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::OasisWetlands))
		{
			VegetationPatch = FMath::SmoothStep(0.66, 0.84, ClimatePatch01);
			HumidityValue += VegetationPatch * 0.72 * ClimatePatchStrength - 0.06;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::TundraBands))
		{
			const double PolarBand = FMath::SmoothStep(0.42, 0.82, FMath::Abs(ClampedLatitude));
			HumidityValue += PolarBand * 0.13;
			TemperatureValue -= PolarBand * 0.08;
		}
		if (SurfaceProfile.HasModifier(EAPSPlanetSurfaceModifier::ChemicalBands))
		{
			const double ChemicalBand = FMath::Sin(ClampedLatitude * UE_PI * 7.0 + ClimatePatch01 * 2.0);
			HumidityValue += ChemicalBand * 0.10 * ClimatePatchStrength;
			TemperatureValue -= ChemicalBand * 0.045 * ClimatePatchStrength;
		}
	}

	Data.Temperature = static_cast<float>(FMath::Clamp(TemperatureValue, 0.0, 1.0));
	Data.Humidity = static_cast<float>(FMath::Clamp(HumidityValue, 0.0, 1.0));

	// This channel is the stable below-datum classification for every solid body.
	// Wet profiles classify the final displaced coast. Dry profiles classify the
	// macro continental field itself: craters remain visible terrain rather than
	// becoming imaginary lakes, while Tundra/Exoplanet/Unknown still retain their
	// authored basin-to-highland ratio. LiquidType alone controls ocean allocation.
	if (SurfaceProfile.LiquidType == EAPSPlanetLiquidType::None)
	{
		Data.WaterMask = static_cast<float>(1.0 - FMath::SmoothStep(
			-0.035, 0.035, SignedLand));
	}
	else
	{
		Data.WaterMask = static_cast<float>(1.0 - FMath::SmoothStep(
			static_cast<double>(SurfaceProfile.OceanLevel) - 0.012,
			static_cast<double>(SurfaceProfile.OceanLevel) + 0.012,
			PhysicalHeightNormalized));
	}
	const float ThermalSuitability = 1.0f - FMath::Abs(Data.Temperature - 0.58f) / 0.58f;
	const float MoistureSuitability = FMath::SmoothStep(0.18f, 0.72f, Data.Humidity);
	Data.FoliageMask = FMath::Clamp(
		SurfaceProfile.Biomass * ThermalSuitability * MoistureSuitability
			* static_cast<float>(FMath::Lerp(0.18, 1.0, VegetationPatch)) * (1.0f - Data.WaterMask),
		0.0f, 1.0f);
	return Data;
}
