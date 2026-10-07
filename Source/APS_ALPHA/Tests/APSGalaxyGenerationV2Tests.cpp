#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Generation/StarClusterGenerator.h"
#include "APS_ALPHA/UI/MainMenu/APSWorldRoll.h"
#include "Engine/World.h"

// Rio 03.10 (galaxy/cluster generation V2, Docs/Design/GALAXY_GENERATION_V2.md).
// The frozen copies below are the pre-03.10 algorithms verbatim. They prove that every historic
// (type, class) pair and every historic cluster formation still produces the same output, which
// is what makes old saves re-resolve identical worlds. Do not "fix" them.
namespace APSGalaxyV2TestsFrozen
{
	inline uint64 Mix64(uint64 Value)
	{
		Value ^= Value >> 30;
		Value *= 0xbf58476d1ce4e5b9ull;
		Value ^= Value >> 27;
		Value *= 0x94d049bb133111ebull;
		Value ^= Value >> 31;
		return Value;
	}

	inline uint64 HashCatalogIndex(const int32 Seed, const int64 CatalogIndex, const uint64 Salt = 0)
	{
		const uint64 SeedBits = static_cast<uint64>(static_cast<uint32>(Seed)) << 32;
		return Mix64(SeedBits ^ static_cast<uint64>(CatalogIndex) ^ Salt ^ 0x9e3779b97f4a7c15ull);
	}

	struct FCatalogRandomStream
	{
		explicit FCatalogRandomStream(const uint64 InSeed)
			: State(InSeed)
		{
		}

		uint64 NextUInt64()
		{
			State += 0x9e3779b97f4a7c15ull;
			return Mix64(State);
		}

		double Fraction()
		{
			return static_cast<double>(NextUInt64() >> 11) * (1.0 / 9007199254740992.0);
		}

		double Range(const double Min, const double Max)
		{
			return FMath::Lerp(Min, Max, Fraction());
		}

		int32 Range(const int32 Min, const int32 Max)
		{
			return Min + static_cast<int32>(NextUInt64() % static_cast<uint64>(Max - Min + 1));
		}

		uint64 State;
	};

	inline FVector RandomUnitVector(FCatalogRandomStream& Stream)
	{
		for (int32 Attempt = 0; Attempt < 12; ++Attempt)
		{
			const FVector Candidate(
				Stream.Range(-1.0, 1.0),
				Stream.Range(-1.0, 1.0),
				Stream.Range(-1.0, 1.0));
			const double SizeSquared = Candidate.SizeSquared();
			if (SizeSquared > UE_DOUBLE_SMALL_NUMBER && SizeSquared <= 1.0)
			{
				return Candidate.GetSafeNormal();
			}
		}
		return FVector::ForwardVector;
	}

	inline ESpectralClass ChooseSpectralClass(const float Value)
	{
		if (Value < 0.650f) return ESpectralClass::M;
		if (Value < 0.790f) return ESpectralClass::K;
		if (Value < 0.880f) return ESpectralClass::G;
		if (Value < 0.930f) return ESpectralClass::F;
		if (Value < 0.960f) return ESpectralClass::A;
		if (Value < 0.975f) return ESpectralClass::B;
		if (Value < 0.980f) return ESpectralClass::O;
		if (Value < 0.988f) return ESpectralClass::L;
		if (Value < 0.994f) return ESpectralClass::T;
		if (Value < 0.997f) return ESpectralClass::Y;
		if (Value < 0.9985f) return ESpectralClass::NS;
		if (Value < 0.9995f) return ESpectralClass::PS;
		return ESpectralClass::BH;
	}

	struct FRecord
	{
		FVector Position{FVector::ZeroVector};
		int32 GenerationSeed{0};
		ESpectralClass SpectralClass{ESpectralClass::Unknown};
		int32 SpectralSubclass{0};
		bool bPotentialStarSystem{false};
	};

	/** FGalaxyCatalogDescriptor::ResolveStar as of 05f544e6 (02.10). */
	inline bool ResolveStar(const FGalaxyCatalogDescriptor& Catalog, const int64 CatalogIndex, FRecord& OutRecord)
	{
		const int32 GenerationSeed = Catalog.GenerationSeed;
		const EGalaxyType GalaxyType = Catalog.GalaxyType;
		const EGalaxyClass GalaxyClass = Catalog.GalaxyClass;
		if (CatalogIndex < 0 || CatalogIndex >= Catalog.ModeledStarCount)
		{
			return false;
		}

		const uint64 RecordHash = HashCatalogIndex(GenerationSeed, CatalogIndex);
		FCatalogRandomStream Stream(RecordHash);
		const double DensityScale = FMath::Sqrt(10.0 / FMath::Clamp(Catalog.StarDensity, 0.01, 1000.0));
		const double GalaxyRadius = FMath::Max(50000.0,
			static_cast<double>(FMath::Max(Catalog.GalaxySize, 1)) * 50000.0) * DensityScale;
		FVector Position = FVector::ZeroVector;
		const bool bDiskLikeGalaxy = GalaxyType == EGalaxyType::Lenticular
			|| GalaxyType == EGalaxyType::Spiral
			|| GalaxyType == EGalaxyType::BarredSpiral
			|| GalaxyType == EGalaxyType::Peculiar;
		const double StructureSelector = static_cast<double>((RecordHash >> 24) & 0xffffull) / 65535.0;
		if (bDiskLikeGalaxy && StructureSelector < 0.18)
		{
			FCatalogRandomStream HaloStream(HashCatalogIndex(GenerationSeed, CatalogIndex, 0x47414c5f48414c4full));
			const double HaloRadius = FMath::Pow(HaloStream.Fraction(), 0.55) * GalaxyRadius;
			Position = RandomUnitVector(HaloStream) * HaloRadius;
		}
		else if (bDiskLikeGalaxy && StructureSelector < 0.34)
		{
			FCatalogRandomStream BulgeStream(HashCatalogIndex(GenerationSeed, CatalogIndex, 0x47414c5f42554c47ull));
			const double BulgeRadius = FMath::Pow(BulgeStream.Fraction(), 1.0 / 3.0) * GalaxyRadius * 0.22;
			Position = RandomUnitVector(BulgeStream) * BulgeRadius;
		}
		else
		{
			switch (GalaxyType)
			{
			case EGalaxyType::Elliptical:
				{
					const int32 Ellipticity = FMath::Clamp(
						static_cast<int32>(GalaxyClass) - static_cast<int32>(EGalaxyClass::E0), 0, 7);
					const double Radius = FMath::Pow(Stream.Fraction(), 1.0 / 3.0) * GalaxyRadius;
					Position = RandomUnitVector(Stream) * Radius;
					const double Flattening = 1.0 - Ellipticity * 0.09;
					Position.Y *= Flattening;
					Position.Z *= Flattening;
				}
				break;
			case EGalaxyType::Lenticular:
				{
					const double Radius = FMath::Sqrt(Stream.Fraction()) * GalaxyRadius;
					const double Angle = Stream.Range(0.0, UE_TWO_PI);
					const double Thickness = GalaxyRadius * (Stream.Fraction() < 0.18 ? 0.16 : 0.035);
					Position = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
						Stream.Range(-1.0, 1.0) * Thickness * (1.0 - Radius / GalaxyRadius * 0.65));
				}
				break;
			case EGalaxyType::Spiral:
			case EGalaxyType::BarredSpiral:
				{
					const int32 ClassValue = static_cast<int32>(GalaxyClass);
					const int32 ArmCount = FMath::Clamp(2 + (ClassValue % 4), 2, 5);
					const int32 ArmIndex = static_cast<int32>(RecordHash % static_cast<uint64>(ArmCount));
					const double RadiusAlpha = FMath::Sqrt(Stream.Fraction());
					const double Radius = RadiusAlpha * GalaxyRadius;
					const double ArmAngle = UE_TWO_PI * static_cast<double>(ArmIndex) / ArmCount;
					const double Twist = RadiusAlpha * UE_TWO_PI * 2.35;
					const double Jitter = Stream.Range(-0.22, 0.22) * (0.35 + RadiusAlpha);
					double X = FMath::Cos(ArmAngle + Twist + Jitter) * Radius;
					double Y = FMath::Sin(ArmAngle + Twist + Jitter) * Radius;
					if (GalaxyType == EGalaxyType::BarredSpiral && Stream.Fraction() < 0.24)
					{
						X = Stream.Range(-1.0, 1.0) * GalaxyRadius * 0.42;
						Y = Stream.Range(-1.0, 1.0) * GalaxyRadius * 0.055;
					}
					Position = FVector(X, Y, Stream.Range(-1.0, 1.0) * GalaxyRadius * 0.025 * (1.15 - RadiusAlpha));
				}
				break;
			case EGalaxyType::Irregular:
				{
					FCatalogRandomStream LobeStream(HashCatalogIndex(GenerationSeed,
						static_cast<int64>(RecordHash % 7ull), 0x4952524547554c52ull));
					const FVector LobeCenter = RandomUnitVector(LobeStream) * LobeStream.Range(0.05, 0.55) * GalaxyRadius;
					Position = LobeCenter + RandomUnitVector(Stream) * FMath::Pow(Stream.Fraction(), 1.8)
						* GalaxyRadius * 0.48;
				}
				break;
			case EGalaxyType::Peculiar:
			default:
				{
					const double Angle = Stream.Range(0.0, UE_TWO_PI);
					const double Radius = GalaxyRadius * Stream.Range(0.28, 1.0);
					Position = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
						FMath::Sin(Angle * 2.0) * GalaxyRadius * 0.18 + Stream.Range(-1.0, 1.0) * GalaxyRadius * 0.06);
				}
				break;
			}
		}

		OutRecord.Position = Position;
		OutRecord.GenerationSeed = FMath::Max(1, static_cast<int32>(RecordHash & 0x7fffffffull));
		OutRecord.SpectralClass = ChooseSpectralClass(static_cast<float>(Stream.Fraction()));
		OutRecord.SpectralSubclass = Stream.Range(0, 9);
		OutRecord.bPotentialStarSystem = Stream.Fraction() < 0.82;
		return true;
	}

	/** UStarClusterGenerator::CalculateStarPosition as of 05f544e6, before its final x100. */
	inline FVector ClusterPosition(const int StarIndex, const int32 StarAmount, const int32 GenerationSeed,
		const EStarClusterType ClusterType, const FVector& ClusterBounds, const double StarSize)
	{
		FVector StarPosition;
		const double NormalizedIndex = StarAmount > 1
			? static_cast<double>(StarIndex) / static_cast<double>(StarAmount - 1) : 0.5;
		switch (ClusterType)
		{
		case EStarClusterType::OpenCluster:
			StarPosition = FMath::RandPointInBox(FBox(
				FVector(-ClusterBounds.X / 2 - StarSize, -ClusterBounds.Y / 2 - StarSize, -ClusterBounds.Z / 2 - StarSize),
				FVector(ClusterBounds.X / 2 + StarSize, ClusterBounds.Y / 2 + StarSize, ClusterBounds.Z / 2 + StarSize)));
			break;
		case EStarClusterType::GlobularCluster:
			{
				double Radius = FMath::RandRange(static_cast<double>(StarSize), ClusterBounds.X / 2) + StarSize * 100;
				double Angle = FMath::RandRange(0.0f, 2 * PI);
				double Z = FMath::RandRange(-ClusterBounds.Z / 2, ClusterBounds.Z / 2);
				StarPosition = FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
			}
			break;
		case EStarClusterType::Supercluster:
			{
				double SphereRadius = FMath::Max(ClusterBounds.X, FMath::Max(ClusterBounds.Y, ClusterBounds.Z)) / 2;
				FVector RandomPoint = FMath::VRand();
				double RandomScale = FMath::Pow(FMath::FRand(), 1.6) * FMath::Max(SphereRadius - StarSize, SphereRadius * 0.1);
				RandomPoint *= RandomScale;
				StarPosition = RandomPoint;
				if (StarSize > 10.0f)
				{
					StarPosition /= 2;
				}
				if (StarSize > 10.0f)
				{
					StarPosition /= 2;
				}
			}
			break;
		case EStarClusterType::Nebula:
			{
				const int32 PairCount = StarAmount / 2;
				const bool bUnpairedCentre = (StarAmount % 2) != 0 && StarIndex == StarAmount - 1;
				if (bUnpairedCentre || PairCount <= 0)
				{
					StarPosition = FVector::ZeroVector;
					break;
				}
				const int32 PairIndex = StarIndex / 2;
				const bool bOppositePoint = (StarIndex & 1) != 0;
				const uint32 PairSeedHash = HashCombine(GetTypeHash(GenerationSeed), GetTypeHash(PairIndex));
				FRandomStream PairStream(FMath::Max(1, static_cast<int32>(PairSeedHash & 0x7fffffffu)));
				const double PairAlpha = (static_cast<double>(PairIndex) + 0.5) / static_cast<double>(PairCount);
				const double MaxRadialExtent = FMath::Max(0.0,
					FMath::Min(FMath::Abs(ClusterBounds.X), FMath::Abs(ClusterBounds.Y)) * 0.5);
				const double Radius = MaxRadialExtent * FMath::Sqrt(PairAlpha) * PairStream.FRandRange(0.78f, 1.0f);
				const double BaseAngle = PairAlpha * UE_TWO_PI * 3.25 + PairStream.FRandRange(-0.16f, 0.16f);
				const double Angle = BaseAngle + (bOppositePoint ? PI : 0.0);
				const double HalfHeight = FMath::Abs(ClusterBounds.Z) * 0.5;
				const double VerticalEnvelope = FMath::Lerp(1.0, 0.18, PairAlpha);
				const double PairZ = PairStream.FRandRange(-HalfHeight, HalfHeight) * VerticalEnvelope;
				StarPosition = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
					bOppositePoint ? -PairZ : PairZ);
			}
			break;
		case EStarClusterType::ElongatedStream:
			{
				const double X = FMath::Lerp(-ClusterBounds.X * 0.5, ClusterBounds.X * 0.5, NormalizedIndex);
				const double Taper = 0.2 + 0.8 * FMath::Sin(PI * NormalizedIndex);
				const double Y = FMath::RandRange(-ClusterBounds.Y * 0.5, ClusterBounds.Y * 0.5) * Taper
					+ FMath::Sin(NormalizedIndex * UE_TWO_PI * 2.0) * ClusterBounds.Y * 0.16;
				const double Z = FMath::RandRange(-ClusterBounds.Z * 0.5, ClusterBounds.Z * 0.5) * Taper;
				StarPosition = FVector(X, Y, Z);
			}
			break;
		case EStarClusterType::RingArc:
			{
				const double ArcHalfAngle = PI * 0.82;
				const double Angle = FMath::Lerp(-ArcHalfAngle, ArcHalfAngle, NormalizedIndex);
				const double BaseRadius = FMath::Min(ClusterBounds.X, ClusterBounds.Y) * 0.38;
				const int32 MirroredIndex = StarAmount - 1 - StarIndex;
				const int32 PairIndex = FMath::Min(StarIndex, MirroredIndex);
				const uint32 PairSeedHash = HashCombine(GetTypeHash(GenerationSeed), GetTypeHash(PairIndex));
				FRandomStream PairStream(FMath::Max(1, static_cast<int32>(PairSeedHash & 0x7fffffffu)));
				const double JitterEnvelope = FMath::Square(FMath::Sin(UE_TWO_PI * NormalizedIndex));
				const double Radius = BaseRadius * (1.0 + PairStream.FRandRange(-0.14f, 0.14f) * JitterEnvelope);
				const double ArcBoundsCentreX = BaseRadius * (1.0 + FMath::Cos(ArcHalfAngle)) * 0.5;
				const double HalfHeight = FMath::Abs(ClusterBounds.Z) * 0.5;
				const double HeightMagnitude = PairStream.FRandRange(0.0f, HalfHeight);
				const double HeightSign = (PairSeedHash & 1u) != 0u ? 1.0 : -1.0;
				StarPosition = FVector(FMath::Cos(Angle) * Radius - ArcBoundsCentreX, FMath::Sin(Angle) * Radius,
					HeightMagnitude * HeightSign);
			}
			break;
		case EStarClusterType::Hourglass:
			{
				const double SignedHeight = FMath::RandRange(-1.0, 1.0);
				const double Z = SignedHeight * ClusterBounds.Z * 0.5;
				const double LobeRadius = FMath::Lerp(ClusterBounds.X * 0.04, ClusterBounds.X * 0.48, FMath::Abs(SignedHeight));
				const double Radius = FMath::Sqrt(FMath::FRand()) * LobeRadius;
				const double Angle = FMath::FRandRange(0.0, UE_TWO_PI);
				StarPosition = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z);
			}
			break;
		case EStarClusterType::Unknown:
		default:
			StarPosition = FMath::RandPointInBox(FBox(-ClusterBounds * 0.5, ClusterBounds * 0.5));
			break;
		}
		return StarPosition;
	}
}

namespace APSGalaxyV2Tests
{
	constexpr int32 FirstNewClassValue = static_cast<int32>(EGalaxyClass::Unknown) + 1;

	FGalaxyCatalogDescriptor MakeCatalog(const int32 Seed, const EGalaxyType Type, const EGalaxyClass Class)
	{
		FGalaxyCatalogDescriptor Catalog;
		Catalog.GenerationSeed = Seed;
		Catalog.ModeledStarCount = 100000000;
		Catalog.GalaxySize = 250;
		Catalog.StarDensity = 10.0;
		Catalog.GalaxyType = Type;
		Catalog.GalaxyClass = Class;
		return Catalog;
	}

	double CatalogRadius(const FGalaxyCatalogDescriptor& Catalog)
	{
		return FMath::Max(50000.0, static_cast<double>(FMath::Max(Catalog.GalaxySize, 1)) * 50000.0)
			* FMath::Sqrt(10.0 / FMath::Clamp(Catalog.StarDensity, 0.01, 1000.0));
	}

	int64 SampleIndex(const FGalaxyCatalogDescriptor& Catalog, const int32 Sample, const int32 Samples)
	{
		// The +17 stride pushed the last samples of a 100M catalogue past its end (3998 and 3999 of 4000), where
		// ResolveStar refuses the index and every shape test reported "not finite" (gate g2, 03.10).
		return FMath::Min(Catalog.ModeledStarCount / Samples * Sample + 17 * Sample, Catalog.ModeledStarCount - 1);
	}

	bool IsHot(const ESpectralClass Class)
	{
		return Class == ESpectralClass::O || Class == ESpectralClass::B || Class == ESpectralClass::A;
	}

	struct FShapeStats
	{
		double Central{0.0};
		double Outer{0.0};
		double MeanAbsZ{0.0};
		double AxisYX{0.0};
		double AxisZX{0.0};
		double Hot{0.0};
		double MaxRadius{0.0};
		bool bFinite{true};
	};

	FShapeStats Measure(const FGalaxyCatalogDescriptor& Catalog, const int32 Samples)
	{
		FShapeStats Result;
		const double Radius = CatalogRadius(Catalog);
		double SumX2 = 0.0;
		double SumY2 = 0.0;
		double SumZ2 = 0.0;
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			FGalaxyCatalogStarRecord Record;
			if (!Catalog.ResolveStar(SampleIndex(Catalog, Sample, Samples), Record))
			{
				Result.bFinite = false;
				continue;
			}
			const FVector Unit = Record.GalaxyLocalLocation / Radius;
			Result.bFinite &= !Unit.ContainsNaN();
			const double Distance = Unit.Size();
			Result.Central += Distance < 0.15 ? 1.0 : 0.0;
			Result.Outer += Distance > 0.6 ? 1.0 : 0.0;
			Result.MeanAbsZ += FMath::Abs(Unit.Z);
			Result.Hot += IsHot(Record.SpectralClass) ? 1.0 : 0.0;
			Result.MaxRadius = FMath::Max(Result.MaxRadius, Distance);
			SumX2 += Unit.X * Unit.X;
			SumY2 += Unit.Y * Unit.Y;
			SumZ2 += Unit.Z * Unit.Z;
		}
		Result.Central /= Samples;
		Result.Outer /= Samples;
		Result.MeanAbsZ /= Samples;
		Result.Hot /= Samples;
		Result.AxisYX = FMath::Sqrt(SumY2 / FMath::Max(SumX2, UE_DOUBLE_SMALL_NUMBER));
		Result.AxisZX = FMath::Sqrt(SumZ2 / FMath::Max(SumX2, UE_DOUBLE_SMALL_NUMBER));
		return Result;
	}

	UWorld* CreateTestWorld()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false)
			.RequiresHitProxies(false)
			.CreatePhysicsScene(true)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.ShouldSimulatePhysics(false)
			.SetTransactional(false);
		return UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
			ERHIFeatureLevel::Num, &Values);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGalaxyV2LegacyCatalogTest,
	"APS.Generation.GalaxyV2.LegacyCatalogUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGalaxyV2LegacyCatalogTest::RunTest(const FString& Parameters)
{
	using namespace APSGalaxyV2Tests;
	// Rio's accepted spiral (02.10 log: Spiral + E4) and the default menu seed.
	constexpr int32 Seeds[] = {1585025466, 271828};
	constexpr int32 Samples = 48;
	int32 Compared = 0;
	int32 Mismatched = 0;
	for (const int32 Seed : Seeds)
	{
		for (int32 TypeValue = 0; TypeValue <= static_cast<int32>(EGalaxyType::Unknown); ++TypeValue)
		{
			for (int32 ClassValue = 0; ClassValue < FirstNewClassValue; ++ClassValue)
			{
				const FGalaxyCatalogDescriptor Catalog = MakeCatalog(Seed, static_cast<EGalaxyType>(TypeValue),
					static_cast<EGalaxyClass>(ClassValue));
				const double Tolerance = CatalogRadius(Catalog) * 1.0e-9;
				for (int32 Sample = 0; Sample < Samples; ++Sample)
				{
					const int64 Index = SampleIndex(Catalog, Sample, Samples);
					FGalaxyCatalogStarRecord Live;
					APSGalaxyV2TestsFrozen::FRecord Frozen;
					const bool bLive = Catalog.ResolveStar(Index, Live);
					const bool bFrozen = APSGalaxyV2TestsFrozen::ResolveStar(Catalog, Index, Frozen);
					++Compared;
					if (bLive != bFrozen || !Live.GalaxyLocalLocation.Equals(Frozen.Position, Tolerance)
						|| Live.SpectralClass != Frozen.SpectralClass || Live.SpectralSubclass != Frozen.SpectralSubclass
						|| Live.bPotentialStarSystem != Frozen.bPotentialStarSystem
						|| Live.GenerationSeed != Frozen.GenerationSeed
						|| Live.StableId != Catalog.MakeStableStarId(Index))
					{
						if (++Mismatched <= 5)
						{
							AddError(FString::Printf(TEXT("Historic record changed: type=%d class=%d index=%lld"),
								TypeValue, ClassValue, Index));
						}
					}
				}
			}
		}
	}
	TestTrue(TEXT("Historic (type, class) pairs were compared"), Compared > 10000);
	TestEqual(TEXT("Every historic (type, class) pair resolves exactly as before 03.10"), Mismatched, 0);

	// The new per-type defaults reproduce the historic look of their type.
	struct FRoute
	{
		EGalaxyType Type;
		EGalaxyClass NewDefault;
		EGalaxyClass Historic;
	};
	const FRoute Routes[] = {
		{EGalaxyType::Spiral, EGalaxyClass::SpiralSb, EGalaxyClass::E4},
		{EGalaxyType::Spiral, EGalaxyClass::SpiralSb, EGalaxyClass::E0},
		{EGalaxyType::BarredSpiral, EGalaxyClass::BarredSBb, EGalaxyClass::E0},
		{EGalaxyType::Peculiar, EGalaxyClass::PecWarped, EGalaxyClass::E0},
	};
	for (const FRoute& Route : Routes)
	{
		TestEqual(TEXT("Route target is the type default"),
			APSGalaxyMorphology::GetDefaultSubclass(Route.Type), Route.NewDefault);
		bool bSame = true;
		for (const int32 Seed : Seeds)
		{
			const FGalaxyCatalogDescriptor NewCatalog = MakeCatalog(Seed, Route.Type, Route.NewDefault);
			const FGalaxyCatalogDescriptor OldCatalog = MakeCatalog(Seed, Route.Type, Route.Historic);
			for (int32 Sample = 0; Sample < 512; ++Sample)
			{
				const int64 Index = SampleIndex(NewCatalog, Sample, 512);
				FGalaxyCatalogStarRecord New;
				APSGalaxyV2TestsFrozen::FRecord Old;
				bSame &= NewCatalog.ResolveStar(Index, New) && APSGalaxyV2TestsFrozen::ResolveStar(OldCatalog, Index, Old)
					&& New.GalaxyLocalLocation.Equals(Old.Position, CatalogRadius(NewCatalog) * 1.0e-9)
					&& New.SpectralClass == Old.SpectralClass && New.SpectralSubclass == Old.SpectralSubclass
					&& New.GenerationSeed == Old.GenerationSeed;
			}
		}
		TestTrue(FString::Printf(TEXT("Default subclass %d of type %d keeps the historic look"),
			static_cast<int32>(Route.NewDefault), static_cast<int32>(Route.Type)), bSame);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGalaxyV2SubclassTablesTest,
	"APS.Generation.GalaxyV2.SubclassTables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGalaxyV2SubclassTablesTest::RunTest(const FString& Parameters)
{
	TMap<EGalaxyClass, EGalaxyType> Owner;
	for (int32 TypeValue = 0; TypeValue < static_cast<int32>(EGalaxyType::Unknown); ++TypeValue)
	{
		const EGalaxyType Type = static_cast<EGalaxyType>(TypeValue);
		const TConstArrayView<EGalaxyClass> Subclasses = APSGalaxyMorphology::GetSubclasses(Type);
		TestTrue(FString::Printf(TEXT("Type %d offers subclasses"), TypeValue), Subclasses.Num() >= 3);
		TestTrue(FString::Printf(TEXT("Type %d default is offered"), TypeValue),
			Subclasses.Contains(APSGalaxyMorphology::GetDefaultSubclass(Type)));
		for (const EGalaxyClass Class : Subclasses)
		{
			TestFalse(TEXT("A subclass belongs to one type only"), Owner.Contains(Class));
			Owner.Add(Class, Type);
			const APSGalaxyMorphology::FProfile* Profile = APSGalaxyMorphology::FindProfile(Type, Class);
			const bool bHistoricValue = static_cast<int32>(Class) < APSGalaxyV2Tests::FirstNewClassValue;
			TestTrue(TEXT("Historic offered values resolve with the historic code, new ones own a profile"),
				bHistoricValue ? Profile == nullptr : (Profile != nullptr && Profile->Type == Type));
			TestEqual(TEXT("An offered subclass survives a type coercion"),
				APSGalaxyMorphology::CoerceSubclass(Type, Class), Class);
		}
	}
	for (const EGalaxyClass Legacy : {EGalaxyClass::Sa, EGalaxyClass::Sb, EGalaxyClass::Sc, EGalaxyClass::Sd,
		EGalaxyClass::SBa, EGalaxyClass::SBb, EGalaxyClass::SBc, EGalaxyClass::SBd, EGalaxyClass::Unknown})
	{
		TestFalse(TEXT("Legacy spiral classes are no longer offered"), Owner.Contains(Legacy));
	}
	for (const APSGalaxyMorphology::FProfile& Profile : APSGalaxyMorphology::GetProfiles())
	{
		TestTrue(TEXT("Every profile is offered by its own type"),
			APSGalaxyMorphology::IsSubclassOf(Profile.Type, Profile.Class));
		TestTrue(TEXT("Profiles exist only for values appended on 03.10"),
			static_cast<int32>(Profile.Class) >= APSGalaxyV2Tests::FirstNewClassValue);
		TestTrue(TEXT("A profile is unreachable from another type"),
			APSGalaxyMorphology::FindProfile(Profile.Type == EGalaxyType::Spiral
				? EGalaxyType::Elliptical : EGalaxyType::Spiral, Profile.Class) == nullptr);
	}
	TestEqual(TEXT("A type change leaves Rio's Spiral + E4 for the matching default"),
		APSGalaxyMorphology::CoerceSubclass(EGalaxyType::Spiral, EGalaxyClass::E4), EGalaxyClass::SpiralSb);
	TestEqual(TEXT("Elliptical keeps E3"),
		APSGalaxyMorphology::CoerceSubclass(EGalaxyType::Elliptical, EGalaxyClass::E3), EGalaxyClass::E3);
	TestTrue(TEXT("Unknown type offers nothing"), APSGalaxyMorphology::GetSubclasses(EGalaxyType::Unknown).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGalaxyV2SubclassShapeTest,
	"APS.Generation.GalaxyV2.SubclassesChangeShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGalaxyV2SubclassShapeTest::RunTest(const FString& Parameters)
{
	using namespace APSGalaxyV2Tests;
	constexpr int32 Seed = 271828;
	constexpr int32 Samples = 4000;
	TMap<EGalaxyClass, FShapeStats> ShapeByClass;
	for (int32 TypeValue = 0; TypeValue < static_cast<int32>(EGalaxyType::Unknown); ++TypeValue)
	{
		const EGalaxyType Type = static_cast<EGalaxyType>(TypeValue);
		const TConstArrayView<EGalaxyClass> Subclasses = APSGalaxyMorphology::GetSubclasses(Type);
		for (const EGalaxyClass Class : Subclasses)
		{
			const FShapeStats Shape = Measure(MakeCatalog(Seed, Type, Class), Samples);
			ShapeByClass.Add(Class, Shape);
			TestTrue(FString::Printf(TEXT("Class %d stays finite and inside the 1.05 R catalogue envelope"),
				static_cast<int32>(Class)), Shape.bFinite && Shape.MaxRadius <= 1.05);
		}
		// "The subclass actually changes the result": for the same catalogue indices at least a
		// fifth of the stars move by more than 2% of the radius between any two subclasses.
		// (Neighbouring subclasses share their random draws on purpose, so halo stars stay.)
		for (int32 First = 0; First < Subclasses.Num(); ++First)
		{
			for (int32 Second = First + 1; Second < Subclasses.Num(); ++Second)
			{
				const FGalaxyCatalogDescriptor A = MakeCatalog(Seed, Type, Subclasses[First]);
				const FGalaxyCatalogDescriptor B = MakeCatalog(Seed, Type, Subclasses[Second]);
				const double Radius = CatalogRadius(A);
				int32 Moved = 0;
				constexpr int32 PairSamples = 1500;
				for (int32 Sample = 0; Sample < PairSamples; ++Sample)
				{
					const int64 Index = SampleIndex(A, Sample, PairSamples);
					FGalaxyCatalogStarRecord RecordA;
					FGalaxyCatalogStarRecord RecordB;
					A.ResolveStar(Index, RecordA);
					B.ResolveStar(Index, RecordB);
					Moved += FVector::Distance(RecordA.GalaxyLocalLocation, RecordB.GalaxyLocalLocation)
						> Radius * 0.02 ? 1 : 0;
				}
				TestTrue(FString::Printf(TEXT("Subclasses %d and %d of type %d give different galaxies (%d/%d moved)"),
					static_cast<int32>(Subclasses[First]), static_cast<int32>(Subclasses[Second]), TypeValue,
					Moved, PairSamples), Moved * 5 >= PairSamples);
			}
		}
	}

	// The sequences are logical, not just different.
	const auto Central = [&ShapeByClass](const EGalaxyClass Class) { return ShapeByClass.FindRef(Class).Central; };
	const auto Hot = [&ShapeByClass](const EGalaxyClass Class) { return ShapeByClass.FindRef(Class).Hot; };
	TestTrue(TEXT("Spiral bulges shrink from Sa to Sd"),
		Central(EGalaxyClass::SpiralSa) > Central(EGalaxyClass::SpiralSbc)
		&& Central(EGalaxyClass::SpiralSbc) > Central(EGalaxyClass::SpiralSd));
	TestTrue(TEXT("Barred bulges shrink from SBa to SBd"),
		Central(EGalaxyClass::BarredSBa) > Central(EGalaxyClass::BarredSBc)
		&& Central(EGalaxyClass::BarredSBc) > Central(EGalaxyClass::BarredSBd));
	TestTrue(TEXT("Late spirals are younger (more hot stars) than early ones"),
		Hot(EGalaxyClass::SpiralSd) > Hot(EGalaxyClass::SpiralSa) + 0.02
		&& Hot(EGalaxyClass::BarredSBd) > Hot(EGalaxyClass::BarredSBa) + 0.02);
	TestTrue(TEXT("E7 is flatter than E0"),
		ShapeByClass.FindRef(EGalaxyClass::E7).AxisYX < ShapeByClass.FindRef(EGalaxyClass::E0).AxisYX - 0.3);
	TestTrue(TEXT("cD is more centrally concentrated than E0"),
		Central(EGalaxyClass::cD) > Central(EGalaxyClass::E0) + 0.05);
	TestTrue(TEXT("dE is flatter than E0"),
		ShapeByClass.FindRef(EGalaxyClass::dE).AxisZX < ShapeByClass.FindRef(EGalaxyClass::E0).AxisZX - 0.2);
	TestTrue(TEXT("Ellipticals are old: cD has fewer hot stars than a young irregular"),
		Hot(EGalaxyClass::cD) + 0.03 < Hot(EGalaxyClass::Im));
	TestTrue(TEXT("The ring galaxy keeps most light away from the centre"),
		ShapeByClass.FindRef(EGalaxyClass::PecRing).Outer > ShapeByClass.FindRef(EGalaxyClass::PecRing).Central + 0.2);
	TestTrue(TEXT("The polar ring rises out of the host plane"),
		ShapeByClass.FindRef(EGalaxyClass::PecPolarRing).AxisZX > ShapeByClass.FindRef(EGalaxyClass::S0).AxisZX + 0.3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGalaxyV2BatchTest,
	"APS.Generation.GalaxyV2.BatchResolveAndDensity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGalaxyV2BatchTest::RunTest(const FString& Parameters)
{
	using namespace APSGalaxyV2Tests;
	for (const EGalaxyClass Class : {EGalaxyClass::SpiralSc, EGalaxyClass::SpiralSb})
	{
		const FGalaxyCatalogDescriptor Catalog = MakeCatalog(1585025466, EGalaxyType::Spiral, Class);
		const APSCanonicalStellarProjection::FNestedCatalogPermutation Order =
			APSCanonicalStellarProjection::MakeNestedCatalogPermutation(Catalog.GenerationSeed, Catalog.ModeledStarCount);
		TArray<FGalaxyCatalogStarRecord> Batch;
		const int32 Resolved = APSGalaxyCatalogBatch::ResolveStars(Catalog, Order, 0, 40000, Batch);
		TestEqual(TEXT("Parallel batch resolves the whole range"), Resolved, 40000);
		bool bSame = Batch.Num() == 40000;
		const double Tolerance = CatalogRadius(Catalog) * 1.0e-9;
		for (int32 Ordinal = 0; bSame && Ordinal < Batch.Num(); Ordinal += 7)
		{
			FGalaxyCatalogStarRecord Serial;
			bSame &= Catalog.ResolveStar(Order.Resolve(Ordinal), Serial)
				&& Serial.CatalogIndex == Batch[Ordinal].CatalogIndex
				&& Serial.StableId == Batch[Ordinal].StableId
				&& Serial.GalaxyLocalLocation.Equals(Batch[Ordinal].GalaxyLocalLocation, Tolerance)
				&& Serial.SpectralClass == Batch[Ordinal].SpectralClass;
		}
		TestTrue(TEXT("Parallel batch equals one-by-one resolution"), bSame);
		TArray<FGalaxyCatalogStarRecord> Window;
		APSGalaxyCatalogBatch::ResolveStars(Catalog, Order, 25000, 100, Window);
		TestTrue(TEXT("A later window is the same nested prefix slice"),
			Window.Num() == 100 && Window[0].StableId == Batch[25000].StableId
			&& Window[99].StableId == Batch[25099].StableId);
	}

	using APSGalaxyMorphology::GetDensityCompensation;
	const APSGalaxyMorphology::FDensityCompensation Historic = GetDensityCompensation(25000, 25000);
	TestTrue(TEXT("Historic budgets render exactly as before"),
		Historic.EmissionScale == 1.0 && Historic.RadiusScale == 1.0
		&& GetDensityCompensation(1000000, 0).EmissionScale == 1.0
		&& GetDensityCompensation(1800, 1800).RadiusScale == 1.0);
	const APSGalaxyMorphology::FDensityCompensation Mid = GetDensityCompensation(100000, 1800);
	const APSGalaxyMorphology::FDensityCompensation Million = GetDensityCompensation(1000000, 1800);
	TestTrue(TEXT("Denser placements get dimmer and finer stars"),
		Million.EmissionScale < Mid.EmissionScale && Mid.EmissionScale < 1.0
		&& Million.RadiusScale <= Mid.RadiusScale && Million.RadiusScale >= 0.18);
	TestTrue(TEXT("A million stars stay within a few times the 1800-star light"),
		1000000.0 * Million.EmissionScale < 1800.0 * 6.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSClusterV2FormationTest,
	"APS.Generation.ClusterV2.NewFormations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSClusterV2FormationTest::RunTest(const FString& Parameters)
{
	UStarClusterGenerator* Generator = NewObject<UStarClusterGenerator>();
	const EStarClusterType NewTypes[] = {
		EStarClusterType::YoungAssociation, EStarClusterType::MovingGroup, EStarClusterType::SuperStarCluster,
		EStarClusterType::EmbeddedCluster, EStarClusterType::DoubleCluster};
	constexpr int32 Samples = 4000;
	TMap<EStarClusterType, double> CoreShare;
	for (const EStarClusterType Type : NewTypes)
	{
		const FVector Bounds = Generator->GetStarClusterBoundsByRange(Type);
		TestTrue(TEXT("New formation owns its bounds"), Bounds != FVector(100000.0));
		bool bInside = true;
		bool bDeterministic = true;
		bool bSeeded = false;
		int32 Core = 0;
		for (int32 Index = 0; Index < Samples; ++Index)
		{
			const double Mass = 0.3 + (Index % 17) * 0.4;
			FVector First;
			FVector Second;
			FVector OtherSeed;
			FMath::RandInit(Index);
			TestTrue(TEXT("New formation is seeded"),
				UStarClusterGenerator::SampleSeededFormation(Type, 4242, Index, Bounds, Mass, First));
			FMath::RandInit(Index * 7 + 1);
			UStarClusterGenerator::SampleSeededFormation(Type, 4242, Index, Bounds, Mass, Second);
			UStarClusterGenerator::SampleSeededFormation(Type, 4243, Index, Bounds, Mass, OtherSeed);
			bDeterministic &= First == Second;
			bSeeded |= !First.Equals(OtherSeed, 1.0);
			bInside &= FMath::Abs(First.X) <= Bounds.X * 0.5 + 1.0 && FMath::Abs(First.Y) <= Bounds.Y * 0.5 + 1.0
				&& FMath::Abs(First.Z) <= Bounds.Z * 0.5 + 1.0 && !First.ContainsNaN();
			Core += (First / Bounds).Size() < 0.08 ? 1 : 0;
		}
		TestTrue(TEXT("New formation stays inside its bounds (projection envelope)"), bInside);
		TestTrue(TEXT("New formation ignores the global RNG"), bDeterministic);
		TestTrue(TEXT("New formation depends on the cluster seed"), bSeeded);
		CoreShare.Add(Type, static_cast<double>(Core) / Samples);
	}
	TestTrue(TEXT("A super star cluster is far denser in the core than an association or a moving group"),
		CoreShare.FindRef(EStarClusterType::SuperStarCluster) > 2.0 * CoreShare.FindRef(EStarClusterType::YoungAssociation)
		&& CoreShare.FindRef(EStarClusterType::SuperStarCluster) > 2.0 * CoreShare.FindRef(EStarClusterType::MovingGroup));
	TestTrue(TEXT("A double cluster has an empty middle"),
		CoreShare.FindRef(EStarClusterType::DoubleCluster) < 0.05);

	FVector Ignored;
	for (const EStarClusterType Historic : {EStarClusterType::OpenCluster, EStarClusterType::GlobularCluster,
		EStarClusterType::Supercluster, EStarClusterType::Nebula, EStarClusterType::Unknown,
		EStarClusterType::ElongatedStream, EStarClusterType::RingArc, EStarClusterType::Hourglass})
	{
		TestFalse(TEXT("Historic formations keep their own code"),
			UStarClusterGenerator::SampleSeededFormation(Historic, 4242, 0, FVector(100000.0), 1.0, Ignored));
	}

	for (int32 Draw = 0; Draw < 32; ++Draw)
	{
		const int32 Colossal = Generator->GetStarsAmountByRange(EStarClusterSize::Colossal);
		const int32 Giant = Generator->GetStarsAmountByRange(EStarClusterSize::Giant);
		TestTrue(TEXT("Colossal clusters are bigger than Giant ones"), Colossal >= 50000 && Colossal <= 100000);
		TestTrue(TEXT("Giant keeps its historic range"), Giant >= 25000 && Giant <= 50000);
	}
	TestEqual(TEXT("Unknown size keeps zero"), Generator->GetStarsAmountByRange(EStarClusterSize::Unknown), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSClusterV2LegacyFormationTest,
	"APS.Generation.ClusterV2.LegacyFormationsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSClusterV2LegacyFormationTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGalaxyV2Tests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	AStarCluster* Cluster = World->SpawnActor<AStarCluster>();
	UStarClusterGenerator* Generator = NewObject<UStarClusterGenerator>();
	if (TestNotNull(TEXT("Cluster actor"), Cluster))
	{
		for (const EStarClusterType Type : {EStarClusterType::OpenCluster, EStarClusterType::GlobularCluster,
			EStarClusterType::Supercluster, EStarClusterType::Nebula, EStarClusterType::Unknown,
			EStarClusterType::ElongatedStream, EStarClusterType::RingArc, EStarClusterType::Hourglass})
		{
			Cluster->ClusterType = Type;
			Cluster->ClusterBounds = Generator->GetStarClusterBoundsByRange(Type);
			Cluster->StarAmount = 1600;
			Cluster->GenerationSeed = 2030576230;
			for (const float StarRadius : {0.6f, 14.0f})
			{
				TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();
				StarModel->Radius = StarRadius;
				StarModel->Mass = 1.0f;
				TArray<FVector> Frozen;
				FMath::RandInit(9001);
				for (int32 Index = 0; Index < 300; ++Index)
				{
					Frozen.Add(APSGalaxyV2TestsFrozen::ClusterPosition(Index, Cluster->StarAmount,
						Cluster->GenerationSeed, Type, Cluster->ClusterBounds, StarModel->Radius) * 100);
				}
				FMath::RandInit(9001);
				bool bSame = true;
				for (int32 Index = 0; Index < 300; ++Index)
				{
					bSame &= Generator->CalculateStarPosition(Index, Cluster, StarModel).Equals(
						Frozen[Index], Cluster->ClusterBounds.GetAbsMax() * 1.0e-6);
				}
				TestTrue(FString::Printf(TEXT("Historic formation %d is unchanged (radius %.1f)"),
					static_cast<int32>(Type), StarRadius), bSame);
			}
		}
	}
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSClusterV2SizingTest,
	"APS.Generation.ClusterV2.SizeShowsInExtentAndLiveSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSClusterV2SizingTest::RunTest(const FString& Parameters)
{
	const EStarClusterSize Sizes[] = {EStarClusterSize::Tiny, EStarClusterSize::Small, EStarClusterSize::Medium,
		EStarClusterSize::Large, EStarClusterSize::Giant, EStarClusterSize::Colossal};
	for (int32 Index = 1; Index < static_cast<int32>(UE_ARRAY_COUNT(Sizes)); ++Index)
	{
		TestTrue(TEXT("A bigger size draws a bigger cluster"), UStarClusterGenerator::GetSizeExtentFactor(Sizes[Index])
			> UStarClusterGenerator::GetSizeExtentFactor(Sizes[Index - 1]));
		TestTrue(TEXT("A bigger size shows more live systems"), UStarClusterGenerator::GetPreviewFormationBudget(Sizes[Index])
			> UStarClusterGenerator::GetPreviewFormationBudget(Sizes[Index - 1]));
	}
	TestEqual(TEXT("Giant keeps the historic extent (sealed datasets compose identically)"),
		UStarClusterGenerator::GetSizeExtentFactor(EStarClusterSize::Giant), 1.0);
	TestTrue(TEXT("Tiny shows every one of its systems"),
		UStarClusterGenerator::GetPreviewFormationBudget(EStarClusterSize::Tiny) >= 500);
	UStarClusterGenerator* Generator = NewObject<UStarClusterGenerator>();
	const FVector Bounds = Generator->GetStarClusterBoundsByRange(EStarClusterType::GlobularCluster);
	TestEqual(TEXT("Logical half extent mirrors the globular envelope of GenerateStarCluster"),
		UStarClusterGenerator::GetLogicalHalfExtent(Bounds, EStarClusterType::GlobularCluster),
		FMath::Max(Bounds.GetAbsMax() * 50.0, (Bounds.GetAbsMax() * 0.5 + 1000.0 * 100.0) * 100.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldRollSkyTest,
	"APS.Generation.GalaxyV2.RegenerateRollsGalaxyAndCluster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldRollSkyTest::RunTest(const FString& Parameters)
{
	TSet<EGalaxyType> GalaxyTypes;
	TSet<EStarClusterType> ClusterTypes;
	int32 Archetypes = 0;
	bool bValidClasses = true;
	bool bSaneSizes = true;
	bool bDeterministic = true;
	constexpr int32 Rolls = 300;
	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		const int32 Seed = 1 + Roll * 7919;
		UGeneratedWorld* First = NewObject<UGeneratedWorld>();
		UGeneratedWorld* Second = NewObject<UGeneratedWorld>();
		const APSWorldRoll::FResult Result = APSWorldRoll::Apply(*First, Seed, APSWorldRoll::EScope::System);
		APSWorldRoll::Apply(*Second, Seed, APSWorldRoll::EScope::System);
		bDeterministic &= First->GalaxyType == Second->GalaxyType && First->GalaxyClass == Second->GalaxyClass
			&& First->GalaxySize == Second->GalaxySize && First->StarClusterType == Second->StarClusterType
			&& First->StarClusterSize == Second->StarClusterSize
			&& First->StarClusterComposition == Second->StarClusterComposition
			&& First->GalaxyPlacedStarCount == Second->GalaxyPlacedStarCount;
		bValidClasses &= APSGalaxyMorphology::IsSubclassOf(First->GalaxyType, First->GalaxyClass);
		bDeterministic &= First->GalaxyStarPopulation == Second->GalaxyStarPopulation
			&& First->GalaxyStarComposition == Second->GalaxyStarComposition;
		bSaneSizes &= First->GalaxyStarPopulation != EStarClusterPopulation::Unknown
			&& First->GalaxyStarComposition != EStarClusterComposition::Unknown;
		bSaneSizes &= First->StarClusterSize != EStarClusterSize::Colossal && First->StarClusterSize != EStarClusterSize::Tiny
			&& First->StarClusterSize != EStarClusterSize::Unknown && First->StarClusterType != EStarClusterType::Unknown
			&& First->GalaxySize >= 100 && First->GalaxySize <= 600
			&& First->GalaxyPlacedStarCount >= APSGalaxyMorphology::PreviewReferenceBudget
			&& First->GalaxyPlacedStarCount <= APSGalaxyMorphology::MaxPlacedStars;
		GalaxyTypes.Add(First->GalaxyType);
		ClusterTypes.Add(First->StarClusterType);
		Archetypes += Result.Summary.Contains(TEXT("sky=standard")) ? 0 : 1;
	}
	TestTrue(TEXT("REGENERATE is deterministic for a seed"), bDeterministic);
	TestTrue(TEXT("The rolled class always belongs to the rolled type (never a legacy class)"), bValidClasses);
	TestTrue(TEXT("Rolls stay within Small..Giant (Colossal is a deliberate choice) and sane galaxy sizes"), bSaneSizes);
	TestEqual(TEXT("Every galaxy type appears"), GalaxyTypes.Num(), static_cast<int32>(EGalaxyType::Unknown));
	TestTrue(TEXT("Most cluster formations appear, the new ones included"), ClusterTypes.Num() >= 10
		&& ClusterTypes.Contains(EStarClusterType::YoungAssociation) && ClusterTypes.Contains(EStarClusterType::SuperStarCluster));
	TestTrue(TEXT("Paired archetypes appear now and then, but not often"), Archetypes > 10 && Archetypes < Rolls / 4);

	UGeneratedWorld* PlanetOnly = NewObject<UGeneratedWorld>();
	const EGalaxyType BeforeType = PlanetOnly->GalaxyType;
	const EStarClusterType BeforeCluster = PlanetOnly->StarClusterType;
	APSWorldRoll::Apply(*PlanetOnly, 4242, APSWorldRoll::EScope::PlanetOnly);
	TestTrue(TEXT("The PLANET route leaves the galaxy and cluster alone"),
		PlanetOnly->GalaxyType == BeforeType && PlanetOnly->StarClusterType == BeforeCluster);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGalaxyV2StarMixTest,
	"APS.Generation.GalaxyV2.StarMixPopulationComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGalaxyV2StarMixTest::RunTest(const FString& Parameters)
{
	using namespace APSGalaxyV2Tests;
	const FGalaxyCatalogDescriptor Historic = MakeCatalog(1585025466, EGalaxyType::Spiral, EGalaxyClass::SpiralSb);
	const auto WithMix = [&Historic](const EStarClusterPopulation Population, const EStarClusterComposition Composition)
	{
		FGalaxyCatalogDescriptor Catalog = Historic;
		Catalog.StarPopulation = static_cast<uint8>(Population);
		Catalog.StarComposition = static_cast<uint8>(Composition);
		return Catalog;
	};
	const FGalaxyCatalogDescriptor Red = WithMix(EStarClusterPopulation::AllSequenses, EStarClusterComposition::OnlyRed);
	const FGalaxyCatalogDescriptor Giants = WithMix(EStarClusterPopulation::Giants, EStarClusterComposition::AllSpectral);
	const FGalaxyCatalogDescriptor Dwarfs = WithMix(EStarClusterPopulation::Dwarfs, EStarClusterComposition::AllSpectral);
	constexpr int32 Samples = 2000;
	bool bHistoricUnit = true;
	bool bPositionsKept = true;
	bool bAllRed = true;
	double GiantScale = 0.0;
	double DwarfScale = 0.0;
	int32 BrownDwarfs = 0;
	for (int32 Sample = 0; Sample < Samples; ++Sample)
	{
		const int64 Index = SampleIndex(Historic, Sample, Samples);
		FGalaxyCatalogStarRecord Base, RedRecord, GiantRecord, DwarfRecord;
		if (!Historic.ResolveStar(Index, Base) || !Red.ResolveStar(Index, RedRecord)
			|| !Giants.ResolveStar(Index, GiantRecord) || !Dwarfs.ResolveStar(Index, DwarfRecord))
		{
			AddError(TEXT("A catalogue record failed to resolve"));
			return false;
		}
		bHistoricUnit &= Base.RadiusScale == 1.0f;
		bPositionsKept &= Base.GalaxyLocalLocation == RedRecord.GalaxyLocalLocation
			&& Base.GalaxyLocalLocation == GiantRecord.GalaxyLocalLocation && Base.StableId == DwarfRecord.StableId;
		bAllRed &= RedRecord.SpectralClass == ESpectralClass::M && RedRecord.RadiusScale == 1.0f;
		GiantScale += GiantRecord.RadiusScale;
		DwarfScale += DwarfRecord.RadiusScale;
		BrownDwarfs += DwarfRecord.SpectralClass == ESpectralClass::L || DwarfRecord.SpectralClass == ESpectralClass::T
			|| DwarfRecord.SpectralClass == ESpectralClass::Y;
	}
	TestTrue(TEXT("The historic mix keeps every record unscaled"), bHistoricUnit);
	TestTrue(TEXT("POPULATION / COMPOSITION never move a star or change its identity"), bPositionsKept);
	TestTrue(TEXT("Only Red makes every catalogue star an M star"), bAllRed);
	TestTrue(TEXT("Mostly Giants enlarges the stars"), GiantScale / Samples > 4.0);
	TestTrue(TEXT("Mostly Dwarfs shrinks the stars"), DwarfScale / Samples < 1.0);
	TestTrue(TEXT("Mostly Dwarfs includes brown dwarfs (L/T/Y)"), BrownDwarfs > Samples / 50);
	FGalaxyCatalogStarRecord First, Second;
	TestTrue(TEXT("The mix is deterministic"), Giants.ResolveStar(12345, First) && Giants.ResolveStar(12345, Second)
		&& First.SpectralClass == Second.SpectralClass && First.RadiusScale == Second.RadiusScale);
	return true;
}

#endif
