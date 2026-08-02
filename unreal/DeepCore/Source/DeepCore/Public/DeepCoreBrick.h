// DeepCoreBrick.h : generic studded construction-toy geometry, emitted into UE mesh buffers.
//
// This is the Unreal port of src/game3d/Brick.hpp. The shapes and proportions are identical;
// what changes is where the triangles go. The OpenGL version pushed vertices through
// immediate mode, which meant the CPU walked the whole world every frame. Here geometry is
// accumulated into arrays once and handed to a UProceduralMeshComponent, so it lives on the
// GPU and gets shadows, Lumen bounce and post-processing for free.
//
// ON WHAT THIS IS AND IS NOT
// --------------------------
// These are GENERIC studded building blocks. The original stud-and-tube patents expired
// decades ago and compatible bricks are made by many companies, which is why generic brick
// geometry is fine to generate.
//
// What is NOT done here, deliberately:
//   - the LEGO name, logo, or any branding. It appears nowhere; no stud carries a mark.
//   - the minifigure. Its specific shape is a registered trade mark in several jurisdictions,
//     so the characters here are NOT minifigures: they have tapered bodies, articulated limbs
//     and rounded helmets rather than the cylindrical head, C-grip hands and trapezoidal torso
//     that define that design.
// Generic bricks, original characters. That distinction is the whole reason this file exists.
//
// UNITS
// Unreal works in centimetres. One tile of the map is one stud pitch is 100cm, which puts a
// crew member at roughly 150cm tall -- human scale, so the engine's default camera speeds,
// shadow cascades and Lumen tuning all behave sensibly without fighting them. Real bricks use
// a 5:6 ratio between stud pitch and brick height; keeping that is most of why a brick "looks
// right", and getting it wrong is instantly visible even to someone who could not say why.
//

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

namespace DeepCoreBrick
{
	/** One map tile. Gameplay is on this grid; the brickwork is finer than it. */
	static constexpr float TileSize     = 100.0f;

	/**
	 * Studs per tile edge.
	 *
	 * This is the number that decides whether the world reads as brick-built or as a tiled
	 * heightmap. At one stud per tile every tile is a metre-wide 1x1 brick, the crew look like
	 * ants standing on a baseplate, and the studs read as bumps in terrain rather than as
	 * construction. At four, each tile is an ordinary 4x4 brick, a crew member stands about
	 * six studs tall -- close to the ratio a minifigure has to a baseplate -- and the eye
	 * finally reads the floor as something that was BUILT.
	 */
	static constexpr int   StudsPerTile = 4;

	/** Centre-to-centre spacing of studs. */
	static constexpr float StudPitch   = TileSize / StudsPerTile;   // 25cm
	/** A plate. Three plates make a brick, as on the real thing. */
	static constexpr float PlateHeight = StudPitch * 0.40f;         // 10cm
	/** 5:6 against the stud pitch, which is the ratio a real brick uses. */
	static constexpr float BrickHeight = StudPitch * 1.20f;         // 30cm
	static constexpr float StudRadius  = StudPitch * 0.30f;         // 7.5cm
	static constexpr float StudHeight  = StudPitch * 0.18f;         // 4.5cm

	/** Ground level. Floor plates sit just below it and units stand on it. */
	static constexpr float FloorTop    = 0.0f;
}

/**
 * An accumulating triangle soup with everything UProceduralMeshComponent wants.
 *
 * Colour is per-vertex rather than per-material because the terrain is one mesh with hundreds
 * of distinct shades once ambient occlusion is folded in. Splitting that into material
 * sections would mean hundreds of draw calls for what is properly a single one.
 */
struct FBrickMesh
{
	TArray<FVector>           Vertices;
	TArray<int32>             Triangles;
	TArray<FVector>           Normals;
	TArray<FVector2D>         UVs;
	TArray<FColor>            Colors;
	TArray<FProcMeshTangent>  Tangents;

	/** Colour applied to every vertex emitted from here on. */
	FColor Ink = FColor::White;

	/**
	 * When set, every emitted vertex is tinted by DeepCoreRock::Strata at its world position.
	 *
	 * Applied in Vert() rather than at each call site so that it cannot be forgotten on one
	 * surface and applied on another -- a single untinted wall in a banded cavern reads as a
	 * different material and is instantly obvious.
	 */
	bool bStrata = false;

	void SetInk(const FLinearColor& C) { Ink = C.ToFColor(false); }

	void Reset()
	{
		Vertices.Reset();  Triangles.Reset(); Normals.Reset();
		UVs.Reset();       Colors.Reset();    Tangents.Reset();
	}

	void Reserve(int32 VertexCount)
	{
		Vertices.Reserve(VertexCount);  Normals.Reserve(VertexCount);
		UVs.Reserve(VertexCount);       Colors.Reserve(VertexCount);
		Tangents.Reserve(VertexCount);  Triangles.Reserve(VertexCount * 3 / 2);
	}

	int32 Num() const { return Vertices.Num(); }

	/** One vertex. Returns its index so callers can wire triangles by hand. */
	int32 Vert(const FVector& P, const FVector& N, const FVector2D& UV);

	/** A flat quad, wound counter-clockwise when seen from the side the normal points at. */
	void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
	          const FVector& N);

	/**
	 * An axis-aligned box. Origin is the centre of the footprint at the BOTTOM face, because
	 * bricks stack upward from a floor and that makes placement arithmetic trivial.
	 */
	void Box(const FVector& Centre, float HalfX, float Height, float HalfZ);

	/**
	 * A stud: a short cylinder with a cap. Twelve segments is the sweet spot -- eight reads as
	 * a polygon at close range, and twenty-four costs geometry nobody can see.
	 */
	void Stud(const FVector& Centre, float Scale = 1.0f);

	/**
	 * A brick TilesX by TilesY MAP TILES across and Height tall, studded on top.
	 *
	 * The footprint is given in tiles rather than studs because callers think in map tiles;
	 * the stud grid inside it is filled in automatically at StudsPerTile resolution.
	 */
	void Studded(const FVector& Centre, int32 TilesX, int32 TilesY, float Height,
	             bool bTopStuds = true, float StudScale = 1.0f);

	/**
	 * A stack of brick courses, studded only on the topmost.
	 *
	 * A wall emitted as one tall box has no seams and reads as extruded terrain -- the single
	 * clearest tell of a prototype. Real courses are visible because consecutive bricks do not
	 * line up perfectly, so alternate courses are inset very slightly here: enough for the
	 * light to catch a line, not enough to look like damage. Interior studs are skipped
	 * because they are buried inside the course above and would be pure cost.
	 */
	void Courses(const FVector& Base, int32 TilesX, int32 TilesY, int32 CourseCount,
	             float CourseHeight, const FLinearColor& Colour, float Shade = 0.94f);

	/**
	 * A part for character assembly: sized in world units rather than tiles, centred on its
	 * own middle rather than its base, because limbs rotate about a pivot inside them.
	 */
	void Part(const FVector& Centre, float HalfX, float HalfY, float HalfZ,
	          bool bStud = false, float StudScale = 0.55f);

	/**
	 * A rounded part, for helmets and crystal caps. Three stacked boxes of decreasing width,
	 * which is far cheaper than a sphere and reads better against the hard-edged bricks around
	 * it than a smooth sphere would -- the faceting is the point.
	 */
	void Domed(const FVector& Centre, float Radius, float Height);

	/**
	 * A quad subdivided Subdiv x Subdiv and displaced into rock.
	 *
	 * Corners are given in world space and wound so the normal points out of the solid. Every
	 * sample is displaced by DeepCoreRock::Displace, which depends only on world position, so
	 * adjacent quads -- in this tile or the next one -- agree exactly along their shared edge
	 * and no crack can open. Normals are recomputed per sub-quad from the DISPLACED corners:
	 * faceted, because rock breaks in flat conchoidal faces and smoothing them reads as clay.
	 */
	void RockQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
	              int32 Subdiv);

	/** Push this soup into a component as one section. */
	void Commit(UProceduralMeshComponent* Comp, int32 Section, bool bCollision) const;
};
