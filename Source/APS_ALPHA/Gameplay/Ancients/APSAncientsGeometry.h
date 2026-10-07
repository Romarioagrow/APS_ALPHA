#pragma once

#include "CoreMinimal.h"
#include "APSAncientsTypes.h"
#include "ProceduralMeshComponent.h"

/**
 * The Builders' shapes as procedural geometry (Docs/Design/ANCIENT_STRUCTURES.md, "Visuals"): boxes and faceted
 * frustums, flat-shaded, gathered into one stone mesh and two glow meshes per site, so a whole monument costs two or
 * three draw calls, one collision body and no lights. Built once, in the site's own frame: X forward, Y right, Z up from
 * the ground at its centre, centimetres.
 */
struct FAPSAncientMesh
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;

	/** A box: its centre, half its size along its own axes and its turn. */
	void Box(const FVector& Centre, const FVector& HalfSize, const FQuat& Rotation = FQuat::Identity);
	/**
	 * A frustum along its own Z: Sides faces between a bottom and a top ring (a cone when TopRadius is 0, a prism when the
	 * radii match), closed at both ends. Smooth spreads the normals round the sides (hulls), else every face is flat.
	 */
	void Frustum(const FVector& Centre, const FQuat& Rotation, double BottomRadius, double TopRadius, double HalfHeight,
		int32 Sides, bool bSmooth = false);
	bool IsEmpty() const { return Triangles.IsEmpty(); }
	int32 NumTriangles() const { return Triangles.Num() / 3; }

private:
	/** Four corners in order round the face; the winding follows the engine's front faces whatever the corner order. */
	void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal, const FVector& UAxis);
	void QuadSmooth(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& NormalA,
		const FVector& NormalB, const FVector& NormalC, const FVector& NormalD, const FVector& UAxis);
	void Tri(const FVector& A, const FVector& B, const FVector& C, const FVector& Normal, const FVector& UAxis);
	void AddVertex(const FVector& Position, const FVector& Normal, const FVector& UAxis);
};

/** Ground under the site's own plane: the height along its up (cm) relative to its centre. */
struct FAPSAncientGround
{
	virtual ~FAPSAncientGround() = default;
	virtual double Z(double X, double Y) const = 0;
};

/** A site's look and measure: stone (collides, casts shadows), glow (the glyph seams) and beacon (crowns, brighter). */
struct FAPSAncientShape
{
	FAPSAncientMesh Stone;
	FAPSAncientMesh Glow;
	FAPSAncientMesh Beacon;
	APSAncients::FMetrics Metrics;
	FLinearColor StoneColour{0.03f, 0.03f, 0.035f};
	FLinearColor GlowColour{0.25f, 0.9f, 1.0f};
};

namespace APSAncientsGeometry
{
	/**
	 * Builds the site's shape on its ground. Without Ground it only measures (the footprint the site search needs, the
	 * height): every random draw comes from the site's seed in the same order either way, so the measure matches the build.
	 */
	void Build(const APSAncients::FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out);
	/** The steepest ground (rise over run) a kind accepts under its footprint. */
	double MaxSlope(APSAncients::EKind Kind);
}
