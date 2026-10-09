#include "StarClusterGenerator.h"
#include "APSHashStream.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"

namespace APSClusterFormations
{
	// Rio 03.10: the formations appended on 03.10. Positions are fractions of the cluster bounds
	// (each axis within +-0.5) until the final multiply; every stream is keyed by (seed, index).
	using APSHashStream::FStream;

	constexpr uint64 SaltStar = 0x4356325f53544152ull; // CV2_STAR
	constexpr uint64 SaltGroup = 0x4356325f47525550ull; // CV2_GRUP
	constexpr uint64 SaltFilament = 0x4356325f46494c41ull; // CV2_FILA

	/** Bell noise per axis, each component in [-1, 1]. */
	FVector Bell3(FStream& Stream)
	{
		const double X = Stream.Bell();
		const double Y = Stream.Bell();
		const double Z = Stream.Bell();
		return FVector(X, Y, Z) / 1.5;
	}

	/** Truncated Plummer radius: scale A, cut at Cut (both in bounds fractions). */
	double PlummerRadius(FStream& Stream, const double A, const double Cut)
	{
		const double MaxFraction = FMath::Pow(Cut * Cut / (Cut * Cut + A * A), 1.5);
		const double Fraction = FMath::Max(Stream.U() * MaxFraction, 1.0e-12);
		return A / FMath::Sqrt(FMath::Max(FMath::Pow(Fraction, -2.0 / 3.0) - 1.0, 1.0e-12));
	}

	/** Massive stars sink towards the centre of bound, dense clusters. */
	double MassSegregation(const double StarMass)
	{
		return StarMass > 3.0 ? 0.55 : StarMass > 1.5 ? 0.8 : 1.0;
	}

	/** OB association: 5-8 loose, unbound sub-groups in a flattened field population. */
	FVector YoungAssociation(FStream& Stream, const int32 Seed, const FVector& Bounds)
	{
		const int32 Groups = 5 + static_cast<int32>(APSHashStream::Key(Seed, 0, SaltGroup) % 4ull);
		if (Stream.U() < 0.72)
		{
			FStream Group(APSHashStream::Key(Seed, 1 + Stream.Index(Groups), SaltGroup));
			// One draw per statement: argument evaluation order is unspecified in C++.
			const double CenterX = Group.Range(-0.30, 0.30);
			const double CenterY = Group.Range(-0.26, 0.26);
			const double CenterZ = Group.Range(-0.14, 0.14);
			const FVector Center(CenterX, CenterY, CenterZ);
			const double Size = Group.Range(0.05, 0.11);
			return (Center + Bell3(Stream) * FVector(Size, Size * 0.9, Size * 0.6)) * Bounds;
		}
		return Bell3(Stream) * FVector(0.40, 0.34, 0.22) * Bounds;
	}

	/** Moving group: a sparse, elongated co-moving cloud with a faint core and a gentle S-bend. */
	FVector MovingGroup(FStream& Stream, const FVector& Bounds)
	{
		if (Stream.U() < 0.15)
		{
			return Bell3(Stream) * FVector(0.06, 0.06, 0.05) * Bounds;
		}
		const double T = Stream.Bell() / 1.5;
		const double Y = (Stream.Bell() / 1.5 * 0.30 + 0.12 * FMath::Sin(T * UE_DOUBLE_PI))
			* (1.0 - 0.4 * T * T);
		const double Z = Stream.Bell() / 1.5 * 0.30;
		return FVector(T * 0.46, Y, Z) * Bounds;
	}

	/** Super star cluster: a very dense Plummer core with mass segregation and a wide halo. */
	FVector SuperStarCluster(FStream& Stream, const FVector& Bounds, const double StarMass)
	{
		const double Radius = PlummerRadius(Stream, 0.035, 0.47) * MassSegregation(StarMass);
		return Stream.UnitVector() * Radius * Bounds;
	}

	/** Embedded cluster: a dense hub fed by 4-6 gas filaments that young stars still trace. */
	FVector EmbeddedCluster(FStream& Stream, const int32 Seed, const FVector& Bounds, const double StarMass)
	{
		if (Stream.U() < 0.40)
		{
			const FVector Direction = Stream.UnitVector();
			return Direction * (PlummerRadius(Stream, 0.05, 0.25) * MassSegregation(StarMass)) * Bounds;
		}
		const int32 Filaments = 4 + static_cast<int32>(APSHashStream::Key(Seed, 0, SaltFilament) % 3ull);
		FStream Filament(APSHashStream::Key(Seed, 1 + Stream.Index(Filaments), SaltFilament));
		FVector Direction = Filament.UnitVector();
		Direction.Z *= 0.35;
		Direction = Direction.GetSafeNormal();
		const FVector BendSeed = Filament.UnitVector();
		const FVector Bend = (BendSeed - Direction * FVector::DotProduct(BendSeed, Direction)).GetSafeNormal() * 0.06;
		const double Length = Filament.Range(0.28, 0.38);
		const double T = 0.08 + 0.92 * FMath::Pow(Stream.U(), 1.3);
		const double Width = 0.012 + 0.025 * T;
		return (Direction * (Length * T) + Bend * (4.0 * T * (1.0 - T)) + Bell3(Stream) * Width) * Bounds;
	}

	/** Double cluster (h and chi Persei): two round open clusters side by side in a shared halo. */
	FVector DoubleCluster(FStream& Stream, const FVector& Bounds, const double StarMass)
	{
		const double Selector = Stream.U();
		if (Selector < 0.10)
		{
			return Bell3(Stream) * FVector(0.40, 0.30, 0.25) * Bounds;
		}
		const bool bFirst = Selector < 0.55;
		const FVector Center = bFirst
			? FVector(-0.24 * Bounds.X, -0.04 * Bounds.Y, 0.0)
			: FVector(0.24 * Bounds.X, 0.05 * Bounds.Y, 0.02 * Bounds.Z);
		// Round in physical space: scale by the smallest bound, not per axis.
		const double Scale = FMath::Min3(Bounds.X, Bounds.Y, Bounds.Z);
		const double Radius = PlummerRadius(Stream, bFirst ? 0.050 : 0.062, 0.20) * MassSegregation(StarMass);
		return Center + Stream.UnitVector() * (Radius * Scale);
	}
}

bool UStarClusterGenerator::SampleSeededFormation(const EStarClusterType ClusterType, const int32 GenerationSeed,
	const int32 StarIndex, const FVector& ClusterBounds, const double StarMass, FVector& OutPosition)
{
	APSClusterFormations::FStream Stream(APSHashStream::Key(GenerationSeed, StarIndex, APSClusterFormations::SaltStar));
	const FVector Bounds = ClusterBounds.GetAbs();
	switch (ClusterType)
	{
	case EStarClusterType::YoungAssociation:
		OutPosition = APSClusterFormations::YoungAssociation(Stream, GenerationSeed, Bounds);
		return true;
	case EStarClusterType::MovingGroup:
		OutPosition = APSClusterFormations::MovingGroup(Stream, Bounds);
		return true;
	case EStarClusterType::SuperStarCluster:
		OutPosition = APSClusterFormations::SuperStarCluster(Stream, Bounds, StarMass);
		return true;
	case EStarClusterType::EmbeddedCluster:
		OutPosition = APSClusterFormations::EmbeddedCluster(Stream, GenerationSeed, Bounds, StarMass);
		return true;
	case EStarClusterType::DoubleCluster:
		OutPosition = APSClusterFormations::DoubleCluster(Stream, Bounds, StarMass);
		return true;
	default:
		return false;
	}
}

bool UStarClusterGenerator::SampleSeededStreamFormation(const EStarClusterType ClusterType, const int32 GenerationSeed,
	const int32 FormationIndex, const int32 FormationCount, const FVector& ClusterBounds, FVector& OutPosition)
{
	// Rio 09.10 (playtest 30): the same formulas as CalculateStarPosition's ElongatedStream / Hourglass cases; only the
	// random draws come from the cluster's own stream. One draw per statement (argument order is unspecified in C++).
	constexpr uint64 SaltHistoricScatter = 0x4356325f48495354ull; // CV2_HIST
	APSClusterFormations::FStream Stream(APSHashStream::Key(GenerationSeed, FormationIndex, SaltHistoricScatter));
	switch (ClusterType)
	{
	case EStarClusterType::ElongatedStream:
		{
			const double NormalizedIndex = FormationCount > 1
				? FMath::Clamp(static_cast<double>(FormationIndex) / static_cast<double>(FormationCount - 1), 0.0, 1.0) : 0.5;
			const double X = FMath::Lerp(-ClusterBounds.X * 0.5, ClusterBounds.X * 0.5, NormalizedIndex);
			const double Taper = 0.2 + 0.8 * FMath::Sin(PI * NormalizedIndex);
			const double ScatterY = Stream.Range(-ClusterBounds.Y * 0.5, ClusterBounds.Y * 0.5);
			const double ScatterZ = Stream.Range(-ClusterBounds.Z * 0.5, ClusterBounds.Z * 0.5);
			const double Y = ScatterY * Taper
				+ FMath::Sin(NormalizedIndex * UE_TWO_PI * 2.0) * ClusterBounds.Y * 0.16;
			OutPosition = FVector(X, Y, ScatterZ * Taper) * 100;
			return true;
		}
	case EStarClusterType::Hourglass:
		{
			const double SignedHeight = Stream.Range(-1.0, 1.0);
			const double RadiusFraction = FMath::Sqrt(Stream.U());
			const double Angle = Stream.U() * UE_TWO_PI;
			const double LobeRadius = FMath::Lerp(
				ClusterBounds.X * 0.04, ClusterBounds.X * 0.48, FMath::Abs(SignedHeight));
			const double Radius = RadiusFraction * LobeRadius;
			OutPosition = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
				SignedHeight * ClusterBounds.Z * 0.5) * 100;
			return true;
		}
	default:
		return false;
	}
}

double UStarClusterGenerator::GetSizeExtentFactor(const EStarClusterSize StarClusterSize)
{
	// Roughly constant star density (extent ~ count^1/3), softened so Tiny stays readable.
	switch (StarClusterSize)
	{
	case EStarClusterSize::Tiny: return 0.30;
	case EStarClusterSize::Small: return 0.40;
	case EStarClusterSize::Medium: return 0.55;
	case EStarClusterSize::Large: return 0.78;
	case EStarClusterSize::Colossal: return 1.26;
	case EStarClusterSize::Giant:
	default: return 1.0;
	}
}

int32 UStarClusterGenerator::GetPreviewFormationBudget(const EStarClusterSize StarClusterSize)
{
	// Each live system costs ~0.2 us per moving preview frame (continuous frame) plus its HISM instance.
	switch (StarClusterSize)
	{
	case EStarClusterSize::Tiny: return 500;
	case EStarClusterSize::Small: return 1500;
	case EStarClusterSize::Medium: return 3000;
	case EStarClusterSize::Large: return 6000;
	case EStarClusterSize::Giant: return 12000;
	case EStarClusterSize::Colossal: return 20000;
	default: return 1000;
	}
}

double UStarClusterGenerator::GetLogicalHalfExtent(const FVector& ClusterBounds, const EStarClusterType ClusterType)
{
	// Mirrors AAstroGenerator::GenerateStarCluster exactly (historic datasets must compose to the same scale).
	double HalfExtent = ClusterBounds.GetAbs().GetMax() * 50.0;
	if (ClusterType == EStarClusterType::GlobularCluster)
	{
		constexpr double MaximumGeneratedStarRadiusSolar = 1000.0;
		HalfExtent = FMath::Max(HalfExtent,
			(ClusterBounds.GetAbs().GetMax() * 0.5 + MaximumGeneratedStarRadiusSolar * 100.0) * 100.0);
	}
	return HalfExtent;
}

UStarClusterGenerator::UStarClusterGenerator()
{
	BP_StarClusterClass = AStarCluster::StaticClass();
}

FVector UStarClusterGenerator::CalculateStarPosition(int StarIndex, AStarCluster* StarCluster,
                                                     const TSharedPtr<FStarModel> StarModel)
{
	if (!StarCluster || !StarModel || StarCluster->StarAmount <= 0)
	{
		return FVector::ZeroVector;
	}
	FVector StarPosition;
	const double NormalizedIndex = StarCluster->StarAmount > 1
		? static_cast<double>(StarIndex) / static_cast<double>(StarCluster->StarAmount - 1) : 0.5;

	// ������ ������
	double StarSize = StarModel->Radius;

	switch (StarCluster->ClusterType)
	{
	case EStarClusterType::OpenCluster:
		{
			// ����������� ������������� � ������ ������� ������
			StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
			                                                  , -StarCluster->ClusterBounds.Y / 2 - StarSize,
			                                                  -StarCluster->ClusterBounds.Z / 2 - StarSize)
			                                          , FVector(StarCluster->ClusterBounds.X / 2 + StarSize,
			                                                    StarCluster->ClusterBounds.Y / 2 + StarSize
			                                                    , StarCluster->ClusterBounds.Z / 2 + StarSize)));
		}
		break;
	case EStarClusterType::GlobularCluster: /// TODO: ALL STAR MAIN SEQUENCE !
		{
			// ������ ����� ����� � ������, � ������ ������� ������
			double Radius = FMath::RandRange(static_cast<double>(StarSize), StarCluster->ClusterBounds.X / 2) + StarSize
				* 100; // �������� �� 100, ��� ��� StarSize � UE scale, � �� � �����������.
			double Angle = FMath::RandRange(0.0f, 2 * PI);
			double Z = FMath::RandRange(-StarCluster->ClusterBounds.Z / 2, StarCluster->ClusterBounds.Z / 2);
			StarPosition = FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
		}
		break;
	case EStarClusterType::Supercluster:
		{
			double SphereRadius = FMath::Max(StarCluster->ClusterBounds.X,
			                                 FMath::Max(StarCluster->ClusterBounds.Y,
			                                            StarCluster->ClusterBounds.Z)) / 2;
			FVector RandomPoint = FMath::VRand();
			double Weight = 1 - FMath::Pow(StarSize, 3); // ���������� �����
			double RandomScale = FMath::Pow(FMath::FRand(), 1.6)
				* FMath::Max(SphereRadius - StarSize, SphereRadius * 0.1);
			RandomPoint *= RandomScale;
			StarPosition = RandomPoint;

			if (StarSize > 10.0f)
			{
				StarPosition /= 2; // ���������� ������� ������ ����� � ������
			}

			//// ����� ����������� �������������, �� � ���������� �������� �������� � ������, � ������ ������� ������
			//StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
			//    , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
			//    , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
			//    , StarCluster->ClusterBounds.Z / 2 + StarSize)));
			if (StarSize > 10.0f)
			{
				StarPosition /= 2; // ���������� ������� ������ ����� � ������
			}
		}
		break;
	case EStarClusterType::Nebula:
		{
			//StarCluster->ClusterBounds = FVector(10, 10, 10);

			// ������������� ����� �� ������� � ������ ������� ������
			// Build the nebula from deterministic antipodal pairs. The former
			// StarIndex * StarSize radius made the formation depend on the HISM budget
			// and allowed giant stars to escape far beyond the authored cluster bounds.
			// Pairing every point with its inverse also keeps the visible formation
			// centred on the cluster origin for every seed.
			const int32 PairCount = StarCluster->StarAmount / 2;
			const bool bUnpairedCentre = (StarCluster->StarAmount % 2) != 0
				&& StarIndex == StarCluster->StarAmount - 1;
			if (bUnpairedCentre || PairCount <= 0)
			{
				StarPosition = FVector::ZeroVector;
				break;
			}

			const int32 PairIndex = StarIndex / 2;
			const bool bOppositePoint = (StarIndex & 1) != 0;
			const uint32 PairSeedHash = HashCombine(
				GetTypeHash(StarCluster->GenerationSeed), GetTypeHash(PairIndex));
			FRandomStream PairStream(FMath::Max(
				1, static_cast<int32>(PairSeedHash & 0x7fffffffu)));

			const double PairAlpha = (static_cast<double>(PairIndex) + 0.5)
				/ static_cast<double>(PairCount);
			const double MaxRadialExtent = FMath::Max(0.0,
				FMath::Min(FMath::Abs(StarCluster->ClusterBounds.X),
					FMath::Abs(StarCluster->ClusterBounds.Y)) * 0.5);
			const double Radius = MaxRadialExtent * FMath::Sqrt(PairAlpha)
				* PairStream.FRandRange(0.78f, 1.0f);
			const double BaseAngle = PairAlpha * UE_TWO_PI * 3.25
				+ PairStream.FRandRange(-0.16f, 0.16f);
			const double Angle = BaseAngle + (bOppositePoint ? PI : 0.0);
			const double HalfHeight = FMath::Abs(StarCluster->ClusterBounds.Z) * 0.5;
			const double VerticalEnvelope = FMath::Lerp(1.0, 0.18, PairAlpha);
			const double PairZ = PairStream.FRandRange(-HalfHeight, HalfHeight)
				* VerticalEnvelope;
			StarPosition = FVector(
				FMath::Cos(Angle) * Radius,
				FMath::Sin(Angle) * Radius,
				bOppositePoint ? -PairZ : PairZ);
		}
		break;
	case EStarClusterType::ElongatedStream:
		{
			const double X = FMath::Lerp(-StarCluster->ClusterBounds.X * 0.5,
				StarCluster->ClusterBounds.X * 0.5, NormalizedIndex);
			const double Taper = 0.2 + 0.8 * FMath::Sin(PI * NormalizedIndex);
			const double Y = FMath::RandRange(-StarCluster->ClusterBounds.Y * 0.5,
				StarCluster->ClusterBounds.Y * 0.5) * Taper
				+ FMath::Sin(NormalizedIndex * UE_TWO_PI * 2.0) * StarCluster->ClusterBounds.Y * 0.16;
			const double Z = FMath::RandRange(-StarCluster->ClusterBounds.Z * 0.5,
				StarCluster->ClusterBounds.Z * 0.5) * Taper;
			StarPosition = FVector(X, Y, Z);
		}
		break;
	case EStarClusterType::RingArc:
		{
			// Leave a readable gap in the ring so the preset is visually distinct
			// from both a globular cluster and the continuous nebula spiral. Symmetric
			// pairs share radial jitter and height, while the nominal arc bounds are
			// translated back to the cluster origin. Using opposite height signs split
			// the two middle neighbours by the full ribbon thickness and exposed a hard
			// technical seam. Pair-coherent height preserves the same volume and seed.
			const double ArcHalfAngle = PI * 0.82;
			const double Angle = FMath::Lerp(-ArcHalfAngle, ArcHalfAngle, NormalizedIndex);
			const double BaseRadius = FMath::Min(StarCluster->ClusterBounds.X,
				StarCluster->ClusterBounds.Y) * 0.38;
			const int32 MirroredIndex = StarCluster->StarAmount - 1 - StarIndex;
			const int32 PairIndex = FMath::Min(StarIndex, MirroredIndex);
			const uint32 PairSeedHash = HashCombine(
				GetTypeHash(StarCluster->GenerationSeed), GetTypeHash(PairIndex));
			FRandomStream PairStream(FMath::Max(
				1, static_cast<int32>(PairSeedHash & 0x7fffffffu)));
			// Keep the nominal X extrema fixed so the translated arc remains centred.
			const double JitterEnvelope = FMath::Square(FMath::Sin(UE_TWO_PI * NormalizedIndex));
			const double Radius = BaseRadius * (1.0
				+ PairStream.FRandRange(-0.14f, 0.14f) * JitterEnvelope);
			const double ArcBoundsCentreX = BaseRadius
				* (1.0 + FMath::Cos(ArcHalfAngle)) * 0.5;
			const double HalfHeight = FMath::Abs(StarCluster->ClusterBounds.Z) * 0.5;
			const double HeightMagnitude = PairStream.FRandRange(0.0f, HalfHeight);
			const double HeightSign = (PairSeedHash & 1u) != 0u ? 1.0 : -1.0;
			const double Height = HeightMagnitude * HeightSign;
			StarPosition = FVector(
				FMath::Cos(Angle) * Radius - ArcBoundsCentreX,
				FMath::Sin(Angle) * Radius,
				Height);
		}
		break;
	case EStarClusterType::Hourglass:
		{
			const double SignedHeight = FMath::RandRange(-1.0, 1.0);
			const double Z = SignedHeight * StarCluster->ClusterBounds.Z * 0.5;
			const double LobeRadius = FMath::Lerp(
				StarCluster->ClusterBounds.X * 0.04, StarCluster->ClusterBounds.X * 0.48,
				FMath::Abs(SignedHeight));
			const double Radius = FMath::Sqrt(FMath::FRand()) * LobeRadius;
			const double Angle = FMath::FRandRange(0.0, UE_TWO_PI);
			StarPosition = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z);
		}
		break;
	case EStarClusterType::YoungAssociation:
	case EStarClusterType::MovingGroup:
	case EStarClusterType::SuperStarCluster:
	case EStarClusterType::EmbeddedCluster:
	case EStarClusterType::DoubleCluster:
		// Rio 03.10: seeded formations, independent of the global RNG and of the render prefix.
		SampleSeededFormation(StarCluster->ClusterType, StarCluster->GenerationSeed, StarIndex,
			StarCluster->ClusterBounds, StarModel->Mass, StarPosition);
		break;
	case EStarClusterType::Unknown:
	default:
		StarPosition = FMath::RandPointInBox(FBox(
			-StarCluster->ClusterBounds * 0.5, StarCluster->ClusterBounds * 0.5));
		break;
	}
	return StarPosition * 100;
}

TMap<EStarClusterSize, TPair<int, int>> StarClusterSizes =
{
	{EStarClusterSize::Tiny, TPair<int, int>(100, 500)},
	{EStarClusterSize::Small, TPair<int, int>(500, 1500)},
	{EStarClusterSize::Medium, TPair<int, int>(1500, 5000)},
	{EStarClusterSize::Large, TPair<int, int>(5000, 25000)},
	{EStarClusterSize::Giant, TPair<int, int>(25000, 50000)},
	{EStarClusterSize::Unknown, TPair<int, int>(0, 0)},
	// Rio 03.10: above Giant. Every system is a sealed record in the save (~2.2 KB each today).
	{EStarClusterSize::Colossal, TPair<int, int>(50000, 100000)},
};

int UStarClusterGenerator::GetStarsAmountByRange(EStarClusterSize StarClusterSize)
{
	const TPair<int, int>* Range = StarClusterSizes.Find(StarClusterSize);
	if (Range == nullptr)
	{
		return 0;
	}
	return FMath::RandRange(Range->Key, Range->Value);
}

double UStarClusterGenerator::GetStarClusterDensityByRange()
{
	return FMath::RandRange(0.1f, 1.0f); /// TODO: RANGE
}

FVector UStarClusterGenerator::GetStarClusterBoundsByRange(EStarClusterType ClusterType)
{
	switch (ClusterType)
	{
	case EStarClusterType::OpenCluster: return FVector(100000.0, 100000.0, 100000.0);
	case EStarClusterType::GlobularCluster: return FVector(80000.0, 80000.0, 80000.0);
	case EStarClusterType::Supercluster: return FVector(250000.0, 250000.0, 180000.0);
	case EStarClusterType::Nebula: return FVector(160000.0, 160000.0, 25000.0);
	case EStarClusterType::ElongatedStream: return FVector(240000.0, 32000.0, 22000.0);
	case EStarClusterType::RingArc: return FVector(160000.0, 160000.0, 18000.0);
	case EStarClusterType::Hourglass: return FVector(100000.0, 100000.0, 200000.0);
	case EStarClusterType::YoungAssociation: return FVector(220000.0, 160000.0, 90000.0);
	case EStarClusterType::MovingGroup: return FVector(260000.0, 110000.0, 80000.0);
	case EStarClusterType::SuperStarCluster: return FVector(60000.0, 60000.0, 60000.0);
	case EStarClusterType::EmbeddedCluster: return FVector(120000.0, 120000.0, 90000.0);
	case EStarClusterType::DoubleCluster: return FVector(200000.0, 100000.0, 80000.0);
	case EStarClusterType::Unknown:
	default: return FVector(100000.0, 100000.0, 100000.0);
	}
}

EStarClusterType UStarClusterGenerator::GetRandomClusterType()
{
	static constexpr EStarClusterType Types[] = {
		EStarClusterType::OpenCluster,
		EStarClusterType::GlobularCluster,
		EStarClusterType::Supercluster,
		EStarClusterType::Nebula,
		EStarClusterType::ElongatedStream,
		EStarClusterType::RingArc,
		EStarClusterType::Hourglass
	};
	return Types[FMath::RandRange(0, UE_ARRAY_COUNT(Types) - 1)];
}

TUniquePtr<FStarClusterModel> UStarClusterGenerator::GenerateRandomStarClusterModelByParams(
	TUniquePtr<FStarClusterModel> StarClusterModel)
{
	//TUniquePtr<FStarClusterModel> StarClusterModel = MakeUnique<FStarClusterModel>();;

	return StarClusterModel;
}

/*TUniquePtr<FStarClusterModel>*/
void UStarClusterGenerator::GetRandomStarClusterModel(TSharedPtr<FStarClusterModel> StarClusterModel)
{
	//TUniquePtr<FStarClusterModel> StarClusterModel = MakeUnique<FStarClusterModel>();

	StarClusterModel->StarClusterSize = static_cast<EStarClusterSize>(FMath::RandRange(
		0, static_cast<int>(EStarClusterSize::Giant)));
	StarClusterModel->StarClusterType = GetRandomClusterType();
	StarClusterModel->StarClusterPopulation = static_cast<EStarClusterPopulation>(FMath::RandRange(
		0, static_cast<int>(EStarClusterPopulation::Protostars)));
	StarClusterModel->StarClusterComposition = static_cast<EStarClusterComposition>(FMath::RandRange(
		0, static_cast<int>(EStarClusterComposition::MostlyRed)));

	//return StarClusterModel;
}
