// Rio 03.10 (galaxy phase 3): the scene view extension. It subscribes after motion blur (post-TSR, pre-bloom):
// stars and glow are stable under TSR (no jitter, no missing velocities), still depth-tested against the
// scene, and bloom/eye adaptation see them (the engine regenerates the half-res chain after this pass).
#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

struct FPostProcessMaterialInputs;
struct FScreenPassTexture;

class FAPSStarViewExtension final : public FSceneViewExtensionBase
{
public:
	explicit FAPSStarViewExtension(const FAutoRegister& AutoRegister);

	virtual void SetupViewFamily(FSceneViewFamily& /*InViewFamily*/) override {}
	virtual void SetupView(FSceneViewFamily& /*InViewFamily*/, FSceneView& /*InView*/) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& /*InViewFamily*/) override {}
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks,
		bool bIsPassEnabled) override;

protected:
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

private:
	FScreenPassTexture PostMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs);
};
