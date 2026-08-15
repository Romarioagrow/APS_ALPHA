#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"

/** Render-only stellar catalog layers. Canonical records never depend on this enum. */
enum class EAPSCanonicalStellarProxyLayer : uint8
{
	Galaxy,
	StarCluster
};

/**
 * The physical/model radius remains canonical. A later bounded angular LOD may
 * improve readability, but it is not allowed to feed back into model data.
 */
enum class EAPSStellarProxyRadiusPolicy : uint8
{
	PreserveCanonicalRatiosThenBoundedAngularLod
};

/**
 * One immutable affine mapping from canonical catalog units into a precision-safe
 * local render envelope. Canonical subtraction and scaling are performed in double
 * before FVector storage; no camera, frame or time input is permitted here.
 */
struct APS_ALPHA_API FAPSCanonicalStellarProjectionFrame
{
	static constexpr uint32 CurrentVersion = 3u;

	uint32 ProjectionVersion{CurrentVersion};
	uint32 ContextHash{0u};
	/** Fixed layer-local -> canonical-root layout transform. It never scales physical radii. */
	FVector LayerOriginCanonicalUnits{FVector::ZeroVector};
	double LayerToRootPositionScale{1.0};
	FVector CanonicalAnchorCm{FVector::ZeroVector};
	FVector RenderAnchorCm{FVector::ZeroVector};
	double CanonicalCmPerUnit{1.0};
	double PositionScale{1.0};
	double CanonicalHalfExtentUnits{0.0};
	double ProxyHalfExtentCm{0.0};
	double MaxProxyCoordinateCm{0.0};
	EAPSStellarProxyRadiusPolicy RadiusPolicy{
		EAPSStellarProxyRadiusPolicy::PreserveCanonicalRatiosThenBoundedAngularLod};
	double VisualRadiusFloorFraction{0.0};
	double VisualRadiusCeilingFraction{0.0};
	double VisualRadiusClassExponent{0.30};
	double VisualRadiusMinClassScale{0.60};
	double VisualRadiusMaxClassScale{1.80};
	bool bEnabled{false};

	/** Layer-local canonical coordinates composed into the shared root domain. */
	FVector ComposeCanonicalRootUnits(const FVector& CanonicalUnits) const
	{
		return LayerOriginCanonicalUnits + CanonicalUnits * LayerToRootPositionScale;
	}

	/**
	 * Canonical root address before the render anchor/scale is applied. This is
	 * suitable for semantic distance calculations and must never be treated as a
	 * render-world position.
	 */
	FVector GetCanonicalRootPositionCm(const FVector& CanonicalUnits) const
	{
		return ComposeCanonicalRootUnits(CanonicalUnits) * CanonicalCmPerUnit;
	}

	double GetCanonicalDistanceFromAnchorCm(const FVector& CanonicalUnits) const
	{
		return FVector::Distance(GetCanonicalRootPositionCm(CanonicalUnits),
			CanonicalAnchorCm);
	}

	FVector ProjectCanonicalUnits(const FVector& CanonicalUnits) const
	{
		const FVector CanonicalRootCm = GetCanonicalRootPositionCm(CanonicalUnits);
		return FVector(
			RenderAnchorCm.X + (CanonicalRootCm.X - CanonicalAnchorCm.X) * PositionScale,
			RenderAnchorCm.Y + (CanonicalRootCm.Y - CanonicalAnchorCm.Y) * PositionScale,
			RenderAnchorCm.Z + (CanonicalRootCm.Z - CanonicalAnchorCm.Z) * PositionScale);
	}

	FVector UnprojectToCanonicalUnits(const FVector& ProxyPositionCm) const
	{
		const double SafeScale = FMath::Abs(PositionScale) > UE_DOUBLE_SMALL_NUMBER
			? PositionScale : 1.0;
		const double SafeCmPerUnit = FMath::Abs(CanonicalCmPerUnit) > UE_DOUBLE_SMALL_NUMBER
			? CanonicalCmPerUnit : 1.0;
		const FVector RootCanonicalUnits(
			(CanonicalAnchorCm.X + (ProxyPositionCm.X - RenderAnchorCm.X) / SafeScale)
				/ SafeCmPerUnit,
			(CanonicalAnchorCm.Y + (ProxyPositionCm.Y - RenderAnchorCm.Y) / SafeScale)
				/ SafeCmPerUnit,
			(CanonicalAnchorCm.Z + (ProxyPositionCm.Z - RenderAnchorCm.Z) / SafeScale)
				/ SafeCmPerUnit);
		const double SafeLayerScale = FMath::Abs(LayerToRootPositionScale)
			> UE_DOUBLE_SMALL_NUMBER ? LayerToRootPositionScale : 1.0;
		return (RootCanonicalUnits - LayerOriginCanonicalUnits) / SafeLayerScale;
	}

	double ProjectCanonicalLengthUnits(const double CanonicalLengthUnits) const
	{
		return CanonicalLengthUnits * LayerToRootPositionScale
			* CanonicalCmPerUnit * PositionScale;
	}

	double UnprojectProxyLengthCm(const double ProxyLengthCm) const
	{
		const double Denominator = LayerToRootPositionScale
			* CanonicalCmPerUnit * PositionScale;
		return FMath::Abs(Denominator) > UE_DOUBLE_SMALL_NUMBER
			? ProxyLengthCm / Denominator : 0.0;
	}

	double GetEquivalentComponentScale() const
	{
		return LayerToRootPositionScale * CanonicalCmPerUnit * PositionScale;
	}

	bool IsFinite() const
	{
		return !LayerOriginCanonicalUnits.ContainsNaN()
			&& !CanonicalAnchorCm.ContainsNaN() && !RenderAnchorCm.ContainsNaN()
			&& FMath::IsFinite(LayerToRootPositionScale)
			&& LayerToRootPositionScale > 0.0
			&& FMath::IsFinite(CanonicalCmPerUnit) && CanonicalCmPerUnit > 0.0
			&& FMath::IsFinite(PositionScale) && PositionScale > 0.0
			&& FMath::IsFinite(MaxProxyCoordinateCm) && MaxProxyCoordinateCm >= 0.0
			&& FMath::IsFinite(VisualRadiusFloorFraction)
			&& FMath::IsFinite(VisualRadiusCeilingFraction)
			&& FMath::IsFinite(VisualRadiusClassExponent)
			&& FMath::IsFinite(VisualRadiusMinClassScale)
			&& FMath::IsFinite(VisualRadiusMaxClassScale)
			&& VisualRadiusFloorFraction >= 0.0
			&& VisualRadiusCeilingFraction >= VisualRadiusFloorFraction
			&& VisualRadiusClassExponent > 0.0
			&& VisualRadiusMinClassScale > 0.0
			&& VisualRadiusMaxClassScale >= VisualRadiusMinClassScale;
	}
};

/** Read-only runtime descriptor shared by menu and gameplay consumers. */
struct APS_ALPHA_API FAPSCanonicalStellarProjectionDescriptor
{
	uint32 ProjectionVersion{FAPSCanonicalStellarProjectionFrame::CurrentVersion};
	uint32 ContextHash{0u};
	/** Hash of immutable seeds/config/counts/home identity; independent of render budget and view. */
	uint32 CanonicalDatasetHash{0u};
	uint32 CanonicalDatasetVersion{0u};
	uint32 CanonicalDatasetInputHash{0u};
	uint64 CanonicalDatasetBuildSerial{0u};
	int32 CanonicalDatasetRecordCount{0};
	/** Compatibility alias for older diagnostics; always equals CanonicalDatasetHash. */
	uint32 CanonicalIdentityHash{0u};
	uint32 RenderedMappingHash{0u};
	FAPSCanonicalStellarProjectionFrame Galaxy;
	FAPSCanonicalStellarProjectionFrame StarCluster;
	uint64 ProxyBuildSerial{0u};
	uint64 InstanceUploadCount{0u};
	uint64 TransformMutationSerial{0u};
	int64 GalaxyModeledCount{0};
	int32 GalaxyRenderedCount{0};
	int32 ClusterModeledCount{0};
	int32 ClusterRenderedCount{0};
	FGuid MaterializedHomeStableId;
	int32 MaterializedHomeInstanceIndex{INDEX_NONE};
	double MaxObservedMatrixMagnitudeCm{0.0};
	double GalaxyMaxObservedMatrixMagnitudeCm{0.0};
	double ClusterMaxObservedMatrixMagnitudeCm{0.0};
	double ClusterToGalaxyPositionScale{1.0};
	double ViewDistanceScale{1.0};
	double GalaxyViewVisualScale{1.0};
	double ClusterViewVisualScale{1.0};
	FVector ViewAnchorWorldCm{FVector::ZeroVector};
	bool bMaterializedHomeProxySuppressed{false};
	/** True when this generator consumed the already-finalized persisted dataset. */
	bool bConsumedFinalizedDataset{false};
	bool bMappingsComplete{false};
	bool bUnitRoots{false};
	bool bBoundsValid{false};
	bool bProjectionValid{false};
	bool bFinalized{false};
};

/** One exact canonical record and its current render counterpart. */
struct APS_ALPHA_API FAPSCanonicalStellarProxyRecord
{
	EAPSCanonicalStellarProxyLayer Layer{EAPSCanonicalStellarProxyLayer::Galaxy};
	FGuid StableId;
	int64 CanonicalIndex{INDEX_NONE};
	int32 InstanceIndex{INDEX_NONE};
	FVector CanonicalPositionUnits{FVector::ZeroVector};
	double CanonicalPhysicalRadiusSolar{0.0};
	/** Linear physical-radius projection. This is independent of layer layout scale and visual LOD. */
	double ExpectedPhysicalProxyRadiusCm{0.0};
	/** Immutable construction-time impostor radius after the explicit bounded visual LOD policy. */
	double AppliedVisualProxyRadiusCm{0.0};
	FVector ExpectedBaseProxyPositionCm{FVector::ZeroVector};
	double ExpectedBaseProxyScale{0.0};
	FVector ActualProxyPositionCm{FVector::ZeroVector};
	double ActualProxyScale{0.0};
	double ProjectionErrorCm{TNumericLimits<double>::Max()};
	double MaxMatrixMagnitudeCm{TNumericLimits<double>::Max()};
	bool bSuppressedMaterializedHome{false};
	bool bSuppressedByView{false};
};

/** Canonical semantic address plus its immutable render counterpart. */
struct APS_ALPHA_API FAPSCanonicalClusterSystemAddress
{
	uint32 ProjectionVersion{0u};
	uint32 ContextHash{0u};
	uint32 CanonicalDatasetHash{0u};
	FGuid StableId;
	int32 CanonicalIndex{INDEX_NONE};
	int32 InstanceIndex{INDEX_NONE};
	FVector CanonicalRootPositionCm{FVector::ZeroVector};
	FVector CanonicalDeltaFromHomeCm{FVector::ZeroVector};
	double CanonicalDistanceFromHomeCm{0.0};
	FVector ImmutableProxyWorldLocationCm{FVector::ZeroVector};
	double CanonicalRadiusSolar{0.0};
	bool bMaterializedHome{false};
};

namespace APSCanonicalStellarProjection
{
	inline constexpr double FullScaleCanonicalCmPerUnit = 1.0e9;
	inline constexpr double SolarRadiusCm = 6.957e10;
	inline constexpr double GalaxyMaxProxyCoordinateCm = 1.0e8;
	inline constexpr double ClusterMaxProxyCoordinateCm = 1.6e7;
	inline constexpr double ProjectionToleranceCm = 0.01;
	inline constexpr int32 SharedHomeSelectionWindow = 100;
	// Immutable equivalents of the accepted parent-scope point sizes. They replace
	// the legacy per-focus O(N) HISM scale rewrites with one construction-time LOD.
	inline constexpr double GalaxyImpostorFloorFraction = 0.00896;
	inline constexpr double ClusterImpostorFloorFraction = 0.00720;
	inline constexpr double ImpostorCeilingFraction = 0.02000;
	inline constexpr double ImpostorClassExponent = 0.30;
	inline constexpr double ImpostorMinClassScale = 0.60;
	inline constexpr double ImpostorMaxClassScale = 1.80;

	inline double MaxAbsComponent(const FVector& Value)
	{
		return FMath::Max3(FMath::Abs(Value.X), FMath::Abs(Value.Y), FMath::Abs(Value.Z));
	}

	inline double TransformMatrixMagnitude(const FTransform& Transform)
	{
		const FMatrix Matrix = Transform.ToMatrixWithScale();
		double Maximum = 0.0;
		for (int32 Row = 0; Row < 4; ++Row)
		{
			for (int32 Column = 0; Column < 4; ++Column)
			{
				Maximum = FMath::Max(Maximum, FMath::Abs(static_cast<double>(Matrix.M[Row][Column])));
			}
		}
		return Maximum;
	}

	inline uint32 HashQuantizedDouble(const double Value, const double Quantum = 0.001)
	{
		const int64 Quantized = FMath::RoundToInt64(Value / FMath::Max(Quantum, UE_DOUBLE_SMALL_NUMBER));
		return HashCombine(GetTypeHash(static_cast<uint32>(Quantized)),
			GetTypeHash(static_cast<uint32>(static_cast<uint64>(Quantized) >> 32u)));
	}

	struct FNestedCatalogPermutation
	{
		int64 Count{0};
		int64 Start{0};
		int64 Step{1};

		int64 Resolve(const int32 RenderOrdinal) const
		{
			return Count > 0 && RenderOrdinal >= 0
				? (Start + static_cast<int64>(RenderOrdinal) * Step) % Count
				: INDEX_NONE;
		}
	};

	inline int64 GreatestCommonDivisor(int64 A, int64 B)
	{
		A = FMath::Abs(A);
		B = FMath::Abs(B);
		while (B != 0)
		{
			const int64 Remainder = A % B;
			A = B;
			B = Remainder;
		}
		return A;
	}

	/** A deterministic full-cycle order whose first N records are a prefix of every larger LOD. */
	inline FNestedCatalogPermutation MakeNestedCatalogPermutation(
		const int32 GenerationSeed, const int64 ModeledCount)
	{
		FNestedCatalogPermutation Result;
		Result.Count = FMath::Max<int64>(ModeledCount, int64(0));
		if (Result.Count <= 1)
		{
			Result.Start = 0;
			Result.Step = 1;
			return Result;
		}
		const uint32 StartHash = HashCombine(GetTypeHash(GenerationSeed), 0x9e3779b9u);
		const uint32 StepHash = HashCombine(GetTypeHash(GenerationSeed), 0x85ebca6bu);
		Result.Start = static_cast<int64>(StartHash) % Result.Count;
		Result.Step = 1 + static_cast<int64>(StepHash) % (Result.Count - 1);
		while (GreatestCommonDivisor(Result.Step, Result.Count) != 1)
		{
			Result.Step = Result.Step + 1 < Result.Count ? Result.Step + 1 : 1;
		}
		return Result;
	}

	inline FAPSCanonicalStellarProjectionFrame MakeBoundedFrame(
		const double CanonicalHalfExtentUnits, const double MaxProxyCoordinateCm,
		const uint32 ContextHash, const bool bEnabled)
	{
		FAPSCanonicalStellarProjectionFrame Frame;
		Frame.ContextHash = ContextHash;
		Frame.CanonicalHalfExtentUnits = FMath::Max(CanonicalHalfExtentUnits, 0.0);
		Frame.MaxProxyCoordinateCm = FMath::Max(MaxProxyCoordinateCm, 0.0);
		Frame.bEnabled = bEnabled;
		if (bEnabled && Frame.CanonicalHalfExtentUnits > UE_DOUBLE_SMALL_NUMBER)
		{
			Frame.CanonicalCmPerUnit = FullScaleCanonicalCmPerUnit;
			const double CanonicalHalfExtentCm =
				Frame.CanonicalHalfExtentUnits * Frame.CanonicalCmPerUnit;
			Frame.PositionScale = Frame.MaxProxyCoordinateCm / CanonicalHalfExtentCm;
			Frame.ProxyHalfExtentCm = Frame.MaxProxyCoordinateCm;
		}
		else
		{
			Frame.CanonicalCmPerUnit = 1.0;
			Frame.PositionScale = 1.0;
			Frame.ProxyHalfExtentCm = Frame.CanonicalHalfExtentUnits;
			Frame.MaxProxyCoordinateCm = Frame.CanonicalHalfExtentUnits;
		}
		return Frame;
	}

	/**
	 * Composes both catalog layers through one home-centred root affine mapping.
	 * ClusterToGalaxyPositionScale applies to layout coordinates only; physical
	 * stellar radii use ProjectPhysicalRadiusSolar and therefore remain comparable.
	 */
	inline bool ConfigureSharedHomeCentredFrames(
		const double GalaxyHalfExtentUnits, const double ClusterHalfExtentUnits,
		const double ClusterToGalaxyPositionScale, const FVector& HomeClusterLocalUnits,
		const uint32 ContextHash, FAPSCanonicalStellarProjectionFrame& OutGalaxy,
		FAPSCanonicalStellarProjectionFrame& OutCluster)
	{
		const double SafeGalaxyExtent = FMath::Max(GalaxyHalfExtentUnits, 0.0);
		const double SafeClusterExtent = FMath::Max(ClusterHalfExtentUnits, 0.0);
		const double SafeClusterScale = FMath::Max(ClusterToGalaxyPositionScale,
			UE_DOUBLE_SMALL_NUMBER);
		const FVector HomeRootUnits = HomeClusterLocalUnits * SafeClusterScale;
		const double HomeRootMagnitude = MaxAbsComponent(HomeRootUnits);
		const double AnchoredGalaxyExtent = SafeGalaxyExtent + HomeRootMagnitude;
		const double AnchoredClusterExtent = SafeClusterExtent * SafeClusterScale
			+ HomeRootMagnitude;
		if (AnchoredGalaxyExtent <= UE_DOUBLE_SMALL_NUMBER
			|| AnchoredClusterExtent <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}

		const double GalaxyScale = GalaxyMaxProxyCoordinateCm
			/ (AnchoredGalaxyExtent * FullScaleCanonicalCmPerUnit);
		const double ClusterScale = ClusterMaxProxyCoordinateCm
			/ (AnchoredClusterExtent * FullScaleCanonicalCmPerUnit);
		const double SharedRootScale = FMath::Min(GalaxyScale, ClusterScale);
		if (!FMath::IsFinite(SharedRootScale) || SharedRootScale <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}

		const auto ConfigureLayer = [&](FAPSCanonicalStellarProjectionFrame& Frame,
			const double LayerHalfExtentUnits, const double LayerScale,
			const double LayerMaximumCm, const double VisualFloorFraction)
		{
			Frame = FAPSCanonicalStellarProjectionFrame{};
			Frame.ContextHash = ContextHash;
			Frame.LayerToRootPositionScale = LayerScale;
			Frame.CanonicalAnchorCm = HomeRootUnits * FullScaleCanonicalCmPerUnit;
			Frame.RenderAnchorCm = FVector::ZeroVector;
			Frame.CanonicalCmPerUnit = FullScaleCanonicalCmPerUnit;
			Frame.PositionScale = SharedRootScale;
			Frame.CanonicalHalfExtentUnits = LayerHalfExtentUnits;
			Frame.ProxyHalfExtentCm = LayerHalfExtentUnits * LayerScale
				* FullScaleCanonicalCmPerUnit * SharedRootScale;
			Frame.MaxProxyCoordinateCm = LayerMaximumCm;
			Frame.VisualRadiusFloorFraction = VisualFloorFraction;
			Frame.VisualRadiusCeilingFraction = ImpostorCeilingFraction;
			Frame.VisualRadiusClassExponent = ImpostorClassExponent;
			Frame.VisualRadiusMinClassScale = ImpostorMinClassScale;
			Frame.VisualRadiusMaxClassScale = ImpostorMaxClassScale;
			Frame.bEnabled = true;
		};
		ConfigureLayer(OutGalaxy, SafeGalaxyExtent, 1.0, GalaxyMaxProxyCoordinateCm,
			GalaxyImpostorFloorFraction);
		ConfigureLayer(OutCluster, SafeClusterExtent, SafeClusterScale,
			ClusterMaxProxyCoordinateCm, ClusterImpostorFloorFraction);
		return OutGalaxy.IsFinite() && OutCluster.IsFinite();
	}

	inline double ProjectPhysicalRadiusSolar(
		const FAPSCanonicalStellarProjectionFrame& Frame, const double RadiusSolar)
	{
		return FMath::Max(RadiusSolar, 0.0) * SolarRadiusCm * Frame.PositionScale;
	}

	inline double UnprojectPhysicalRadiusSolar(
		const FAPSCanonicalStellarProjectionFrame& Frame, const double ProxyRadiusCm)
	{
		const double Denominator = SolarRadiusCm * Frame.PositionScale;
		return FMath::Abs(Denominator) > UE_DOUBLE_SMALL_NUMBER
			? ProxyRadiusCm / Denominator : 0.0;
	}

	inline double GetAppliedVisualRadiusCm(const EAPSCanonicalStellarProxyLayer Layer,
		const FAPSCanonicalStellarProjectionFrame& Frame, const double PhysicalRadiusSolar)
	{
		const double FallbackFloor = Layer == EAPSCanonicalStellarProxyLayer::Galaxy
			? GalaxyImpostorFloorFraction : ClusterImpostorFloorFraction;
		const double FloorFraction = Frame.VisualRadiusFloorFraction > 0.0
			? Frame.VisualRadiusFloorFraction : FallbackFloor;
		const double CeilingFraction = Frame.VisualRadiusCeilingFraction >= FloorFraction
			? Frame.VisualRadiusCeilingFraction : ImpostorCeilingFraction;
		const double ClassScale = FMath::Clamp(FMath::Pow(
			FMath::Max(PhysicalRadiusSolar, 0.08),
			Frame.VisualRadiusClassExponent > 0.0
				? Frame.VisualRadiusClassExponent : ImpostorClassExponent),
			Frame.VisualRadiusMinClassScale > 0.0
				? Frame.VisualRadiusMinClassScale : ImpostorMinClassScale,
			Frame.VisualRadiusMaxClassScale > 0.0
				? Frame.VisualRadiusMaxClassScale : ImpostorMaxClassScale);
		const double MinimumRadiusCm = Frame.ProxyHalfExtentCm * FloorFraction * ClassScale;
		const double MaximumRadiusCm = Frame.ProxyHalfExtentCm * CeilingFraction;
		return FMath::Clamp(ProjectPhysicalRadiusSolar(Frame, PhysicalRadiusSolar),
			FMath::Min(MinimumRadiusCm, MaximumRadiusCm), MaximumRadiusCm);
	}

	inline double GetCanonicalStellarRadiusSolar(const ESpectralClass SpectralClass)
	{
		switch (SpectralClass)
		{
		case ESpectralClass::O: return 9.0;
		case ESpectralClass::B: return 4.8;
		case ESpectralClass::A: return 2.1;
		case ESpectralClass::F: return 1.35;
		case ESpectralClass::G: return 1.0;
		case ESpectralClass::K: return 0.78;
		case ESpectralClass::M: return 0.42;
		case ESpectralClass::L: return 0.18;
		case ESpectralClass::T: return 0.14;
		case ESpectralClass::Y: return 0.11;
		case ESpectralClass::NS: return 0.08;
		case ESpectralClass::PS: return 2.8;
		case ESpectralClass::BH: return 0.12;
		default: return 1.0;
		}
	}

	inline double GetCanonicalStellarLuminositySolar(const ESpectralClass SpectralClass)
	{
		switch (SpectralClass)
		{
		case ESpectralClass::O: return 60000.0;
		case ESpectralClass::B: return 1200.0;
		case ESpectralClass::A: return 55.0;
		case ESpectralClass::F: return 6.0;
		case ESpectralClass::G: return 1.0;
		case ESpectralClass::K: return 0.35;
		case ESpectralClass::M: return 0.045;
		case ESpectralClass::L: return 0.008;
		case ESpectralClass::T: return 0.003;
		case ESpectralClass::Y: return 0.001;
		case ESpectralClass::NS: return 35.0;
		case ESpectralClass::PS: return 140.0;
		case ESpectralClass::BH: return 0.0;
		default: return 1.0;
		}
	}

	inline int32 SelectSharedHomeInstanceIndex(
		const int32 WorldSeed, const int32 ClusterSeed, const int32 CanonicalModeledCount)
	{
		if (CanonicalModeledCount <= 0)
		{
			return INDEX_NONE;
		}
		const int32 SharedCount = FMath::Min(CanonicalModeledCount, SharedHomeSelectionWindow);
		FRandomStream Stream(static_cast<int32>(HashCombine(
			GetTypeHash(WorldSeed), GetTypeHash(ClusterSeed)) & 0x7fffffffu));
		return Stream.RandRange(0, FMath::Max(SharedCount - 1, 0));
	}

	inline bool SuppressExactMaterializedProxy(
		const FGuid& RecordStableId, const FGuid& MaterializedStableId,
		FTransform& InOutProxyTransform)
	{
		if (!RecordStableId.IsValid() || RecordStableId != MaterializedStableId)
		{
			return false;
		}
		InOutProxyTransform.SetScale3D(FVector::ZeroVector);
		return true;
	}
}
