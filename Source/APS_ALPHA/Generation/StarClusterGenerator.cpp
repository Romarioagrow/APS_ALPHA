#include "StarClusterGenerator.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"

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
			// pairs share radial jitter and opposite height, while the nominal arc
			// bounds are translated back to the cluster origin. This preserves the arc
			// silhouette without making its camera focus drift toward the filled side.
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
			const double Height = StarIndex == MirroredIndex ? 0.0
				: (StarIndex < MirroredIndex ? -HeightMagnitude : HeightMagnitude);
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
