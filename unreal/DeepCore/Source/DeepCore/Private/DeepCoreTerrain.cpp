#include "DeepCoreTerrain.h"

#include "DeepCoreMaterials.h"
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

void ADeepCoreTerrain::Rebuild()
{
	const FDeepCorePalette& Palette = GetDeepCorePalette();

	FBrickMesh Surface;
	FBrickMesh Glow;

	// Studs multiply vertex count by roughly eight over a plain box world, so reserving up
	// front matters: without it this reallocates dozens of times per rebuild, and a rebuild
	// happens on every drill.
	Surface.Reserve(Level.Width() * Level.Height() * 200);

	for (int32 Y = 0; Y < Level.Height(); Y++)
	{
		for (int32 X = 0; X < Level.Width(); X++)
		{
			const Block& B = Level.At(X, Y);
			const float AO = TileAO(X, Y);
			const bool bSolid = !B.Has(BLOCK_FLOOR) && !B.Has(BLOCK_WATER);
			const FVector C = TileToWorld(X, Y);

			// Everything is measured from the floor plate, which sits just under ground level
			// so that units placed at Z=0 stand ON it.
			const float PlateBase = FloorTop - PlateHeight;
			// Eight courses puts a wall at 240cm -- well over head height on a 147cm crew
			// member. Five courses read as a kerb from a strategy camera: the eye needs the
			// walls to dominate before a corridor reads as a corridor rather than as a colour
			// change in the floor.
			const int32 WallCourses = 8;

			// Undiscovered rock is deliberately UNSTUDDED and emitted as a single block, so
			// unexplored ground reads as raw stone rather than as something already built.
			// The visual grammar carries information: studs mean "known", bare means "not
			// yet". It is also most of the map, so it is the cheap case by design.
			if (B.Has(BLOCK_HIDDEN) && bSolid)
			{
				Surface.SetInk(FLinearColor(0.15f, 0.135f, 0.12f) * AO);
				Surface.Studded(C + FVector(0, 0, PlateBase), 1, 1,
				                PlateHeight + WallCourses * BrickHeight, false);
				continue;
			}

			if (B.Has(BLOCK_CRYSTAL_SEAM))
			{
				Surface.Courses(C + FVector(0, 0, PlateBase), 1, 1, WallCourses - 1,
				                BrickHeight, FLinearColor(0.30f, 0.21f, 0.36f) * AO);
				Glow.SetInk(FLinearColor(0.80f, 0.38f, 1.00f));
				Glow.Domed(C + FVector(0, 0, PlateBase + (WallCourses - 1) * BrickHeight),
				           26.0f, 42.0f);
				continue;
			}

			if (B.Has(BLOCK_ORE_SEAM))
			{
				Surface.Courses(C + FVector(0, 0, PlateBase), 1, 1, WallCourses - 1,
				                BrickHeight, FLinearColor(0.36f, 0.26f, 0.16f) * AO);
				Surface.SetInk(FLinearColor(0.66f, 0.47f, 0.24f) * AO);
				Surface.Studded(C + FVector(0, 0, PlateBase + (WallCourses - 1) * BrickHeight),
				                1, 1, PlateHeight, true);
				continue;
			}

			if (bSolid)
			{
				// An exposed wall is a stack of real courses. One tall box the same size has
				// no seams and reads as extruded terrain -- the single clearest tell of a
				// prototype.
				Surface.Courses(C + FVector(0, 0, PlateBase), 1, 1, WallCourses,
				                BrickHeight, FLinearColor(0.40f, 0.34f, 0.29f) * AO);
				continue;
			}

			if (B.Has(BLOCK_WATER))
			{
				Surface.SetInk(FLinearColor(0.10f, 0.36f, 0.62f) * AO);
				Surface.Box(C + FVector(0, 0, PlateBase), TileSize * 0.5f, 6.0f, TileSize * 0.5f);
				continue;
			}

			if (B.Has(BLOCK_TOOLSTORE))
			{
				Surface.SetInk(FLinearColor(0.30f, 0.28f, 0.26f) * AO);
				Surface.Studded(C + FVector(0, 0, PlateBase), 1, 1, PlateHeight, false);
				Surface.Courses(C + FVector(0, 0, FloorTop), 1, 1, 2, BrickHeight,
				                FLinearColor(0.90f, 0.68f, 0.16f));
				Surface.SetInk(FLinearColor(0.32f, 0.34f, 0.40f));
				Surface.Box(C + FVector(0, 0, FloorTop + 2 * BrickHeight),
				            TileSize * 0.42f, 10.0f, TileSize * 0.42f);
				continue;
			}

			// Floor: a studded plate. This is what makes open ground read as a build surface
			// and gives every chamber a repeating highlight to catch the key light.
			Surface.SetInk(FLinearColor(0.30f, 0.285f, 0.265f) * AO);
			Surface.Studded(C + FVector(0, 0, PlateBase), 1, 1, PlateHeight, true);
		}
	}

	Surface.Commit(Mesh, 0, true);
	Glow.Commit(Mesh, 1, false);

	if (Palette.Surface) { Mesh->SetMaterial(0, Palette.Surface); }
	if (Palette.Glow)    { Mesh->SetMaterial(1, Palette.Glow); }

	UE_LOG(LogTemp, Display, TEXT("DeepCore: terrain rebuilt, %d verts (%d glow)"),
	       Surface.Num(), Glow.Num());
}
