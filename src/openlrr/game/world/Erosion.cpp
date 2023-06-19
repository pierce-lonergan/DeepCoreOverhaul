// Erosion.cpp : 
//

#include "../../engine/core/Maths.h"

#include "../effects/Effects.h"
#include "../interface/InfoMessages.h"
#include "../world/Construction.h"
#include "../Game.h"

#include "Erosion.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004c8eb0>
LegoRR::Erosion_Globs & LegoRR::erosionGlobs = *(LegoRR::Erosion_Globs*)0x004c8eb0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0040e860>
void __cdecl LegoRR::Erosion_Initialise(real32 triggerTime, real32 erodeTime, real32 lockTime)
{
	/// SANITY: Memzero everything.
	std::memset(&erosionGlobs, 0, sizeof(Erosion_Globs));

	erosionGlobs.triggerTime = triggerTime * STANDARD_FRAMERATE; // Seconds to standard units.
	erosionGlobs.erodeTime = erodeTime * STANDARD_FRAMERATE / 5.0f; // Seconds to standard units / 5.
	erosionGlobs.lockTime = lockTime * STANDARD_FRAMERATE; // Seconds to standard units.
	/// FIX APPLY: Reset timer for triggering new active erosions.
	erosionGlobs.updateTime = 0.0f;
	erosionGlobs.lavaBlockCount = 0;

	for (uint32 i = 0; i < EROSION_MAXERODEBLOCKS; i++) {
		erosionGlobs.activeUsed[i] = false;
	}
	for (uint32 i = 0; i < EROSION_MAXLOCKEDBLOCKS; i++) {
		erosionGlobs.lockedUsed[i] = false;
	}
}

// <LegoRR.exe @0040e8c0>
bool32 __cdecl LegoRR::Erosion_GetFreeActiveIndex(OUT uint32* index)
{
	for (uint32 i = 0; i < EROSION_MAXERODEBLOCKS; i++) {
		if (!erosionGlobs.activeUsed[i]) {
			*index = i;
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040e8f0>
real32 __cdecl LegoRR::Erosion_GetBlockErodeRate(const Point2I* blockPos)
{
	const uint32 erodeSpeed = (uint32)blockValue(Lego_GetLevel(), blockPos->x, blockPos->y).erodeSpeed;

	/// TODO: Why are we multiplying by standard framerate A SECOND TIME!?
	///       This is probably because there was a lot of tweaking for erode times,
	///        and it ended up getting tested via code changes.
	return (real32)(6 - erodeSpeed) * erosionGlobs.erodeTime * STANDARD_FRAMERATE;
}

// <LegoRR.exe @0040e940>
void __cdecl LegoRR::Erosion_AddActiveBlock(const Point2I* blockPos, sint32 erodeStage)
{
	uint32 erodeIndex;
	if (Erosion_GetFreeActiveIndex(&erodeIndex)) {
		blockValue(Lego_GetLevel(), blockPos->x, blockPos->y).erodeStage = (uint8)erodeStage;

		erosionGlobs.activeBlocks[erodeIndex] = *blockPos;
		// Set the timer to the beginning of the specified erode stage.
		erosionGlobs.activeTimers[erodeIndex] = (Erosion_GetBlockErodeRate(blockPos) / (real32)(erodeStage + 1));
		erosionGlobs.activeUsed[erodeIndex] = true;

		// Erosion lava blocks both mark actively eroding blocks, and source blocks for adjacent erosion.
		Erosion_AddOrRemoveLavaBlock(blockPos, true);
	}
}

// <LegoRR.exe @0040e9e0>
void __cdecl LegoRR::Erosion_Update(real32 elapsedWorld)
{
	Lego_Level* level = Lego_GetLevel();
	/// TODO: We can move this to after Erosion_GetFreeActiveIndex.
	///       However doing so could remove a bit of randomness from the game, since this is called every tick.
	const uint32 rng = (uint32)Gods98::Maths_Rand();

	// Try to trigger a new actively-eroding block.
	// Note that this logic only handles erosion that starts adjacent to lava blocks.
	// Source erosion is handled during erode map loading, by starting the block as active.
	erosionGlobs.updateTime += elapsedWorld;
	/// TODO: Consider changing this to a while loop and subtracting triggerTime.
	///       This way differences in elapsed time don't cause advantages or disadvantages
	///        (besides how they already do).
	if (erosionGlobs.updateTime > erosionGlobs.triggerTime) {
		erosionGlobs.updateTime = 0.0f; // Restart the timer.

		uint32 erodeIndex;
		if (Erosion_GetFreeActiveIndex(&erodeIndex)) {
			//const sint32 rng = (sint32)Gods98::Maths_Rand();

			for (uint32 i = 0; i < erosionGlobs.lavaBlockCount; i++) {
				Point2I blockPos;
				const uint32 lavaIndex = (rng + i) % erosionGlobs.lavaBlockCount;
				if (Erosion_FindAdjacentBlockPos(&erosionGlobs.lavaBlockList[lavaIndex], &blockPos)) {
					// It seems power paths double the duration that a block is safe.
					// Once it's hit once, it needs to be hit again to start eroding.
					// It's actually slightly more than double, since the block also gets locked,
					//  and that adds additional time to the delay due to Erosion_FindAdjacentBlockPos.
					if (!Level_Block_IsPath(&blockPos) ||
						(blockValue(level, blockPos.x, blockPos.y).flags2 & BLOCK2_ERODEPATHDELAY))
					{
						blockValue(level, blockPos.x, blockPos.y).flags2 &= ~BLOCK2_ERODEPATHDELAY;

						erosionGlobs.activeBlocks[erodeIndex] = blockPos;
						// Start the erosion stage timer at the beginning of the first stage.
						erosionGlobs.activeTimers[erodeIndex] = Erosion_GetBlockErodeRate(&blockPos);
						erosionGlobs.activeUsed[erodeIndex] = true;

						// Set to the very first stage of erosion.
						blockValue(level, blockPos.x, blockPos.y).erodeStage = 0;

						// Erosion lava blocks both mark actively eroding blocks, and source blocks for adjacent erosion.
						Erosion_AddOrRemoveLavaBlock(&blockPos, true);

						// Unlike erode stages 1-4, stage 0 does not produce a puff of smoke/fire effect.

						Info_Send(Info_LavaErode, nullptr, nullptr, &blockPos);
						break; // We've found a block for use with erodeIndex, stop here.
					}

					// So the path delay doesn't truly slow down erosion, it just makes other blocks erode sooner.
					Erosion_AddLockedBlock(&blockPos);
					blockValue(level, blockPos.x, blockPos.y).flags2 |= BLOCK2_ERODEPATHDELAY;
					// Keep going until we find a block or exhaust the list.
				}
			}
		}
	}

	// Update actively-eroding block timers.
	for (uint32 i = 0; i < EROSION_MAXERODEBLOCKS; i++) {
		if (erosionGlobs.activeUsed[i]) {
			const real32 erodeRate = Erosion_GetBlockErodeRate(&erosionGlobs.activeBlocks[i]);
			
			/// REFACTOR: Store last/nextErodeStage local variable as [4,1] to match erode stages,
			///            instead of [0,3] which doesn't mean anything.
			/// FIX APPLY: Ensure erode timer don't go negative and give a negative erodeStage.
			const uint8 lastErodeStage = 4 - (uint8)(sint32)((erosionGlobs.activeTimers[i] / erodeRate) * 4.0f);
			erosionGlobs.activeTimers[i] -= elapsedWorld;
			if (erosionGlobs.activeTimers[i] < 0.0f)
				erosionGlobs.activeTimers[i] = 0.0f;
			const uint8 nextErodeStage = 4 - (uint8)(sint32)((erosionGlobs.activeTimers[i] / erodeRate) * 4.0f);

			// Erode stage has changed, do updates.
			if (nextErodeStage != lastErodeStage) {
				const Point2I blockPos = erosionGlobs.activeBlocks[i];

				// Set the new erode stage.
				blockValue(level, blockPos.x, blockPos.y).erodeStage = nextErodeStage;

				// Spawn a puff of smoke/fire effect.
				Vector3F effectPos = { 0.0f, 0.0f, 0.0f }; // dummy init
				Map3D_BlockToWorldPos(Lego_GetMap(), blockPos.x, blockPos.y, &effectPos.x, &effectPos.y);
				effectPos.z = Map3D_GetWorldZ(Lego_GetMap(), effectPos.x, effectPos.y);
				
				/// FIX APPLY: Ensure we have a non-zero vector after removing the z component.
				///            In the future Maths_Vector2DRandom should be rewritten to handle
				///             this just like Maths_Vector3DRandom.
				Vector3F effectDir = { 0.0f, 0.0f, 0.0f }; // dummy init
				do {
					Gods98::Maths_Vector3DRandom(&effectDir);
					effectDir.z = 0.0f;
				} while (effectDir.x == 0.0f && effectDir.y == 0.0f);
				/// TODO: Should we be normalizing the direction after setting z to 0.0f?
				//Gods98::Maths_Vector3DNormalize(&effectDir);

				/// FIXME: Handle logic here for destroying construction sites, or building paths that have been overwritten.
				///        This should only need to be handled when uping to stage 1.

				// Spawn the effect and change the block to lava if we've reached stage 4.
				switch (nextErodeStage) {
				case 1:
					Effect_Spawn_Particle(MISCOBJECT_LAVAEROSIONSMOKE4, &effectPos, &effectDir);
					break;
				case 2:
					Effect_Spawn_Particle(MISCOBJECT_LAVAEROSIONSMOKE3, &effectPos, &effectDir);
					break;
				case 3:
					Effect_Spawn_Particle(MISCOBJECT_LAVAEROSIONSMOKE2, &effectPos, &effectDir);
					break;
				case 4:
					// Erosion complete, remove from the active list.
					erosionGlobs.activeUsed[i] = false;
					blockValue(level, blockPos.x, blockPos.y).terrain = Lego_SurfaceType_Lava;
					Effect_Spawn_Particle(MISCOBJECT_LAVAEROSIONSMOKE1, &effectPos, &effectDir);
					break;
				}
				// Update the block texture to match the new stage.
				Level_BlockUpdateSurface(level, blockPos.x, blockPos.y, 0);
			}
		}
	}

	// Update locked eroding block timers.
	for (uint32 i = 0; i < EROSION_MAXLOCKEDBLOCKS; i++) {
		if (erosionGlobs.lockedUsed[i]) {
			erosionGlobs.lockedTimers[i] -= elapsedWorld;
			if (erosionGlobs.lockedTimers[i] < 0.0f) {
				erosionGlobs.lockedUsed[i] = false;
			}
		}
	}
}

// <LegoRR.exe @0040ed30>
void __cdecl LegoRR::Erosion_AddLockedBlock(const Point2I* blockPos)
{
	for (uint32 i = 0; i < EROSION_MAXLOCKEDBLOCKS; i++) {
		if (!erosionGlobs.lockedUsed[i]) {
			erosionGlobs.lockedBlocks[i] = *blockPos;
			erosionGlobs.lockedTimers[i] = erosionGlobs.lockTime;
			erosionGlobs.lockedUsed[i] = true;
			break;
		}
	}
}

// <LegoRR.exe @0040ed80>
void __cdecl LegoRR::Erosion_AddOrRemoveLavaBlock(const Point2I* blockPos, bool32 add)
{
	Lego_Level* level = Lego_GetLevel();

	if (!add) {
		for (uint32 i = 0; i < erosionGlobs.lavaBlockCount; i++) {
			if (erosionGlobs.lavaBlockList[i].x == blockPos->x &&
				erosionGlobs.lavaBlockList[i].y == blockPos->y)
			{
				// Resize the list by moving the item at the last index to the index being removed.
				erosionGlobs.lavaBlockList[i] = erosionGlobs.lavaBlockList[erosionGlobs.lavaBlockCount - 1];
				erosionGlobs.lavaBlockCount--;

				blockValue(level, blockPos->x, blockPos->y).flags1 &= ~BLOCK1_ERODEACTIVE;
				break;
			}
		}

		for (uint32 i = 0; i < EROSION_MAXERODEBLOCKS; i++) {
			/// FIX APPLY: Check activeUsed instead of ignoring it.
			///            Otherwise we could remove an old entry and then break.
			if (erosionGlobs.activeUsed[i] &&
				erosionGlobs.activeBlocks[i].x == blockPos->x &&
				erosionGlobs.activeBlocks[i].y == blockPos->y)
			{
				erosionGlobs.activeUsed[i] = false;
				break;
			}
		}

		Erosion_AddLockedBlock(blockPos);
	}
	else if (erosionGlobs.lavaBlockCount < EROSION_MAXLAVABLOCKS) {
		if (!(blockValue(level, blockPos->x, blockPos->y).flags1 & BLOCK1_ERODEACTIVE)) {

			erosionGlobs.lavaBlockList[erosionGlobs.lavaBlockCount] = *blockPos;
			erosionGlobs.lavaBlockCount++;

			Level_Block_Proc_FUN_004301e0(blockPos);

			blockValue(level, blockPos->x, blockPos->y).flags1 |= BLOCK1_ERODEACTIVE;
		}
	}
}

// <LegoRR.exe @0040eee0>
bool32 __cdecl LegoRR::Erosion_IsBlockLocked(const Point2I* blockPos)
{
	for (uint32 i = 0; i < EROSION_MAXLOCKEDBLOCKS; i++) {
		if (erosionGlobs.lockedUsed[i] &&
			erosionGlobs.lockedBlocks[i].x == blockPos->x &&
			erosionGlobs.lockedBlocks[i].y == blockPos->y)
		{
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040ef30>
bool32 __cdecl LegoRR::Erosion_FindAdjacentBlockPos(const Point2I* blockPos, OUT Point2I* adjacentblockPos)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	Lego_Level* level = Lego_GetLevel();

	// The block needs to be adjacent to lava.
	if (blockValue(level, blockPos->x, blockPos->y).terrain != Lego_SurfaceType_Lava) {
		return false;
	}

	/// TODO: Consider using RNG to determine the first directions to check.
	//const uint32 rng = (uint32)Gods98::Maths_Rand();

	for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
		/// TODO: Consider using RNG to determine the first directions to check.
		//const uint32 dirIndex = (rng + i) % _countof(DIRECTIONS);
		const uint32 dirIndex = i;

		adjacentblockPos->x = blockPos->x + DIRECTIONS[dirIndex].x;
		adjacentblockPos->y = blockPos->y + DIRECTIONS[dirIndex].y;

		Lego_Block* block = &blockValue(level, adjacentblockPos->x, adjacentblockPos->y);

		if (!(block->flags1 & BLOCK1_ERODEACTIVE) && (block->flags1 & BLOCK1_FLOOR) &&
			block->erodeSpeed != 0 && !Erosion_IsBlockLocked(adjacentblockPos))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion
