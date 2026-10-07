#pragma once

#include "CoreMinimal.h"
#include "Math/Float16Color.h"
#include "APS_ALPHA/Generation/APSPlanetReliefField.h"

class UTextureCube;

// Unconnected transport foundation. No material/mesh/asset mutation or cache owner.
namespace APSPlanetReliefTexture
{
    struct FMipData
    {
        int32 Size = 0;
        // UE/DX face-major +X,-X,+Y,-Y,+Z,-Z; each face row-major Y then X.
        // Signed planet-local unit normal RGB, A=1; NOT [0,1] normal encoding.
        TArray<FFloat16Color> Pixels;
    };

    struct FUploadData
    {
        APSPlanetReliefField::FMetadata Metadata;
        TArray<FMipData> Mips;
        int64 ByteCount = 0;
    };

    // UE5.4 SkyLightMipTreeCommon.ush:GetTextureCubeVector, at (X+.5,Y+.5).
    // Exposed for orientation contracts; accepts mip sizes 1..256.
    APS_ALPHA_API bool CubeTexelDirection(int32 Face, int32 X, int32 Y, int32 Size,
        FVector3d& OutDirection);

    // CPU-only, worker-safe, no UObject access. Explicitly requires power-of-two
    // N in16..256. Remaps the field's (N+1)^2 shared-edge nodes to N^2 texel centres.
    // Mips average planet-space vectors with solid-angle weights, not encoded
    // colour. Unnormalised sums survive downsampling; packing normalises once.
    // An empty output is required and is unchanged on failure. Resolution bounds
    // and valid data do NOT prove enough bandwidth for a particular planet/view.
    APS_ALPHA_API bool Prepare(const APSPlanetReliefField::FFieldData& Field,
        FUploadData& Out, FString& Error);

    // Game-thread-only upload of Prepare's immutable result. RF_Transient,
    // PF_FloatRGBA, SRGB=false, NeverStream=true. Enqueues resource initialisation;
    // returning a UObject does NOT imply its render resource is ready.
    // Caller must immediately retain a UPROPERTY/TStrongObjectPtr or MID texture
    // parameter reference, and keep availability=0 until its readiness contract.
    // No AddToRoot, package save, material binding, flush or synchronous compile.
    APS_ALPHA_API UTextureCube* CreateTransient(const FUploadData& Data, FString& Error);
}
