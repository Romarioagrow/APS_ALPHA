#include "APSM5CollisionAuthoring.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/SecureHash.h"

namespace
{
FString GeometryKey(const FKConvexElem& Elem)
{
    TArray<FString> Vertices;
    Vertices.Reserve(Elem.VertexData.Num());
    for (const FVector& P : Elem.VertexData)
    {
        Vertices.Add(FString::Printf(TEXT("%lld,%lld,%lld"),
            static_cast<long long>(FMath::RoundToInt64(P.X * 100.0)),
            static_cast<long long>(FMath::RoundToInt64(P.Y * 100.0)),
            static_cast<long long>(FMath::RoundToInt64(P.Z * 100.0))));
    }
    Vertices.Sort();
    const FString Text = FString::Join(Vertices, TEXT(";")) + TEXT("|") + Elem.GetTransform().ToString();
    const FTCHARToUTF8 Utf8(*Text);
    uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Digest);
    return BytesToHex(Digest, FSHA1::DigestSize);
}

FString Describe(const FKConvexElem& Elem)
{
    return FString::Printf(TEXT("%s\t%s\t%d\t%d"), *GeometryKey(Elem), *Elem.GetName().ToString(),
        static_cast<int32>(Elem.GetCollisionEnabled()), Elem.GetContributeToMass() ? 1 : 0);
}
}

FString UAPSM5CollisionAuthoring::GetConvexSnapshot(UStaticMesh* Mesh)
{
    if (!Mesh || !Mesh->GetBodySetup()) return TEXT("ERROR missing mesh or BodySetup");
    TArray<FString> Lines;
    for (const FKConvexElem& Elem : Mesh->GetBodySetup()->AggGeom.ConvexElems) Lines.Add(Describe(Elem));
    return FString::Join(Lines, TEXT("\n"));
}

FString UAPSM5CollisionAuthoring::ConfigureAddedHullSkin(UStaticMesh* Mesh, const FString& PriorSnapshot, int32 ExpectedAdded)
{
#if WITH_EDITOR
    if (!Mesh || !Mesh->GetBodySetup() || ExpectedAdded <= 0) return TEXT("ERROR invalid mesh/count");
    TArray<FString> Lines;
    PriorSnapshot.ParseIntoArrayLines(Lines, true);
    if (Lines.IsEmpty()) return TEXT("ERROR empty prior snapshot");
    TMap<FString, FString> Prior;
    for (const FString& Line : Lines)
    {
        FString Key, Rest;
        if (!Line.Split(TEXT("\t"), &Key, &Rest) || Key.Len() != FSHA1::DigestSize * 2 || Prior.Contains(Key))
            return TEXT("ERROR invalid/duplicate prior geometry");
        Prior.Add(Key, Line);
    }
    UBodySetup* Body = Mesh->GetBodySetup();
    if (Body->AggGeom.ConvexElems.Num() != Prior.Num() + ExpectedAdded) return TEXT("ERROR convex count mismatch");
    TSet<FString> Found;
    TArray<int32> Added;
    for (int32 Index = 0; Index < Body->AggGeom.ConvexElems.Num(); ++Index)
    {
        const FKConvexElem& Elem = Body->AggGeom.ConvexElems[Index];
        const FString Key = GeometryKey(Elem);
        if (const FString* Previous = Prior.Find(Key))
        {
            if (Found.Contains(Key) || Describe(Elem) != *Previous) return TEXT("ERROR fitted shape/filter changed");
            Found.Add(Key);
        }
        else Added.Add(Index);
    }
    if (Found.Num() != Prior.Num() || Added.Num() != ExpectedAdded) return TEXT("ERROR original shapes missing");
    Mesh->Modify();
    Body->Modify();
    for (const int32 Index : Added)
    {
        FKConvexElem& Elem = Body->AggGeom.ConvexElems[Index];
        Elem.SetName(FName(*(TEXT("M5_SKIN_") + GeometryKey(Elem))));
        Elem.SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        Elem.SetContributeToMass(false);
    }
    Mesh->MarkPackageDirty();
    Body->MarkPackageDirty();
    // Geometry is already cooked by FBX import. The filters are read from the
    // FKShapeElem when each runtime body is created; no geometry recook needed.
    return TEXT("PASS\n") + GetConvexSnapshot(Mesh);
#else
    return TEXT("ERROR offline editor operation only");
#endif
}
