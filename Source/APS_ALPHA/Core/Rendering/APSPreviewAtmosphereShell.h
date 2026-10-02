#pragma once

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace APSPreviewAtmosphereShell
{
    // Compare in the presented frame, never against the physical actor radius:
    // the same view also serves compressed/translated preview bodies.
    inline bool IsInside(const FVector& Camera, const FVector& Center, double Radius)
    {
        return !Camera.ContainsNaN() && !Center.ContainsNaN()
            && FMath::IsFinite(Radius) && Radius > 0.0
            && FVector::DistSquared(Camera, Center) < Radius * Radius;
    }

    inline UStaticMeshComponent* Select(UStaticMeshComponent* Outside,
        UStaticMeshComponent* Inside, const FVector& Camera, const FVector& Center,
        double Radius, const FQuat& Rotation)
    {
        if (!IsValid(Outside) || !IsValid(Inside) || !IsValid(Inside->GetStaticMesh()))
            return Outside;
        const double AssetRadius = Inside->GetStaticMesh()->GetBounds().BoxExtent.GetMax();
        if (!FMath::IsFinite(Radius) || Radius <= 0.0 || AssetRadius <= UE_SMALL_NUMBER)
            return Outside;

        // The inward-facing mesh must follow the same committed shell as the
        // outward-facing mesh. Enabling its old physical-scale transform would
        // reintroduce the oversized atmosphere during preview hand-offs.
        const FVector Scale(Radius / AssetRadius);
        const FVector Location = Center - Rotation.RotateVector(
            Inside->GetStaticMesh()->GetBounds().Origin * Scale);
        const FTransform Target(Rotation, Location, Scale);
        if (!Inside->GetComponentTransform().Equals(Target, 1.e-6))
            Inside->SetWorldTransform(Target, false, nullptr, ETeleportType::TeleportPhysics);
        // Same optical integration and uniforms on both sides; only mesh winding
        // differs. Do not add the separate skylight/absorption/outer layers here.
        if (Inside->GetMaterial(0) != Outside->GetMaterial(0))
            Inside->SetMaterial(0, Outside->GetMaterial(0));
        return IsInside(Camera, Center, Radius) ? Inside : Outside;
    }
}
