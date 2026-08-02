// DeepCoreTerrain.h : the cavern, built from bricks into a procedural mesh.
//
// The map itself is Sandbox::Level -- the same generator the headless sandbox and the OpenGL
// build use, compiled here unchanged. That is deliberate: the 40-seed sandbox suite is a real
// test of this world's structure only for as long as it is literally the same code. A ported
// copy would drift, and the suite would quietly stop meaning anything.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeepCoreBrick.h"
#include "SyntheticLevel.hpp"
#include "DeepCoreTerrain.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ADeepCoreTerrain : public AActor
{
	GENERATED_BODY()

public:
	ADeepCoreTerrain();

	/** Carve a fresh cavern and build its geometry. */
	void Generate(uint32 Seed);

	/** Remove the wall at a tile, revealing what is behind it, and rebuild. */
	bool Drill(int32 X, int32 Y);

	/** Reveal a radius, as opening a chamber would. */
	void Discover(int32 X, int32 Y, int32 Radius);

	/** Rebuild the mesh. Called on any change to the map; not called per frame. */
	void Rebuild();

	const Sandbox::Level& GetLevel() const { return Level; }

	/** True when a unit may stand here. */
	bool IsWalkable(int32 X, int32 Y) const;

	// TileSize, NOT StudPitch. These were the same number back when a tile was a single 1x1
	// brick, and stayed compiling when they stopped being -- placing tiles 25cm apart while
	// still drawing them 100cm wide, so every tile overlapped its neighbours fourfold and the
	// whole map collapsed to a quarter of its size.

	/** Tile centre in world space. */
	static FVector TileToWorld(int32 X, int32 Y, float Z = 0.0f)
	{
		return FVector(X * DeepCoreBrick::TileSize, Y * DeepCoreBrick::TileSize, Z);
	}

	/** Nearest tile to a world position. */
	static void WorldToTile(const FVector& P, int32& OutX, int32& OutY)
	{
		OutX = FMath::RoundToInt(P.X / DeepCoreBrick::TileSize);
		OutY = FMath::RoundToInt(P.Y / DeepCoreBrick::TileSize);
	}

	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;

	/**
	 * The back (roof) over every open chamber. Never drawn; lights the mine anyway.
	 *
	 * A real heading is a CLOSED volume, and that is most of why it looks like one: light that
	 * leaves a lamp comes back off the roof. Here the chambers were open to a void, so every
	 * photon heading upward left the scene forever -- which is exactly why the floors clipped
	 * white directly under a lamp and the walls a metre away were black. There was nothing to
	 * return any light to them.
	 *
	 * Drawing the roof would be useless, since the camera looks down through it. So the
	 * component is hidden in game but keeps bAffectDynamicIndirectLighting and
	 * bAffectIndirectLightingWhileHidden set, which is precisely the case
	 * PrimitiveSceneProxy.cpp:1708-1712 tests for when deciding whether a primitive can be
	 * traced: (IsVisibleInRayTracing() && (IsDrawnInGame() || AffectsIndirectLightingWhileHidden())).
	 * So Lumen bounces off a surface the player never sees.
	 */
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> RoofMesh;

private:
	/**
	 * How enclosed a tile is, 0 (open) to 1 (boxed in).
	 *
	 * Lumen supplies real bounce light, but it cannot know that a tile is a corner of a
	 * corridor before it has traced it. Baking cheap eight-neighbour occlusion into vertex
	 * colour costs nothing, is stable under camera motion, and gives the brickwork the
	 * contact darkening that makes stacked geometry read as solid rather than as decals.
	 */
	float TileAO(int32 X, int32 Y) const;

	Sandbox::Level Level;
};
