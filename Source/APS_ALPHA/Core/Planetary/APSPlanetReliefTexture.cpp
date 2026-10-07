#include "APSPlanetReliefTexture.h"

#include "Engine/TextureCube.h"
#include "TextureResource.h"

namespace APSPlanetReliefTexture
{
    namespace Private
    {
        bool ResolutionValid(int32 N)
        {
            return N >= APSPlanetReliefField::MinimumResolution
                && N <= APSPlanetReliefField::MaximumResolution && FMath::IsPowerOfTwo(N);
        }

        bool MetadataValid(const APSPlanetReliefField::FMetadata& M)
        {
            return ResolutionValid(M.FaceResolution) && M.InputSignature != 0
                && FMath::IsFinite(M.RadiusCm) && M.RadiusCm > 0.0
                && M.PresentationScale == 1.0 && FMath::IsFinite(M.DerivativeHalfStepCm)
                && M.DerivativeHalfStepCm > 0.0
                && M.DerivativeHalfStepCm <= M.RadiusCm / M.FaceResolution
                && FMath::IsFinite(M.MaximumTexelArcCm) && M.MaximumTexelArcCm > 0.0;
        }

        // Compare the field's DirectionAt addresses with UE5.4
        // Engine/Shaders/Private/RayTracing/SkyLightMipTreeCommon.ush:9:
        // DX vectors are (1,-v,-u),(-1,-v,u),(u,1,v),(u,-1,-v),
        // (u,-v,1),(-u,-v,-1). Source cells below are therefore rotated/flipped,
        // NOT a memcpy of the field's identically named faces.
        void SourceCell(int32 Face, int32 X, int32 Y, int32 N, int32& SX, int32& SY)
        {
            switch (Face)
            {
            case 0: SX = N - 1 - Y; SY = N - 1 - X; break;
            case 1: SX = Y; SY = X; break;
            case 2: SX = N - 1 - X; SY = Y; break;
            default: SX = X; SY = N - 1 - Y; break; // -Y,+Z,-Z
            }
        }

        double AreaElement(double U, double V)
        { return FMath::Atan2(U * V, FMath::Sqrt(U * U + V * V + 1.0)); }

        double SolidAngle(int32 X, int32 Y, int32 N)
        {
            // Same four-corner integral as SkyLightMipTreeCommon.ush:
            // TexelCoordSolidAngle. Actual cube solid angle, not uniform UV area.
            const double U0 = 2.0 * X / N - 1.0, U1 = 2.0 * (X + 1) / N - 1.0;
            const double V0 = 2.0 * Y / N - 1.0, V1 = 2.0 * (Y + 1) / N - 1.0;
            return AreaElement(U1, V1) - AreaElement(U0, V1)
                - AreaElement(U1, V0) + AreaElement(U0, V0);
        }
    }

    bool CubeTexelDirection(int32 Face, int32 X, int32 Y, int32 Size, FVector3d& OutDirection)
    {
        if (Face < 0 || Face >= 6 || Size < 1 || Size > APSPlanetReliefField::MaximumResolution
            || X < 0 || X >= Size || Y < 0 || Y >= Size) return false;
        const double U = 2.0 * (X + .5) / Size - 1.0, V = 2.0 * (Y + .5) / Size - 1.0;
        const FVector3d Directions[] = {{1.0, -V, -U}, {-1.0, -V, U}, {U, 1.0, V},
            {U, -1.0, -V}, {U, -V, 1.0}, {-U, -V, -1.0}};
        OutDirection = Directions[Face].GetSafeNormal();
        return true;
    }

    bool Prepare(const APSPlanetReliefField::FFieldData& Field, FUploadData& Out, FString& Error)
    {
        Error.Reset();
        if (!Out.Mips.IsEmpty() || Out.ByteCount != 0 || Out.Metadata.FaceResolution != 0
            || Out.Metadata.InputSignature != 0)
        { Error = TEXT("Relief upload output must be empty"); return false; }
        const int32 N = Field.Metadata.FaceResolution;
        if (!Private::MetadataValid(Field.Metadata)
            || Field.PlanetNormals.Num() != 6 * (N + 1) * (N + 1))
        { Error = TEXT("Relief upload needs valid full-scale metadata and a power-of-two16..256 shared-edge field"); return false; }

        // Reject nonfinite/nonunit payloads and drift in the promised welded edges
        // before producing any output. Only boundary nodes need the weld map.
        TMap<FIntVector, FVector3f> Edges;
        for (int32 Face = 0; Face < 6; ++Face)
            for (int32 Y = 0; Y <= N; ++Y)
                for (int32 X = 0; X <= N; ++X)
                {
                    const FVector3f V = Field.PlanetNormals[Face * (N + 1) * (N + 1) + Y * (N + 1) + X];
                    if (V.ContainsNaN() || !FMath::IsNearlyEqual(V.SizeSquared(), 1.0f, .002f))
                    { Error = TEXT("Relief field contains a nonfinite/nonunit normal"); return false; }
                    if (X != 0 && X != N && Y != 0 && Y != N) continue;
                    FIntVector Key; FVector3d Direction;
                    if (!APSPlanetReliefField::DirectionAt(Face, X, Y, N, Key, Direction))
                    { Error = TEXT("Relief source address is invalid"); return false; }
                    if (const FVector3f* Previous = Edges.Find(Key))
                    {
                        if (Previous->X != V.X || Previous->Y != V.Y || Previous->Z != V.Z)
                        { Error = TEXT("Relief source shared-edge normals differ"); return false; }
                    }
                    else Edges.Add(Key, V);
                }

        TArray<FVector3d> VectorSums;
        VectorSums.SetNumUninitialized(6 * N * N);
        for (int32 Face = 0; Face < 6; ++Face)
            for (int32 Y = 0; Y < N; ++Y)
                for (int32 X = 0; X < N; ++X)
                {
                    int32 SX, SY; Private::SourceCell(Face, X, Y, N, SX, SY);
                    const int32 A = Face * (N + 1) * (N + 1) + SY * (N + 1) + SX;
                    FVector3d V = (FVector3d(Field.PlanetNormals[A]) + FVector3d(Field.PlanetNormals[A + 1])
                        + FVector3d(Field.PlanetNormals[A + N + 1]) + FVector3d(Field.PlanetNormals[A + N + 2])) * .25;
                    const double Length = V.Size();
                    if (!FMath::IsFinite(Length) || Length < 1.e-12)
                    { Error = TEXT("Relief texel-centre interpolation is degenerate"); return false; }
                    V /= Length;
                    const int32 I = Face * N * N + Y * N + X;
                    const double Weight = Private::SolidAngle(X, Y, N);
                    if (!FMath::IsFinite(Weight) || Weight <= 0.0)
                    { Error = TEXT("Relief texel solid angle is invalid"); return false; }
                    VectorSums[I] = V * Weight;
                }

        FUploadData Result; Result.Metadata = Field.Metadata;
        for (int32 Size = N;; Size /= 2)
        {
            FMipData Mip; Mip.Size = Size; Mip.Pixels.Reserve(6 * Size * Size);
            for (int32 Face = 0; Face < 6; ++Face)
                for (int32 Y = 0; Y < Size; ++Y)
                    for (int32 X = 0; X < Size; ++X)
                    {
                        const int32 I = Face * Size * Size + Y * Size + X;
                        const double Length = VectorSums[I].Size();
                        if (!FMath::IsFinite(Length) || Length < 1.e-20)
                        { Error = TEXT("Relief mip vector mean is degenerate"); return false; }
                        const FVector3d Normal = VectorSums[I] / Length;
                        FVector3d Direction; CubeTexelDirection(Face, X, Y, Size, Direction);
                        if (Normal.ContainsNaN() || FVector3d::DotProduct(Normal, Direction) <= 0.0)
                        { Error = TEXT("Relief mip normal is not outward"); return false; }
                        Mip.Pixels.Emplace(FLinearColor(float(Normal.X), float(Normal.Y), float(Normal.Z), 1.0f));
                    }
            Result.ByteCount += int64(Mip.Pixels.Num()) * sizeof(FFloat16Color);
            Result.Mips.Add(MoveTemp(Mip));
            if (Size == 1) break;
            const int32 Next = Size / 2;
            TArray<FVector3d> NextSums;
            NextSums.SetNumZeroed(6 * Next * Next);
            for (int32 Face = 0; Face < 6; ++Face)
                for (int32 Y = 0; Y < Next; ++Y)
                    for (int32 X = 0; X < Next; ++X)
                    {
                        const int32 To = Face * Next * Next + Y * Next + X;
                        for (int32 DY = 0; DY < 2; ++DY)
                            for (int32 DX = 0; DX < 2; ++DX)
                            {
                                const int32 From = Face * Size * Size + (Y * 2 + DY) * Size + X * 2 + DX;
                                NextSums[To] += VectorSums[From];
                            }
                    }
            VectorSums = MoveTemp(NextSums);
        }
        Out = MoveTemp(Result);
        return true;
    }

    UTextureCube* CreateTransient(const FUploadData& Data, FString& Error)
    {
        Error.Reset();
        if (!IsInGameThread())
        { Error = TEXT("Relief texture upload requires the game thread"); return nullptr; }
        if (!Private::MetadataValid(Data.Metadata) || Data.Mips.IsEmpty())
        { Error = TEXT("Relief texture upload has no validated CPU mip chain"); return nullptr; }
        int32 ExpectedSize = Data.Metadata.FaceResolution;
        int64 Bytes = 0;
        for (int32 I = 0; I < Data.Mips.Num(); ++I)
        {
            const FMipData& Mip = Data.Mips[I];
            if (ExpectedSize < 1 || Mip.Size != ExpectedSize || Mip.Pixels.Num() != 6 * ExpectedSize * ExpectedSize)
            { Error = TEXT("Relief texture mip dimensions/count changed"); return nullptr; }
            Bytes += int64(Mip.Pixels.Num()) * sizeof(FFloat16Color);
            ExpectedSize /= 2;
        }
        if (ExpectedSize != 0 || Bytes != Data.ByteCount)
        { Error = TEXT("Relief texture mip chain is incomplete or byte count changed"); return nullptr; }
        static_assert(sizeof(FFloat16Color) == 8, "PF_FloatRGBA upload requires four packed half floats");
        UTextureCube* Texture = UTextureCube::CreateTransient(Data.Metadata.FaceResolution,
            Data.Metadata.FaceResolution, PF_FloatRGBA);
        if (!Texture) { Error = TEXT("Transient relief cubemap allocation failed"); return nullptr; }
        Texture->SRGB = false;
        Texture->NeverStream = true;
        Texture->Filter = TF_Trilinear;
        Texture->CompressionSettings = TC_HDR;
        Texture->LODBias = 0;
#if WITH_EDITORONLY_DATA
        Texture->MipGenSettings = TMGS_LeaveExistingMips;
#endif
        FTexturePlatformData* Platform = Texture->GetPlatformData();
        for (int32 I = 0; I < Data.Mips.Num(); ++I)
        {
            const FMipData& Source = Data.Mips[I];
            if (I != 0) Platform->Mips.Add(new FTexture2DMipMap(Source.Size, Source.Size, 1));
            FTexture2DMipMap& Mip = Platform->Mips[I];
            const int64 SizeBytes = int64(Source.Pixels.Num()) * sizeof(FFloat16Color);
            Mip.BulkData.Lock(LOCK_READ_WRITE);
            void* Destination = Mip.BulkData.Realloc(SizeBytes);
            FMemory::Memcpy(Destination, Source.Pixels.GetData(), SizeBytes);
            Mip.BulkData.Unlock();
        }
        // No Source art is installed: UE TextureDerivedData.cpp's Source.IsValid
        // guard leaves these supplied platform mips intact. UpdateResource queues
        // InitRHI; TextureCube.cpp splits each bulk mip into six face-major slices.
        Texture->UpdateResource();
        return Texture;
    }
}
