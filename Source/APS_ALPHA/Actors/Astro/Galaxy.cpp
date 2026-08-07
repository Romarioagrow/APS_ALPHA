#include "Galaxy.h"

namespace APSGalaxyCatalog
{
	uint64 Mix64(uint64 Value)
	{
		// SplitMix64 finalizer. Unlike FRandomStream's 32-bit seed this remains
		// collision-resistant for the hundreds of millions of logical records a
		// galaxy catalog is expected to expose.
		Value ^= Value >> 30;
		Value *= 0xbf58476d1ce4e5b9ull;
		Value ^= Value >> 27;
		Value *= 0x94d049bb133111ebull;
		Value ^= Value >> 31;
		return Value;
	}

	uint64 HashCatalogIndex(const int32 Seed, const int64 CatalogIndex, const uint64 Salt = 0)
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
			// Preserve 53 random bits, matching the precision of a double mantissa.
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

	FVector RandomUnitVector(FCatalogRandomStream& Stream)
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

	ESpectralClass ChooseSpectralClass(const float Value)
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
}

FGuid FGalaxyCatalogDescriptor::MakeStableStarId(const int64 CatalogIndex) const
{
	const uint64 HashA64 = APSGalaxyCatalog::HashCatalogIndex(GenerationSeed, CatalogIndex,
		0x47414c4158595f41ull); // GALAXY_A
	const uint64 HashB64 = APSGalaxyCatalog::HashCatalogIndex(
		GenerationSeed ^ (static_cast<int32>(GalaxyType) << 8) ^ static_cast<int32>(GalaxyClass),
		CatalogIndex, 0x47414c4158595f42ull); // GALAXY_B
	const uint32 HashA = static_cast<uint32>(HashA64);
	const uint32 HashB = static_cast<uint32>(HashA64 >> 32);
	const uint32 HashC = static_cast<uint32>(HashB64);
	const uint32 HashD = static_cast<uint32>(HashB64 >> 32);
	return FGuid(HashA, HashB, HashC, HashD);
}

bool FGalaxyCatalogDescriptor::ResolveStar(const int64 CatalogIndex, FGalaxyCatalogStarRecord& OutRecord) const
{
	if (CatalogIndex < 0 || CatalogIndex >= ModeledStarCount)
	{
		return false;
	}

	const uint64 RecordHash = APSGalaxyCatalog::HashCatalogIndex(GenerationSeed, CatalogIndex);
	APSGalaxyCatalog::FCatalogRandomStream Stream(RecordHash);
	const double DensityScale = FMath::Sqrt(10.0 / FMath::Clamp(StarDensity, 0.01, 1000.0));
	const double GalaxyRadius = FMath::Max(50000.0, static_cast<double>(FMath::Max(GalaxySize, 1)) * 50000.0)
		* DensityScale;
	FVector Position = FVector::ZeroVector;

	// A readable galaxy preview needs more than a razor-thin disk. Real spiral and
	// lenticular galaxies also own a central bulge and an extended stellar halo;
	// keeping those populations explicit makes the home cluster at the catalog
	// origin feel embedded in one continuous 360-degree parent volume. The indexed
	// catalog remains deterministic and the dominant morphology still comes from
	// the selected galaxy type below.
	const bool bDiskLikeGalaxy = GalaxyType == EGalaxyType::Lenticular
		|| GalaxyType == EGalaxyType::Spiral
		|| GalaxyType == EGalaxyType::BarredSpiral
		|| GalaxyType == EGalaxyType::Peculiar;
	const double StructureSelector = static_cast<double>((RecordHash >> 24) & 0xffffull) / 65535.0;
	if (bDiskLikeGalaxy && StructureSelector < 0.18)
	{
		APSGalaxyCatalog::FCatalogRandomStream HaloStream(
			APSGalaxyCatalog::HashCatalogIndex(GenerationSeed, CatalogIndex,
				0x47414c5f48414c4full)); // GAL_HALO
		const double HaloRadius = FMath::Pow(HaloStream.Fraction(), 0.55) * GalaxyRadius;
		Position = APSGalaxyCatalog::RandomUnitVector(HaloStream) * HaloRadius;
	}
	else if (bDiskLikeGalaxy && StructureSelector < 0.34)
	{
		APSGalaxyCatalog::FCatalogRandomStream BulgeStream(
			APSGalaxyCatalog::HashCatalogIndex(GenerationSeed, CatalogIndex,
				0x47414c5f42554c47ull)); // GAL_BULG
		const double BulgeRadius = FMath::Pow(BulgeStream.Fraction(), 1.0 / 3.0)
			* GalaxyRadius * 0.22;
		Position = APSGalaxyCatalog::RandomUnitVector(BulgeStream) * BulgeRadius;
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
				Position = APSGalaxyCatalog::RandomUnitVector(Stream) * Radius;
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
				Position = FVector(X, Y,
					Stream.Range(-1.0, 1.0) * GalaxyRadius * 0.025 * (1.15 - RadiusAlpha));
			}
			break;
		case EGalaxyType::Irregular:
			{
				APSGalaxyCatalog::FCatalogRandomStream LobeStream(
					APSGalaxyCatalog::HashCatalogIndex(GenerationSeed, static_cast<int64>(RecordHash % 7ull),
						0x4952524547554c52ull)); // IRREGULR
				const FVector LobeCenter = APSGalaxyCatalog::RandomUnitVector(LobeStream)
					* LobeStream.Range(0.05, 0.55) * GalaxyRadius;
				Position = LobeCenter + APSGalaxyCatalog::RandomUnitVector(Stream)
					* FMath::Pow(Stream.Fraction(), 1.8) * GalaxyRadius * 0.48;
			}
			break;
		case EGalaxyType::Peculiar:
		default:
			{
				const double Angle = Stream.Range(0.0, UE_TWO_PI);
				const double Radius = GalaxyRadius * Stream.Range(0.28, 1.0);
				Position = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
					FMath::Sin(Angle * 2.0) * GalaxyRadius * 0.18
					+ Stream.Range(-1.0, 1.0) * GalaxyRadius * 0.06);
			}
			break;
		}
	}

	OutRecord.StableId = MakeStableStarId(CatalogIndex);
	OutRecord.CatalogIndex = CatalogIndex;
	OutRecord.GalaxyLocalLocation = Position;
	OutRecord.GenerationSeed = FMath::Max(1, static_cast<int32>(RecordHash & 0x7fffffffull));
	OutRecord.SpectralClass = APSGalaxyCatalog::ChooseSpectralClass(static_cast<float>(Stream.Fraction()));
	OutRecord.SpectralSubclass = Stream.Range(0, 9);
	OutRecord.bPotentialStarSystem = Stream.Fraction() < 0.82;
	return true;
}

AGalaxy::AGalaxy()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);
	StarMeshInstances->NumCustomDataFloats = 6;
	StarMeshInstances->bAutoRebuildTreeOnInstanceChanges = false;
	StarMeshInstances->bUseTranslatedInstanceSpace = true;
	StarMeshInstances->bEnableDensityScaling = false;
	StarMeshInstances->bNeverDistanceCull = true;
	StarMeshInstances->SetCullDistances(0, 0);
	StarMeshInstances->bDisableCollision = true;
	StarMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarMeshInstances->SetGenerateOverlapEvents(false);
	StarMeshInstances->SetCanEverAffectNavigation(false);
	StarMeshInstances->bEvaluateWorldPositionOffset = false;
	StarMeshInstances->bWorldPositionOffsetWritesVelocity = false;
	StarMeshInstances->SetCastShadow(false);
	StarMeshInstances->bAffectDynamicIndirectLighting = false;
	StarMeshInstances->bAffectDistanceFieldLighting = false;
	StarMeshInstances->SetReceivesDecals(false);
}

bool AGalaxy::GetCatalogStarRecord(const int64 CatalogIndex, FGalaxyCatalogStarRecord& OutRecord) const
{
	return StarCatalog.ResolveStar(CatalogIndex, OutRecord);
}
