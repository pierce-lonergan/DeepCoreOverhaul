#include "DeepCoreTerrain.h"

#include "DeepCoreMaterials.h"
#include "DeepCoreRock.h"
#include "ProceduralMeshComponent.h"

using namespace Sandbox;
using namespace DeepCoreBrick;

ADeepCoreTerrain::ADeepCoreTerrain()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain"));
	RootComponent = Mesh;

	// The cavern is static geometry that changes only when a wall is drilled, so it wants to
	// cast and receive real shadows and contribute to Lumen's scene. Movable rather than
	// static because the mesh is rebuilt at runtime.
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->bUseAsyncCooking = true;
	Mesh->SetCastShadow(true);
}

float ADeepCoreTerrain::TileAO(int32 X, int32 Y) const
{
	int32 Occluders = 0;
	for (int32 Dy = -1; Dy <= 1; Dy++)
	{
		for (int32 Dx = -1; Dx <= 1; Dx++)
		{
			if (Dx == 0 && Dy == 0)
			{
				continue;
			}
			const int32 Nx = X + Dx, Ny = Y + Dy;
			// Off-map counts as solid: the world is a cavern, not an island, and treating the
			// border as open would light the map edge like a cliff face.
			if (!Level.InBounds(Nx, Ny))
			{
				Occluders++;
				continue;
			}
			const Block& B = Level.At(Nx, Ny);
			if (!B.Has(BLOCK_FLOOR) && !B.Has(BLOCK_WATER))
			{
				Occluders++;
			}
		}
	}
	// Never fully black. A tile boxed in on all eight sides still reads at 55%, because a
	// pure-black corner in a stylised scene looks like a hole in the geometry.
	return 1.0f - 0.45f * ((float)Occluders / 8.0f);
}

void ADeepCoreTerrain::Generate(uint32 Seed)
{
	LevelDesc Desc;
	Desc.width         = 48;
	Desc.height        = 40;
	Desc.seed          = Seed ? Seed : 1u;
	Desc.caverns       = 7;
	Desc.waterFraction = 0.05f;
	Desc.oreSeams      = 16;
	Desc.crystalSeams  = 12;

	Level.Generate(Desc);
	Level.RecomputeWalls();
	Rebuild();
}

bool ADeepCoreTerrain::IsWalkable(int32 X, int32 Y) const
{
	if (!Level.InBounds(X, Y))
	{
		return false;
	}
	const Block& B = Level.At(X, Y);
	return B.Has(BLOCK_FLOOR) && !B.Has(BLOCK_WATER) && !B.Has(BLOCK_HIDDEN);
}

void ADeepCoreTerrain::Discover(int32 X, int32 Y, int32 Radius)
{
	Level.Discover(X, Y, Radius);
	Level.RecomputeWalls();
	Rebuild();
}

bool ADeepCoreTerrain::Drill(int32 X, int32 Y)
{
	if (!Level.InBounds(X, Y))
	{
		return false;
	}
	Block& B = Level.At(X, Y);
	if (B.Has(BLOCK_FLOOR))
	{
		return false;
	}

	B.Clear(BLOCK_WALL | BLOCK_HIDDEN | BLOCK_ORE_SEAM | BLOCK_CRYSTAL_SEAM);
	B.Set(BLOCK_FLOOR);
	Level.Discover(X, Y, 2);
	Level.RecomputeWalls();
	Rebuild();
	return true;
}

namespace
{
	/** Top of the rock mass, and the base it is cut from. A wall stands ~2.4m over the floor. */
	constexpr float RockTop  = 240.0f;
	constexpr float RockBase = -40.0f;

	/** Subdivision of a one-tile face. 4 gives 25cm rock detail, which is about the finest
	 *  that survives being viewed from strategy distance. */
	constexpr int32 FaceSubdiv  = 4;
	constexpr int32 FloorSubdiv = 3;
}

void ADeepCoreTerrain::Rebuild()
{
	const FDeepCorePalette& Palette = GetDeepCorePalette();

	FBrickMesh Rock;   // the mass: walls, floors, everything structural
	FBrickMesh Vein;   // mineral bands, smoother, so a lamp flares off them

	Rock.bStrata = true;   // banding applied to every vertex; see FBrickMesh::bStrata
	Vein.bStrata = false;  // a vein cuts ACROSS bedding, so it must not be banded by it

	Rock.Reserve(Level.Width() * Level.Height() * 160);

	const float H = TileSize * 0.5f;

	// A tile only contributes geometry where it MEETS OPEN SPACE. Interior faces between two
	// solid tiles are invisible forever, and emitting them was most of the old vertex budget --
	// this is what pays for subdividing and displacing the faces that can actually be seen.
	auto IsSolid = [&](int32 X, int32 Y) -> bool
	{
		if (!Level.InBounds(X, Y))
		{
			return true;   // off-map reads as solid, so the world is sealed at its border
		}
		const Block& B = Level.At(X, Y);
		return !B.Has(BLOCK_FLOOR) && !B.Has(BLOCK_WATER);
	};

	for (int32 Y = 0; Y < Level.Height(); Y++)
	{
		for (int32 X = 0; X < Level.Width(); X++)
		{
			const Block& B = Level.At(X, Y);
			const FVector C = TileToWorld(X, Y);
			const float X0 = C.X - H, X1 = C.X + H;
			const float Y0 = C.Y - H, Y1 = C.Y + H;

			if (IsSolid(X, Y))
			{
				const bool bSeam = B.Has(BLOCK_CRYSTAL_SEAM) || B.Has(BLOCK_ORE_SEAM);
				FBrickMesh& M = bSeam ? Vein : Rock;

				if (B.Has(BLOCK_CRYSTAL_SEAM))
				{
					M.SetInk(FLinearColor(0.34f, 0.40f, 0.33f));   // pegmatite
				}
				else if (B.Has(BLOCK_ORE_SEAM))
				{
					M.SetInk(FLinearColor(0.19f, 0.13f, 0.07f));   // massive sulphide
				}
				else if (B.Has(BLOCK_HIDDEN))
				{
					M.SetInk(FLinearColor(0.042f, 0.040f, 0.038f)); // country rock, undisturbed
				}
				else
				{
					M.SetInk(FLinearColor(0.105f, 0.100f, 0.094f)); // granodiorite
				}

				// Back (roof of the rock mass). Seen constantly from a top-down camera, so it
				// gets the same treatment as the walls rather than being a flat lid.
				M.RockQuad(FVector(X0, Y0, RockTop), FVector(X1, Y0, RockTop),
				           FVector(X1, Y1, RockTop), FVector(X0, Y1, RockTop), FaceSubdiv);

				// Exposed faces only.
				if (!IsSolid(X, Y - 1))
				{
					M.RockQuad(FVector(X0, Y0, RockBase), FVector(X1, Y0, RockBase),
					           FVector(X1, Y0, RockTop),  FVector(X0, Y0, RockTop), FaceSubdiv);
				}
				if (!IsSolid(X, Y + 1))
				{
					M.RockQuad(FVector(X1, Y1, RockBase), FVector(X0, Y1, RockBase),
					           FVector(X0, Y1, RockTop),  FVector(X1, Y1, RockTop), FaceSubdiv);
				}
				if (!IsSolid(X - 1, Y))
				{
					M.RockQuad(FVector(X0, Y1, RockBase), FVector(X0, Y0, RockBase),
					           FVector(X0, Y0, RockTop),  FVector(X0, Y1, RockTop), FaceSubdiv);
				}
				if (!IsSolid(X + 1, Y))
				{
					M.RockQuad(FVector(X1, Y0, RockBase), FVector(X1, Y1, RockBase),
					           FVector(X1, Y1, RockTop),  FVector(X1, Y0, RockTop), FaceSubdiv);
				}
				continue;
			}

			// --- open ground ------------------------------------------------------------
			if (B.Has(BLOCK_WATER))
			{
				// Standing water: very dark and very smooth, so it is read almost entirely by
				// what it reflects. That is what water actually looks like underground.
				Vein.SetInk(FLinearColor(0.014f, 0.020f, 0.024f));
				Vein.RockQuad(FVector(X0, Y0, -14.0f), FVector(X1, Y0, -14.0f),
				              FVector(X1, Y1, -14.0f), FVector(X0, Y1, -14.0f), 1);
				continue;
			}

			if (B.Has(BLOCK_TOOLSTORE))
			{
				Rock.SetInk(FLinearColor(0.086f, 0.082f, 0.077f));
				Rock.RockQuad(FVector(X0, Y0, 0.0f), FVector(X1, Y0, 0.0f),
				              FVector(X1, Y1, 0.0f), FVector(X0, Y1, 0.0f), FloorSubdiv);
				// High-visibility equipment is the ONLY saturated colour permitted underground,
				// which is exactly why it reads as equipment and not as scenery.
				Rock.bStrata = false;
				Rock.SetInk(FLinearColor(0.42f, 0.26f, 0.02f));
				Rock.Box(FVector(C.X, C.Y, 0.0f), H * 0.72f, 90.0f, H * 0.72f);
				Rock.bStrata = true;
				continue;
			}

			// Muck-covered floor.
			Rock.SetInk(FLinearColor(0.092f, 0.088f, 0.081f));
			Rock.RockQuad(FVector(X0, Y0, 0.0f), FVector(X1, Y0, 0.0f),
			              FVector(X1, Y1, 0.0f), FVector(X0, Y1, 0.0f), FloorSubdiv);
		}
	}

	Rock.Commit(Mesh, 0, true);
	Vein.Commit(Mesh, 1, false);

	if (Palette.Surface) { Mesh->SetMaterial(0, Palette.Surface); }
	if (Palette.Glow)    { Mesh->SetMaterial(1, Palette.Glow); }

	UE_LOG(LogTemp, Display, TEXT("DeepCore: terrain rebuilt, %d rock verts, %d vein verts"),
	       Rock.Num(), Vein.Num());
}
