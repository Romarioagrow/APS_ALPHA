#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"

namespace APSStellarViewOptics
{
/** Angular tangent covered by one native output pixel at the optical axis.
 * The actual constrained view rectangle and projection already encode FOV-axis
 * policy, camera aspect ratio and letterboxing. Scene screen percentage is not
 * part of this calculation: late point materials render at native output size.
 */
inline bool TryPixelTangent(const FMatrix& Projection, const FIntRect& ViewRect,
	double& OutPixelTangent)
{
	if (ViewRect.Width() <= 0 || ViewRect.Height() <= 0
		|| !FMath::IsFinite(Projection.M[0][0]) || !FMath::IsFinite(Projection.M[1][1])
		|| !FMath::IsFinite(Projection.M[2][3]) || !FMath::IsFinite(Projection.M[3][3])
		|| !FMath::IsNearlyZero(Projection.M[3][3])
		|| FMath::IsNearlyZero(Projection.M[2][3])) return false;
	const double FocalX = 0.5 * static_cast<double>(ViewRect.Width())
		* FMath::Abs(static_cast<double>(Projection.M[0][0]));
	const double FocalY = 0.5 * static_cast<double>(ViewRect.Height())
		* FMath::Abs(static_cast<double>(Projection.M[1][1]));
	if (!FMath::IsFinite(FocalX) || !FMath::IsFinite(FocalY)
		|| FocalX <= UE_SMALL_NUMBER || FocalY <= UE_SMALL_NUMBER) return false;
	// The axes agree for square output pixels. If a custom projection stretches
	// them, retain minimum optical coverage on both axes instead of undersizing.
	OutPixelTangent = 1.0 / FMath::Min(FocalX, FocalY);
	return FMath::IsFinite(OutPixelTangent) && OutPixelTangent > 0.0;
}

/** Resolve once per view refresh, never once per catalogue instance. Keep the
 * caller's previous valid optics only when no local perspective view exists.
 */
inline double PixelTangent(APlayerController* Controller, const double LegacyFallback)
{
	ULocalPlayer* Player = IsValid(Controller) ? Controller->GetLocalPlayer() : nullptr;
	FViewport* Viewport = IsValid(Player) && IsValid(Player->ViewportClient)
		? Player->ViewportClient->Viewport : nullptr;
	FSceneViewProjectionData Projection;
	double Result = 0.0;
	if (Viewport && Player->GetProjectionData(Viewport, Projection)
		&& Projection.IsPerspectiveProjection()
		&& TryPixelTangent(Projection.ProjectionMatrix, Projection.GetConstrainedViewRect(), Result))
	{
		return Result;
	}
	return FMath::IsFinite(LegacyFallback) && LegacyFallback > 0.0 ? LegacyFallback : 0.0;
}
}
