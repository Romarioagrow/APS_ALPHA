#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "CustomNoise.h"
#include "ProceduralMeshComponent.h"

// Closed geometry only: no material selection, component ownership or streaming.
namespace APSClosedGlobeMesh
{
    constexpr int32 MaximumFaceResolution = 128;

    // Capture on the game thread from one fully applied native profile. Positions
    // are planet-local centimetres, centred at zero; seeded noise is copied per
    // worker. The caller owns the world transform and material frame separately.
    struct FSamplingFrame
    {
        FAPSResolvedPlanetSurfaceProfile Profile;
        CustomNoise SeededNoise;
        double Radius = 0.0;
        double OceanHeight = 0.0;
        double NoiseScale = 0.0;
        double NoiseIntensity = 0.0;
        double PresentationScale = 1.0;
        bool bCoastalReliefCandidate = false;
        // Enable only for a configured native UnifiedLava surface. Changes
        // displacement only, never normalized height/climate/material channels.
        bool bApplyNativeLavaEnvelope = false;
    };

    enum class ENormalPolicy : uint8
    {
        // Area-weighted normals of these physical triangles, welded at cube seams.
        // Not a claim of identical normals on a differently tessellated native LOD.
        PhysicalTriangles,
        // Preserve the existing menu relief cap, exaggeration and radial blend.
        PreviewReliefBlend
    };

    struct FBuildOptions
    {
        int32 FaceResolution = 48;
        bool bBuildOcean = false;
        bool bWaterDepth = false;
        bool bRefineCoast = false;
        // Native terrain alpha is Hole (0 on intact ground), not liquid coverage.
        // False preserves the existing closed-menu terrain payload.
        bool bNativeTerrainHoleAlpha = false;
        double NormalReliefExaggeration = 1.0;
        ENormalPolicy NormalPolicy = ENormalPolicy::PhysicalTriangles;
    };

    struct FMeshData
    {
        TArray<FVector> TerrainVertices;
        TArray<FVector> OceanVertices;
        TArray<int32> Indices;
        TArray<int32> OceanIndices;
        TArray<FVector> Normals;
        TArray<FVector> OceanNormals;
        TArray<FVector2D> OceanDepthUV1;
        TArray<FVector2D> UV0;
        TArray<FLinearColor> VertexColors;
        // Same RGB as terrain, but always WaterMask alpha for the closed ocean.
        TArray<FLinearColor> OceanVertexColors;
        TArray<FProcMeshTangent> Tangents;
    };

    // Synchronous bounded CPU build (including joined ParallelFor workers).
    // Supply an empty output; discard it on false. No UObject is sampled.
    APS_ALPHA_API bool BuildClosedCubeSphere(const FSamplingFrame& Frame,
        const FBuildOptions& Options, FMeshData& OutData);
}
