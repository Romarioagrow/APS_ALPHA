#pragma once
#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/APSClosedGlobeMesh.h"

class APlanetaryBody;
class UMaterialInstanceDynamic;
class UWorld;

// Default-off candidate ownership, NOT seamless moving/global coverage. The first
// planet-local front anchor remains fixed. No material selection or geometry writes.
namespace APSPlanetReliefRuntime
{
    APS_ALPHA_API bool IsEnabled(); // GT; aps.Surface.CanonicalRelief defaults to 0.
    // GT: Frame/Signature MUST come from APSNativeGlobeSnapshot::Capture. Register
    // every existing root/globe MID separately; identical snapshots reuse data.
    // Optional anchor is a planet-local direction, never a world-space position.
    APS_ALPHA_API bool Register(APlanetaryBody* Body, const APSClosedGlobeMesh::FSamplingFrame& Frame,
        uint32 Signature, UMaterialInstanceDynamic* Material, const FVector3d* PlanetLocalAnchor = nullptr);
    APS_ALPHA_API void Tick(UWorld* World, APlanetaryBody* ActiveBody, APlanetaryBody* ArrivingBody);
    // Nonblocking; cancels work and restores only still-owned parameter values.
    APS_ALPHA_API void Release(UWorld* World);
}
