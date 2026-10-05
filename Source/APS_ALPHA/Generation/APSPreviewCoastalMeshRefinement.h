#pragma once

#include "APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Core/Planetary/APSCoastalMeshRefinement.h"
#include "Async/ParallelFor.h"

// Experimental closed-preview topology only. Sample the existing physical
// profile at new directions; never interpolate a new height or alter a seed.
namespace APSPreviewCoastalMeshRefinement
{
    constexpr int32 MaximumExtraVertices = 32768;
    constexpr int32 MaximumPasses = 2;

    struct FStats
    {
        int32 Passes = 0;
        int32 AddedVertices = 0;
        bool bBudgetLimited = false;
    };

    inline bool IsFiniteSample(const FNoiseData& Sample)
    {
        return FMath::IsFinite(Sample.Height) && FMath::IsFinite(Sample.HeightNormalize)
            && FMath::IsFinite(Sample.Temperature) && FMath::IsFinite(Sample.Humidity)
            && FMath::IsFinite(Sample.WaterMask);
    }

    inline bool Crosses(const double A, const double B, const double C)
    {
        return FMath::Min3(A, B, C) < 0.0 && FMath::Max3(A, B, C) >= 0.0;
    }

    inline double MaskDistance(const FNoiseData& Sample)
    {
        // Exact sign of the shared Water/Ammonia presentation clip. The
        // material applies smoothstep(.08,.72,alpha), then clips at .3333.
        const double X = FMath::Clamp((double(Sample.WaterMask) - 0.08) / 0.64, 0.0, 1.0);
        return X * X * (3.0 - 2.0 * X) - double(0.3333f);
    }

    inline bool Refine(TArray<FVector>& Directions, TArray<FNoiseData>& Samples,
        TArray<int32>& Indices, const FAPSResolvedPlanetSurfaceProfile& Profile,
        const CustomNoise& SeededNoise, const double Radius, const double OceanHeight,
        const double NoiseScale, const double NoiseIntensity, const bool bCoastalRelief,
        FStats& Stats)
    {
        using namespace APSCoastalMeshRefinement;
        Stats = {};
        if (Directions.Num() != Samples.Num() || Indices.Num() % 3 != 0)
            return false;

        // Match opposite copies of cube-face borders without merging UV vertices.
        // This quantization is much finer than even the twice-refined base mesh.
        constexpr double WeldQuantization = 1048576.0;
        TMap<FIntVector, FIndex> WeldLookup;
        std::vector<FIndex> WeldIds;
        WeldIds.reserve(Directions.Num());
        for (const FVector& Direction : Directions)
        {
            const FIntVector Key(FMath::RoundToInt(Direction.X * WeldQuantization),
                FMath::RoundToInt(Direction.Y * WeldQuantization),
                FMath::RoundToInt(Direction.Z * WeldQuantization));
            const FIndex NextId = static_cast<FIndex>(WeldLookup.Num());
            WeldIds.push_back(WeldLookup.FindOrAdd(Key, NextId));
        }

        std::vector<FIndex> Topology;
        Topology.reserve(Indices.Num());
        for (const int32 Index : Indices)
        {
            if (!Samples.IsValidIndex(Index)) return false;
            Topology.push_back(static_cast<FIndex>(Index));
        }
        for (int32 Pass = 0; Pass < MaximumPasses; ++Pass)
        {
            std::vector<uint8_t> CoastTriangles(Topology.size() / 3, 0);
            for (size_t Triangle = 0; Triangle < CoastTriangles.size(); ++Triangle)
            {
                const FNoiseData& A = Samples[Topology[Triangle * 3]];
                const FNoiseData& B = Samples[Topology[Triangle * 3 + 1]];
                const FNoiseData& C = Samples[Topology[Triangle * 3 + 2]];
                CoastTriangles[Triangle] = Crosses(A.Height - OceanHeight,
                    B.Height - OceanHeight, C.Height - OceanHeight)
                    || Crosses(MaskDistance(A), MaskDistance(B), MaskDistance(C));
            }
            FPlan Plan = PlanPass(Topology, WeldIds, CoastTriangles,
                static_cast<size_t>(MaximumExtraVertices - Stats.AddedVertices));
            if (Plan.Status == EStatus::NoOp) break;
            if (Plan.Status == EStatus::BudgetExceeded)
            {
                Stats.bBudgetLimited = true;
                break; // Whole pass refused: no partially refined boundary.
            }
            if (Plan.Status != EStatus::Applied || Plan.Indices.size() > MAX_int32)
                return false;

            const int32 Added = static_cast<int32>(Plan.Midpoints.size());
            TArray<FVector> NewDirections;
            NewDirections.Reserve(Added);
            for (const FMidpoint& Midpoint : Plan.Midpoints)
            {
                const FVector Direction = (Directions[Midpoint.A] + Directions[Midpoint.B]).GetSafeNormal();
                if (Direction.ContainsNaN() || Direction.IsNearlyZero()) return false;
                NewDirections.Add(Direction);
            }
            TArray<FNoiseData> NewSamples;
            NewSamples.SetNumUninitialized(Added);
            // Bounded worker count, each owns seeded noise; no UObject access.
            constexpr int32 Workers = 6;
            TArray<uint8> Valid;
            Valid.Init(1, Workers);
            ParallelFor(Workers, [&](const int32 Worker)
            {
                CustomNoise LocalNoise = SeededNoise;
                for (int32 Index = Worker; Index < Added; Index += Workers)
                {
                    const FVector& Direction = NewDirections[Index];
                    DVector NoisePosition;
                    const FNoiseData Sample = UAPSWorldScapePlanetNoise::SampleResolvedProfile(
                        Profile, LocalNoise, DVector(Direction * Radius), DVector(0.0, 0.0, 0.0),
                        NoiseScale, NoiseIntensity, Radius, Direction.Z, NoisePosition, bCoastalRelief);
                    NewSamples[Index] = Sample;
                    if (!IsFiniteSample(Sample)) Valid[Worker] = 0;
                }
            });
            if (Valid.Contains(0)) return false;
            // Commit only after all samples and the complete conforming plan pass.
            Directions.Append(NewDirections);
            Samples.Append(NewSamples);
            Topology = MoveTemp(Plan.Indices);
            WeldIds = MoveTemp(Plan.WeldIds);
            Stats.AddedVertices += Added;
            ++Stats.Passes;
        }
        if (Stats.Passes > 0)
        {
            Indices.Reset(static_cast<int32>(Topology.size()));
            for (const FIndex Index : Topology) Indices.Add(static_cast<int32>(Index));
        }
        return true;
    }
}
