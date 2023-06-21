// Fallin.cpp : 
//

#include "../../engine/core/Maths.h"

#include "../audio/SFX.h"
#include "../effects/Effects.h"
#include "../interface/InfoMessages.h"
#include "../Game.h"
#include "Fallin.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004a2ee4>
LegoRR::Fallin_Globs & LegoRR::fallinGlobs = *(LegoRR::Fallin_Globs*)0x004a2ee4;

// <LegoRR.exe @004d88c4>
real32 & LegoRR::s_fallinUpdateTimer = *(real32*)0x004d88c4;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0040f010>
void __cdecl LegoRR::Fallin_Update(real32 elapsedWorld)
{
	s_fallinUpdateTimer -= elapsedWorld;
	if (s_fallinUpdateTimer <= 0.0f) {
		Map3D* map = Lego_GetMap();

		for (uint32 i = 0; i < FALLIN_MAXTRIES; i++) {
			const Point2I blockPos = {
				(sint32)((uint32)Gods98::Maths_Rand() % map->blockWidth),
				(sint32)((uint32)Gods98::Maths_Rand() % map->blockHeight),
			};

			if (Fallin_TryGenerateLandSlide(&blockPos, true)) {
				Info_Send(Info_Landslide, nullptr, nullptr, &blockPos);
				break;
			}
		}

		// Set the wait time until next fallin.
		const uint32 extraSeconds = (uint32)Gods98::Maths_Rand() % (STANDARD_FRAMERATEI * 2); // 0 to 2 seconds.
		s_fallinUpdateTimer = (real32)extraSeconds + (STANDARD_FRAMERATE * 30.0f); // 30 + extra seconds.
	}
}

// <LegoRR.exe @0040f0c0>
bool32 __cdecl LegoRR::Fallin_TryGenerateLandSlide(const Point2I* blockPos, bool32 allowCaveIn)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	if (Fallin_CanLandSlideAtBlock(blockPos)) {
		bool notSolidOrReinforced = true;
		DirectionFlags fallinDirs = DIRECTION_FLAG_NONE;
		uint32 flatWallCount = 0; // Number of non-corner walls.

		for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
			const Point2I blockOffPos = {
				blockPos->x + DIRECTIONS[i].x,
				blockPos->y + DIRECTIONS[i].y,
			};

			if (Level_Block_IsWall(blockOffPos.x, blockOffPos.y)) {

				if (Level_Block_IsImmovable(&blockOffPos) ||
					Level_Block_IsReinforced(blockOffPos.x, blockOffPos.y))
				{
					notSolidOrReinforced = false;
				}

				fallinDirs |= DirectionToFlag(i); // (1 << i);

				if (!Level_Block_IsCorner(blockOffPos.x, blockOffPos.y)) {
					flatWallCount++;
				}
			}
		}
		if (notSolidOrReinforced && fallinDirs != DIRECTION_FLAG_NONE && flatWallCount > 0) {
			Fallin_GenerateLandSlide(blockPos, fallinDirs, allowCaveIn);
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040f1e0>
bool32 __cdecl LegoRR::Fallin_CanLandSlideAtBlock(const Point2I* blockPos)
{
	Lego_Level* level = Lego_GetLevel();
	if (Level_Block_IsGround(blockPos->x, blockPos->y) && !Level_Block_IsBusy(blockPos)) {

		const uint8 terrain = blockValue(level, blockPos->x, blockPos->y).terrain;
		if (terrain != Lego_SurfaceType_Lava &&
			terrain != Lego_SurfaceType_Lake &&
			terrain != Lego_SurfaceType_Water)
		{
			if (level->SafeCaverns == BOOL3_FALSE || !Level_Block_IsExposed(blockPos)) {
				return true;
			}
		}
	}
	return false;
}

// <LegoRR.exe @0040f260>
void __cdecl LegoRR::Fallin_GenerateLandSlide(const Point2I* blockPos, DirectionFlags fallinDirs, bool32 allowCaveIn)
{
	const Point2F EFFECT_DIRECTIONS[DIRECTION__COUNT] = {
		{  0.0f,  1.0f },
		{  1.0f,  0.0f },
		{  0.0f, -1.0f },
		{ -1.0f,  0.0f },
	};

	// Remove all existing rubble layers so we can overwrite them.
	// This is probably what causes Ore spawns to be removed from rubble.
	bool32 rubbleCleared = false; // dummy init
	do {
		rubbleCleared = Level_Block_ClearRubbleLayer(blockPos);
	} while (!rubbleCleared);

	AITask_DoClearTypeAction(blockPos, Message_ClearFallInComplete);

	/// TODO: What is ROCKFALL_VTUNNEL used for? Should it be used for NorthSouth/EastWest??
	Direction direction       = DIRECTION_UP; // dummy init
	RockFallType rockFallType = ROCKFALL_3SIDES; // dummy init
	switch (fallinDirs) {
	case DIRECTION_FLAG_N:
	case DIRECTION_FLAG_SEW:
		direction    = DIRECTION_UP;
		rockFallType = ROCKFALL_3SIDES;
		break;
	case DIRECTION_FLAG_E:
	case DIRECTION_FLAG_EW:
	case DIRECTION_FLAG_NSW:
		direction    = DIRECTION_RIGHT;
		rockFallType = ROCKFALL_3SIDES;
		break;
	case DIRECTION_FLAG_S:
	case DIRECTION_FLAG_NS:
	case DIRECTION_FLAG_NEW:
		direction    = DIRECTION_DOWN;
		rockFallType = ROCKFALL_3SIDES;
		break;
	case DIRECTION_FLAG_NSE:
	case DIRECTION_FLAG_W:
		direction    = DIRECTION_LEFT;
		rockFallType = ROCKFALL_3SIDES;
		break;

	case DIRECTION_FLAG_NW:
		direction    = DIRECTION_UP;
		rockFallType = ROCKFALL_OUTSIDECORNER;
		break;
	case DIRECTION_FLAG_NE:
		direction    = DIRECTION_RIGHT;
		rockFallType = ROCKFALL_OUTSIDECORNER;
		break;
	case DIRECTION_FLAG_SE:
		direction    = DIRECTION_DOWN;
		rockFallType = ROCKFALL_OUTSIDECORNER;
		break;
	case DIRECTION_FLAG_SW:
		direction    = DIRECTION_LEFT;
		rockFallType = ROCKFALL_OUTSIDECORNER;
		break;
	}

	Vector3F wPos = { 0.0f, 0.0f, 0.0f }; // dummy init
	Map3D_BlockToWorldPos(Lego_GetMap(), blockPos->x, blockPos->y, &wPos.x, &wPos.y);
	wPos.z = Map3D_GetWorldZ(Lego_GetMap(), wPos.x, wPos.y);

	if (Effect_Spawn_RockFall(rockFallType, blockPos->x, blockPos->y, wPos.x, wPos.y, wPos.z,
							  EFFECT_DIRECTIONS[direction].x, EFFECT_DIRECTIONS[direction].y))
	{
		Vector3F sfxPos = { wPos.x, wPos.y, 0.0f };
		sfxPos.z = Map3D_GetWorldZ(Lego_GetMap(), sfxPos.x, sfxPos.y);

		blockValue(Lego_GetLevel(), blockPos->x, blockPos->y).flags1 |= (BLOCK1_ROCKFALLFX | BLOCK1_LANDSLIDING);
		Level_Block_SetBusy(blockPos, true);

		SFX_Random_PlaySound3DOnContainer(nullptr, SFX_FallIn, false, false, &sfxPos);
	}

	if (allowCaveIn) {
		Lego_Block* block = &blockValue(Lego_GetLevel(), blockPos->x, blockPos->y);

		/// FIXME: Is numLandSlides supposed to reset to zero after a cave-in???
		///        Or maybe it could be used as a modulus numLandSlidesTillCaveIn?
		block->numLandSlides++;
		//if ((block->numLandSlides % fallinGlobs.numLandSlidesTillCaveIn) == 0) {
		if (block->numLandSlides >= fallinGlobs.numLandSlidesTillCaveIn) {

			Fallin_GenerateCaveIn(blockPos, fallinDirs);
			Info_Send(Info_Landslide, nullptr, nullptr, blockPos);
		}
	}

	Level_BlockUpdateSurface(legoGlobs.currLevel, blockPos->x, blockPos->y, 0);
}

// <LegoRR.exe @0040f510>
void __cdecl LegoRR::Fallin_Initialise(uint32 numLandSlidesTillCaveIn)
{
	fallinGlobs.numLandSlidesTillCaveIn = numLandSlidesTillCaveIn;

	/// FIXME: Reset update timer between levels.
	///        Should we actually do this? This will nearly always force a landslide on the first tick of a level...
	//s_fallinUpdateTimer = 0.0f;
}

// <LegoRR.exe @0040f520>
void __cdecl LegoRR::Fallin_GenerateCaveIn(const Point2I* blockPos, DirectionFlags fallinDirs)
{
	const Point2I ANGLES[8] = {
		{  0,  1 },
		{  1,  1 },
		{  1,  0 },
		{  1, -1 },
		{  0, -1 },
		{ -1, -1 },
		{ -1,  0 },
		{ -1,  1 },
	};

	for (uint32 i = 0; i < _countof(ANGLES); i++) {
		const Point2I blockOffPos = {
			blockPos->x + ANGLES[i].x,
			blockPos->y + ANGLES[i].y,
		};

		if (Map3D_IsInsideDimensions(Lego_GetMap(), blockOffPos.x, blockOffPos.y)) {
			/// TODO: Why is TryGenerateLandSlide and GenerateLandSlide both being used here!???
			if (!Fallin_TryGenerateLandSlide(&blockOffPos, false)) {
				if (Fallin_CanLandSlideAtBlock(&blockOffPos)) {
					Fallin_GenerateLandSlide(&blockOffPos, fallinDirs, false);
				}
			}
		}
	}
}

#pragma endregion
