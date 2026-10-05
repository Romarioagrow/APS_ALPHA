#pragma once

#include "APSClosedGlobeMesh.h"
#include "Templates/Function.h"

// CPU-only foundation. No production caller, texture, material or mesh mutation.
// A bounded field represents a macro band, not proof of adequate orbital detail.
namespace APSPlanetReliefField
{
    constexpr int32 MinimumResolution = 16;
    constexpr int32 MaximumResolution = 256;

    struct FBuildOptions
    {
        int32 FaceResolution = 128;
        // Required physical bandwidth; zero deliberately does NOT choose one.
        double DerivativeHalfStepCm = 0.0;
        // Nonzero identity captured by the owner with this immutable snapshot.
        uint32 InputSignature = 0;
    };

    struct FMetadata
    {
        int32 FaceResolution = 0;
        uint32 InputSignature = 0;
        EPlanetType PlanetType = EPlanetType::Unknown;
        int32 TerrainSeed = 0, BiomeSeed = 0;
        double RadiusCm = 0.0, NoiseScale = 0.0, NoiseIntensity = 0.0;
        double PresentationScale = 0.0, OceanHeightCm = 0.0, EnvelopeSeaHeightCm = 0.0;
        double DerivativeHalfStepCm = 0.0, DerivativeHalfAngleRadians = 0.0;
        // Cube-centre upper bounds; exact adjacent-node arcs vary over each face.
        double MaximumTexelArcRadians = 0.0, MaximumTexelArcCm = 0.0;
        int32 UniqueDirections = 0;
        int64 MaximumHeightSamples = 0;
        bool bCoastalReliefCandidate = false, bNativeLavaEnvelope = false;
    };

    struct FFieldData
    {
        FMetadata Metadata;
        // Face order/orientation matches APSClosedGlobeMesh. Each face stores
        // (N+1)^2 nodes, row-major Y then X, including bit-identical shared edges.
        TArray<FVector3f> PlanetNormals;
    };

    // Canonical integer cube addresses weld all duplicate faces before sampling.
    APS_ALPHA_API bool DirectionAt(int32 Face, int32 X, int32 Y, int32 Resolution,
        FIntVector& Address, FVector3d& Direction);

    // Six height-only samples on three projected planet axes avoid arbitrary
    // tangent-basis switches. Callback receives unit planet-local directions.
    // This finite stencil is not a sharp-crease or band-limiting guarantee.
    APS_ALPHA_API bool EvaluateNormal(const FVector3d& Direction, double RadiusCm,
        double HalfStepCm, TFunctionRef<bool(const FVector3d&, double&)> Height,
        FVector3f& OutNormal);

    // Caller supplies an empty output. Any failure leaves it untouched. The
    // signature is provenance, not an inferred hash of CustomNoise internals.
    // Joined bounded CPU workers access immutable values and task-local noise.
    APS_ALPHA_API bool Build(const APSClosedGlobeMesh::FSamplingFrame& Frame,
        const FBuildOptions& Options, FFieldData& Out, FString& Error);
}
