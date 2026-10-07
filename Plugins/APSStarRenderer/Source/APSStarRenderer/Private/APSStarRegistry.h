// Rio 03.10 (galaxy phase 3): registry of point sets and glow volumes.
// Game side: a small locked table (what exists, which scene, enabled) for IsActiveThisFrame.
// Render side: FRenderState, touched only by render commands and the view extension (render thread).
#pragma once

#include "CoreMinimal.h"
#include "APSStarRendererAPI.h"
#include "Containers/StaticArray.h"
#include "HAL/CriticalSection.h"
#include "RenderGraphResources.h"
#include "RendererInterface.h"
#include "Templates/Function.h"
#include <atomic>

class FSceneInterface;
class FRHIGPUBufferReadback;

namespace APSStarRenderer::Private
{
	struct FPointSetRT
	{
		FHandle Handle = 0;
		const FSceneInterface* Scene = nullptr;
		FPointSetDesc Desc;
		int32 NumPoints = 0;
		int32 NumUploaded = 0;
		uint32 LastUploadFrame = MAX_uint32;
		/** CPU copy until fully uploaded, then freed. */
		TArray<FPackedStar> PendingPoints;
		TRefCountPtr<FRDGPooledBuffer> Buffer;
	};

	struct FGlowVolumeRT
	{
		FHandle Handle = 0;
		const FSceneInterface* Scene = nullptr;
		FGlowVolumeDesc Desc;
		/** Map metadata; the texel arrays are freed after the upload. */
		FGlowMap Map;
		TRefCountPtr<IPooledRenderTarget> EmissionTexture;
		TRefCountPtr<IPooledRenderTarget> ProfileTexture;
		TRefCountPtr<IPooledRenderTarget> ShapeTexture;
	};

	struct FStatsReadback
	{
		TUniquePtr<FRHIGPUBufferReadback> Readback;
		bool bInFlight = false;
		uint32 FrameNumber = 0;
	};

	struct FRenderState
	{
		FRenderState();
		~FRenderState();

		TArray<FPointSetRT> PointSets;
		TArray<FGlowVolumeRT> GlowVolumes;
		TMap<const FSceneInterface*, float> SceneVisibility;
		/** Rio 04.10: scenes whose points fade over a bright scene (SetWorldSkyMask). */
		TSet<const FSceneInterface*> SkyMaskScenes;
		TStaticArray<FVector4f, 256> Palette;
		TArray<FStatsReadback> Readbacks;

		/** Per-frame CPU counters, published when the frame number changes. */
		uint32 CounterFrame = MAX_uint32;
		int64 FrameSubmitted = 0;

		FPointSetRT* FindPointSet(FHandle Handle);
		FGlowVolumeRT* FindGlowVolume(FHandle Handle);
		float GetSceneVisibility(const FSceneInterface* Scene) const;
		/** Drops every set, volume and read-back (render thread). */
		void Reset();
	};

	class FRegistry
	{
	public:
		static FRegistry& Get();

		FHandle AllocateHandle();

		// Game thread.
		void AddEntry(FHandle Handle, bool bGlow, const UWorld* World, const FSceneInterface* Scene, bool bEnabled);
		void SetEntryEnabled(FHandle Handle, bool bEnabled);
		void RemoveEntry(FHandle Handle);
		void CollectWorldEntries(const UWorld* World, TArray<FHandle>& OutHandles) const;

		/** Any thread. Scene == nullptr matches every scene. */
		bool HasEnabledEntries(const FSceneInterface* Scene, bool bPoints, bool bGlow) const;
		int32 CountEntries(bool bGlow) const;

		/** Queues a mutation of the render state (render thread, in order). */
		void EnqueueRenderUpdate(TUniqueFunction<void(FRenderState&)>&& Update);
		/** Render thread only. */
		FRenderState& GetRenderState() { return *RenderState; }

		void PublishStats(const FStats& InStats);
		void PublishGpuCounters(uint32 FrameNumber, const uint32 Counters[4]);
		FStats GetStats() const;

		/** Engine pre-exit: clears the render state on the render thread and stops accepting work. */
		void Shutdown();
		bool IsShutdown() const { return bShutdown.load(); }

	private:
		FRegistry();

		struct FEntry
		{
			bool bGlow = false;
			bool bEnabled = true;
			const UWorld* World = nullptr;
			const FSceneInterface* Scene = nullptr;
		};

		mutable FCriticalSection EntryMutex;
		TMap<FHandle, FEntry> Entries;
		TSharedPtr<FRenderState, ESPMode::ThreadSafe> RenderState;
		std::atomic<uint32> NextHandle{1};
		std::atomic<bool> bShutdown{false};
		mutable FCriticalSection StatsMutex;
		FStats Stats;
	};
}
