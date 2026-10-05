#pragma once

#include "APS_ALPHA/Generation/APSCanonicalFilteredHeight.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

// Transient transport only. No runtime owner, material assignment, save, flush
// or readiness claim. The caller must retain the returned UObject immediately
// and wait for its render-resource fence before making the chart available.
namespace APSCanonicalCoverageTexture
{
    inline UTexture2D* CreateTransient(const APSCanonicalFilteredHeight::FData& Data, FString& Error)
    {
        Error.Reset();
        const auto& M=Data.Metadata; const auto& L=M.Layout;
        const bool Dense=M.HeightFilter==APSCanonicalFilteredHeight::EHeightFilter::SharedLattice;
        const bool FilterValid=Dense
            ? (M.Oversample==2 || M.Oversample==4) && M.QuadratureOrder==0
            : M.HeightFilter==APSCanonicalFilteredHeight::EHeightFilter::Quadrature
                && M.Oversample==0 && (M.QuadratureOrder==3 || M.QuadratureOrder==5);
        if (!IsInGameThread() || !APSCanonicalCoverageLayout::ValidLayout(L)
            || M.Level<0 || M.Level>=L.Count || M.InputSignature==0 || Data.Mips.IsEmpty()
            || !FilterValid
            || M.SigmaTexels!=APSCanonicalFilteredHeight::SigmaInLocalTexels
            || M.WidthCm!=L.Levels[M.Level].WidthCm
            || M.CenterTexelSpacingCm!=L.Levels[M.Level].CenterTexelSpacingCm
            || !FMath::IsFinite(M.NoiseScale) || M.NoiseScale<1
            || !FMath::IsFinite(M.NoiseIntensity) || M.NoiseIntensity<0
            || !FMath::IsFinite(M.EnvelopeSeaHeightCm)
            || !FMath::IsFinite(M.MinimumLocalSpacingCm) || M.MinimumLocalSpacingCm<=0
            || !FMath::IsFinite(M.MaximumLocalSpacingCm) || M.MaximumLocalSpacingCm<M.MinimumLocalSpacingCm
            || M.MaximumLocalSpacingCm>M.CenterTexelSpacingCm*(1.0+1.e-8)
            || !FMath::IsFinite(M.MinimumFilteredHeightCm) || L.RadiusCm+M.MinimumFilteredHeightCm<=0
            || !FMath::IsFinite(M.MaximumFilteredHeightCm) || M.MaximumFilteredHeightCm<M.MinimumFilteredHeightCm
            || !FMath::IsFinite(M.MaximumSlope) || M.MaximumSlope<0
            || !FMath::IsFinite(M.PreparationWallSeconds) || M.PreparationWallSeconds<0
            || M.bProvenBandLimited)
        { Error=TEXT("Coverage texture needs GT and validated filtered-height residual metadata"); return nullptr; }
        const int64 FinePitch=int64(L.Resolution+7)*M.Oversample+1;
        const int64 Calls=Dense ? FinePitch*FinePitch
            : int64(L.Resolution+2)*(L.Resolution+2)*M.QuadratureOrder*M.QuadratureOrder;
        if (M.MaximumHeightSamples!=Calls || M.ActualHeightSamples!=Calls)
        { Error=TEXT("Coverage texture height-call provenance changed"); return nullptr; }

        int32 Size=L.Resolution; int64 Bytes=0;
        for (const auto& Mip:Data.Mips)
        {
            if (Size<1 || Mip.Size!=Size || Mip.Pixels.Num()!=Size*Size)
            { Error=TEXT("Coverage texture mip dimensions/count changed"); return nullptr; }
            for (const FFloat16Color& Pixel:Mip.Pixels)
            {
                const FLinearColor P=Pixel.GetFloats();
                if (!FMath::IsFinite(P.R) || !FMath::IsFinite(P.G) || !FMath::IsFinite(P.B)
                    || FMath::Abs(P.R)>65000 || FMath::Abs(P.G)>65000 || FMath::Abs(P.B)>65000 || P.A!=1.0f)
                { Error=TEXT("Coverage texture has invalid signed slope residual; RGB is not a unit normal"); return nullptr; }
            }
            Bytes+=int64(Mip.Pixels.Num())*sizeof(FFloat16Color); Size/=2;
        }
        if (Size!=0 || Bytes!=Data.ByteCount)
        { Error=TEXT("Coverage texture mip chain/byte count changed"); return nullptr; }
        static_assert(sizeof(FFloat16Color)==8,"PF_FloatRGBA requires four packed half floats");

        // UE5.4 CreateTransient without InImageData allocates platform mip0 but
        // does not enqueue UpdateResource: extending the mip array here is safe.
        UTexture2D* Texture=UTexture2D::CreateTransient(L.Resolution,L.Resolution,PF_FloatRGBA);
        if (!Texture) { Error=TEXT("Coverage Texture2D allocation failed"); return nullptr; }
        Texture->SRGB=false; Texture->NeverStream=true; Texture->Filter=TF_Trilinear;
        Texture->AddressX=TA_Clamp; Texture->AddressY=TA_Clamp;
        Texture->CompressionSettings=TC_HDR; Texture->LODBias=0;
#if WITH_EDITORONLY_DATA
        Texture->MipGenSettings=TMGS_LeaveExistingMips;
#endif
        FTexturePlatformData* Platform=Texture->GetPlatformData();
        if (!Platform || Platform->Mips.Num()!=1 || Platform->SizeX!=L.Resolution
            || Platform->SizeY!=L.Resolution || Platform->PixelFormat!=PF_FloatRGBA)
        { Error=TEXT("Coverage transient platform-data contract changed"); return nullptr; }
        for (int32 I=0; I<Data.Mips.Num(); ++I)
        {
            const auto& Source=Data.Mips[I];
            if (I) Platform->Mips.Add(new FTexture2DMipMap(Source.Size,Source.Size,1));
            FTexture2DMipMap& Mip=Platform->Mips[I];
            const int64 SizeBytes=int64(Source.Pixels.Num())*sizeof(FFloat16Color);
            Mip.BulkData.Lock(LOCK_READ_WRITE);
            void* Destination=Mip.BulkData.Realloc(SizeBytes);
            FMemory::Memcpy(Destination,Source.Pixels.GetData(),SizeBytes);
            Mip.BulkData.Unlock();
        }
        Texture->UpdateResource();
        return Texture; // Enqueued upload, NOT render readiness or visual proof.
    }
}
