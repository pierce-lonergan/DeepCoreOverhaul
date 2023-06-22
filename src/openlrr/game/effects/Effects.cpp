// Effects.cpp : 
//

#include "../../engine/core/Utils.h"

#include "../object/Object.h"
#include "../world/ElectricFence.h"
#include "../Game.h"
#include "Effects.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004c8180>
LegoRR::Effect_Globs & LegoRR::effectGlobs = *(LegoRR::Effect_Globs*)0x004c8180;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define MiscObjects_ID(...) Config_ID(gameName, "MiscObjects", __VA_ARGS__ )

#pragma endregion
/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0040bcf0>
void __cdecl LegoRR::Effect_StopAll(void)
{
	// Dummy output variables.
	RockFallType* rockFallTypes;
	uint32* rockFallIndexes;

	Effect_UpdateAll(10000.0f, &rockFallTypes, &rockFallIndexes);
}

// <LegoRR.exe @0040bd10>
void __cdecl LegoRR::Effect_Spawn_BoulderExplode_AtSimpleObject(LegoObject* simpleObj)
{
	/// REFACTOR: Support non-simple objects too, and null-check container.
	Gods98::Container* cont = LegoObject_GetActivityContainer(simpleObj);
	//Gods98::Container* cont = simpleObj->other;
	if (cont != nullptr) {
		Vector3F wPos;
		Gods98::Container_GetPosition(cont, nullptr, &wPos);
		Effect_Spawn_BoulderExplode(&wPos);
	}
}

// <LegoRR.exe @0040bd40>
void __cdecl LegoRR::Effect_Spawn_BoulderExplode(const Vector3F* wPos)
{
	for (uint32 i = 0; i < EFFECT_BOULDEREXPLODE_MAXCONTAINERS; i++) {
		if (effectGlobs.boulderExplodeContTable[i] == nullptr) {

			Gods98::Container* sourceCont;
			if (Lego_GetLevel()->BoulderAnimation == TEXTURES_ICE) {
				// Hardcoded Ice boulder explode effect.
				sourceCont = legoGlobs.contBoulderExplodeIce;
			}
			else {
				sourceCont = legoGlobs.contBoulderExplode;
			}

			effectGlobs.boulderExplodeContTable[i] = Gods98::Container_Clone(sourceCont);
			Gods98::Container_SetAnimationTime(effectGlobs.boulderExplodeContTable[i], 0.0f);
			Gods98::Container_SetPosition(effectGlobs.boulderExplodeContTable[i], nullptr, wPos->x, wPos->y, wPos->z);
			Gods98::Container_SetOrientation(effectGlobs.boulderExplodeContTable[i], nullptr, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
			break;
		}
	}
}

// <LegoRR.exe @0040bde0>
void __cdecl LegoRR::Effect_Spawn_SmashPath(OPTIONAL LegoObject* liveObj, OPTIONAL const Vector3F* wPos)
{
	Vector3F effectPos = { 0.0f, 0.0f, 0.0f }; // dummy init
	if (liveObj == nullptr) {
		effectPos = *wPos;
	}
	else {
		Gods98::Container* cont = LegoObject_GetActivityContainer(liveObj);
		Gods98::Container_GetPosition(cont, nullptr, &effectPos);
	}

	for (uint32 i = 0; i < EFFECT_SMASHPATH_MAXCONTAINERS; i++) {
		if (effectGlobs.smashPathContTable[i] == nullptr) {

			effectGlobs.smashPathContTable[i] = Gods98::Container_Clone(legoGlobs.contSmashPath);
			Gods98::Container_SetAnimationTime(effectGlobs.smashPathContTable[i], 0.0f);
			Gods98::Container_SetPosition(effectGlobs.smashPathContTable[i], nullptr, effectPos.x, effectPos.y, effectPos.z);
			Gods98::Container_SetOrientation(effectGlobs.smashPathContTable[i], nullptr, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
			/// FIX APPLY: Break after spawning a smash path effect!!
			break;
		}
	}
}

// <LegoRR.exe @0040bea0>
bool32 __cdecl LegoRR::Effect_FindRockFallStyle(const char* name, OUT uint32* index)
{
	for (uint32 i = 0; i < effectGlobs.rockFallStyleCount; i++) {
		if (::_stricmp(effectGlobs.rockFallStyleName[i], name) == 0) {
			*index = i;
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040bef0>
void __cdecl LegoRR::Effect_SetRockFallStyle(uint32 rockFallStyleIndex)
{
	effectGlobs.rockFallStyleIndex = rockFallStyleIndex;
}

// <LegoRR.exe @0040bf00>
void __cdecl LegoRR::Effect_Load_RockFallStylesAll(const Gods98::Config* config, const char* gameName, Gods98::Container* contRoot)
{
	const Gods98::Config* arrayFirst = Gods98::Config_FindArray(config, Config_ID(gameName, "RockFallStyles"));

	for (const Gods98::Config* item = arrayFirst; item != nullptr; item = Gods98::Config_GetNextItem(item)) {

		if (effectGlobs.rockFallStyleCount >= EFFECT_ROCKFALL_MAXSTYLES) {
			Config_WarnItem(true, item, "Too many entries in RockFallStyles, expected 4 or less.");
			break;
		}
		
		effectGlobs.rockFallStyleName[effectGlobs.rockFallStyleCount] = Gods98::Util_StrCpy(Gods98::Config_GetItemName(item));

		char buff[1024];
		std::strcpy(buff, Gods98::Config_GetDataString(item));

		// usage: <RockFallStyle>    <TumbleNullName>,<3SidesRockFallType>,<OutsideCornerRockFallType>,<VTunnelRockFallType>
		char* stringParts[(uint32)ROCKFALL_TYPE_COUNT + 1 + 1]; // +1 extra for overflow but still-accurate parsing.
		const uint32 argc = Gods98::Util_TokeniseSafe(buff, stringParts, ",", _countof(stringParts));
		if (argc >= 2) {
			const uint32 numRockFallTypes = (argc - 1);
			Config_WarnItem((numRockFallTypes > (uint32)ROCKFALL_TYPE_COUNT), item, "Too many RockFall types defined, expected 3 or less.");
			
			/// SANITY: Don't load more than the max supported types.
			for (uint32 i = 0; i < std::min((uint32)ROCKFALL_TYPE_COUNT, numRockFallTypes); i++) {
				Effect_Load_RockFallStyle(contRoot, stringParts[i + 1], effectGlobs.rockFallStyleCount, (RockFallType)i, stringParts[0]);
			}
		}
		else {
			Config_WarnItem(true, item, "No RockFall types defined.");
		}
		effectGlobs.rockFallStyleCount++;
	}
}

// <LegoRR.exe @0040c000>
bool32 __cdecl LegoRR::Effect_Load_RockFallStyle(Gods98::Container* contRoot, const char* filename, uint32 rockFallStyle, RockFallType rockFallType, const char* tumbleNullName)
{
	EffectRockFall* rockFall = &effectGlobs.rockFallEffects[rockFallStyle][rockFallType];

	rockFall->cont = Gods98::Container_Load(contRoot, filename, CONTAINER_LWSSTRING, true);
	if (rockFall->cont == nullptr) {
		rockFall->cont = Gods98::Container_Load(contRoot, filename, CONTAINER_ANIMSTRING, true);
	}

	if (rockFall->cont != nullptr) {
		/// FIX APPLY: Don't hide container until AFTER searching for nulls.
		uint32 tumbleNullFrames = 0; // dummy init
		const char* partName = Gods98::Container_FormatPartName(rockFall->cont, tumbleNullName, nullptr);
		Gods98::Container_SearchTree(rockFall->cont, partName, Gods98::Container_SearchMode::MatchCount, &tumbleNullFrames);
		rockFall->tumbleNullFrames = tumbleNullFrames;


		/// FIX APPLY: Don't store a temporary string variable here!!!
		rockFall->tumbleNullName = Gods98::Util_StrCpy(tumbleNullName);


		/// FIX APPLY: Don't hide container until AFTER searching for nulls.
		Gods98::Container_Hide(rockFall->cont, true);

		for (uint32 i = 0; i < EFFECT_ROCKFALL_MAXCONTAINERS; i++) {
			rockFall->contTable[i] = Gods98::Container_Clone(rockFall->cont);
			Gods98::Container_Hide(rockFall->contTable[i], true);

			rockFall->finishedTable[i] = true;
			rockFall->tumbleNullUsedTable[i] = 0x0;
		}

		return true;
	}
	return false;
}

// <LegoRR.exe @0040c0e0>
bool32 __cdecl LegoRR::Effect_Load_ElectricFenceBeam(Gods98::Container* contRoot, const char* filename, bool32 longBeam)
{
	EffectElectricFenceBeam* efenceBeam = &effectGlobs.efenceEffects[longBeam];

	efenceBeam->cont = Gods98::Container_Load(contRoot, filename, CONTAINER_LWSSTRING, true);
	if (efenceBeam->cont != nullptr) {
		Gods98::Container_Hide(efenceBeam->cont, true);

		for (uint32 i = 0; i < EFFECT_ELECTRICFENCE_MAXCONTAINERS; i++) {
			efenceBeam->contTable[i] = Gods98::Container_Clone(efenceBeam->cont);
			Gods98::Container_Hide(efenceBeam->contTable[i], true);

			efenceBeam->finishedTable[i] = true;
		}
		return true;
	}
	return false;
}

// <LegoRR.exe @0040c160>
bool32 __cdecl LegoRR::Effect_Spawn_RockFall(RockFallType rockFallType, sint32 bx, sint32 by, real32 xPos, real32 yPos, real32 zPos, real32 xDir, real32 yDir)
{
	EffectRockFall* rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][rockFallType];

	for (uint32 i = 0; i < EFFECT_ROCKFALL_MAXCONTAINERS; i++) {
		if (rockFall->finishedTable[i]) {
			Gods98::Container* cont = rockFall->contTable[i];
			Gods98::Container_Hide(cont, false);
			Gods98::Container_SetAnimationTime(cont, 0.0f);
			Gods98::Container_SetPosition(cont, nullptr, xPos, yPos, zPos);
			Gods98::Container_SetOrientation(cont, nullptr, xDir, yDir, 0.0f, 0.0f, 0.0f, -1.0f);
			
			rockFall->xBlockPosTable[i] = bx;
			rockFall->yBlockPosTable[i] = by;
			rockFall->finishedTable[i] = false;
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040c220>
bool32 __cdecl LegoRR::Effect_Spawn_ElectricFenceBeam(bool32 longBeam, real32 xPos, real32 yPos, real32 zPos, real32 xDir, real32 yDir, real32 zDir)
{
	EffectElectricFenceBeam* efenceBeam = &effectGlobs.efenceEffects[longBeam];

	for (uint32 i = 0; i < EFFECT_ELECTRICFENCE_MAXCONTAINERS; i++) {
		if (efenceBeam->finishedTable[i]) {
			Gods98::Container* cont = efenceBeam->contTable[i];
			Gods98::Container_Hide(cont, false);
			Gods98::Container_SetAnimationTime(cont, 0.0f);
			Gods98::Container_SetPosition(cont, nullptr, xPos, yPos, zPos);
			Gods98::Container_SetOrientation(cont, nullptr, xDir, yDir, zDir, 0.0f, 0.0f, -1.0f);

			efenceBeam->finishedTable[i] = false;
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040c2d0>
uint32 __cdecl LegoRR::Effect_UpdateAll(real32 elapsedWorld, OUT RockFallType** rockFallTypes, OUT uint32** rockFallIndexes)
{
	Effect_Update_Explosion(elapsedWorld);
	Effect_Update_BoulderExplode(elapsedWorld);
	Effect_Update_SmashPath(elapsedWorld);
	Effect_Update_MiscEffectsAll(elapsedWorld);

	uint32 rockFallsFinished = 0;
	for (uint32 rockFallType = 0; rockFallType < (uint32)ROCKFALL_TYPE_COUNT; rockFallType++) {
		EffectRockFall* rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][rockFallType];

		if (rockFall->cont == nullptr)
			continue;

		for (uint32 i = 0; i < EFFECT_ROCKFALL_MAXCONTAINERS; i++) {

			if (!rockFall->finishedTable[i]) {
				Gods98::Container* cont = rockFall->contTable[i];
				const real32 overrun = Gods98::Container_MoveAnimation(cont, elapsedWorld);
				if (overrun > 0.0f) {
					Gods98::Container_Hide(cont, true);

					rockFall->finishedTable[i] = true;
					rockFall->tumbleNullUsedTable[i] = 0x0;

					effectGlobs.rockFallCompletedTypes[rockFallsFinished] = (RockFallType)rockFallType;
					effectGlobs.rockFallCompletedIndexes[rockFallsFinished] = i;
					rockFallsFinished++;
				}
			}
		}
	}
	*rockFallTypes = effectGlobs.rockFallCompletedTypes;
	*rockFallIndexes = effectGlobs.rockFallCompletedIndexes;


	for (uint32 efenceType = 0; efenceType < EFFECT_ELECTRICFENCE_MAXTYPES; efenceType++) {
		EffectElectricFenceBeam* efenceBeam = &effectGlobs.efenceEffects[efenceType];

		if (efenceBeam->cont == nullptr)
			continue;

		for (uint32 i = 0; i < EFFECT_ELECTRICFENCE_MAXCONTAINERS; i++) {

			if (!efenceBeam->finishedTable[i]) {
				Gods98::Container* cont = efenceBeam->contTable[i];
				const real32 overrun = Gods98::Container_MoveAnimation(cont, elapsedWorld);
				if (overrun > 0.0f) {
					Gods98::Container_Hide(cont, true);

					efenceBeam->finishedTable[i] = true;
				}
			}
		}
	}

	return rockFallsFinished;
}

// <LegoRR.exe @0040c400>
void __cdecl LegoRR::Effect_Update_BoulderExplode(real32 elapsedWorld)
{
	for (uint32 i = 0; i < EFFECT_BOULDEREXPLODE_MAXCONTAINERS; i++) {
		if (effectGlobs.boulderExplodeContTable[i] != nullptr) {
			const real32 overrun = Gods98::Container_MoveAnimation(effectGlobs.boulderExplodeContTable[i], elapsedWorld);
			if (overrun > 0.0f) {
				Gods98::Container_Remove(effectGlobs.boulderExplodeContTable[i]);
				effectGlobs.boulderExplodeContTable[i] = nullptr;
			}
		}
	}
}

// <LegoRR.exe @0040c450>
void __cdecl LegoRR::Effect_Update_SmashPath(real32 elapsedWorld)
{
	for (uint32 i = 0; i < EFFECT_SMASHPATH_MAXCONTAINERS; i++) {
		if (effectGlobs.smashPathContTable[i] != nullptr) {
			const real32 overrun = Gods98::Container_MoveAnimation(effectGlobs.smashPathContTable[i], elapsedWorld);
			if (overrun > 0.0f) {
				Gods98::Container_Remove(effectGlobs.smashPathContTable[i]);
				effectGlobs.smashPathContTable[i] = nullptr;
			}
		}
	}
}

// <LegoRR.exe @0040c4a0>
void __cdecl LegoRR::Effect_GetBlockPos_RockFall(RockFallType rockFallType, uint32 index, OUT sint32* bx, OUT sint32* by)
{
	const EffectRockFall* rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][rockFallType];

	*bx = (sint32)rockFall->xBlockPosTable[index];
	*by = (sint32)rockFall->yBlockPosTable[index];
}

// <LegoRR.exe @0040c4e0>
Gods98::Container* __cdecl LegoRR::Effect_GetTumbleNull_RockFall(uint32 bx, uint32 by)
{
	for (uint32 rockFallType = 0; rockFallType < (uint32)ROCKFALL_TYPE_COUNT; rockFallType++) {
		EffectRockFall* rockFall = &effectGlobs.rockFallEffects[effectGlobs.rockFallStyleIndex][rockFallType];

		if (rockFall->cont == nullptr)
			continue;

		// Find a rockfall effect at the current block position.
		for (uint32 i = 0; i < EFFECT_ROCKFALL_MAXCONTAINERS; i++) {

			if (!rockFall->finishedTable[i] && bx == rockFall->xBlockPosTable[i] && by == rockFall->yBlockPosTable[i]) {

				/// SANITY: Avoid using more bits than we have available.
				for (uint32 j = 0; j < std::min(32u, rockFall->tumbleNullFrames); j++) {
					if (!(rockFall->tumbleNullUsedTable[i] & (1 << j))) {
						rockFall->tumbleNullUsedTable[i] |= (1 << j);

						const char* partName = Gods98::Container_FormatPartName(rockFall->contTable[i], rockFall->tumbleNullName, &j);
						return Gods98::Container_SearchTree(rockFall->contTable[i], partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
					}
				}
				return nullptr;
			}
		}
	}
	return nullptr;
}

// <LegoRR.exe @0040c5c0>
void __cdecl LegoRR::Effect_RemoveAll_BoulderExplode(void)
{
	for (uint32 i = 0; i < EFFECT_BOULDEREXPLODE_MAXCONTAINERS; i++) {
		if (effectGlobs.boulderExplodeContTable[i] != nullptr) {
			Gods98::Container_Remove(effectGlobs.boulderExplodeContTable[i]);
			effectGlobs.boulderExplodeContTable[i] = nullptr;
		}
	}
}

// <LegoRR.exe @0040c5f0>
void __cdecl LegoRR::Effect_RemoveAll_RockFall(void)
{
	/// FIX APPLY: Free rockFalls for ALL styles, not just current style.
	//const uint32 rockFallStyle = effectGlobs.rockFallStyleIndex;
	for (uint32 rockFallStyle = 0; rockFallStyle < EFFECT_ROCKFALL_MAXSTYLES; rockFallStyle++) {
		for (uint32 rockFallType = 0; rockFallType < (uint32)ROCKFALL_TYPE_COUNT; rockFallType++) {
			EffectRockFall* rockFall = &effectGlobs.rockFallEffects[rockFallStyle][rockFallType];

			for (uint32 i = 0; i < EFFECT_ROCKFALL_MAXCONTAINERS; i++) {
				Gods98::Container_Remove(rockFall->contTable[i]);
				rockFall->contTable[i] = nullptr;
			}

			/// FIX APPLY: Free tumbleNullName that was allocated in Effect_Load_RockFallStyle.
			///            Effect_Load_RockFallStyle MUST be implemented in order to do this.
			Gods98::Mem_Free(rockFall->tumbleNullName);
			rockFall->tumbleNullName = nullptr;

			/// TODO: We should also be removing the source container: rockFall->cont.
		}
	}
}

// <LegoRR.exe @0040c650>
void __cdecl LegoRR::Effect_Load_Explosion(Gods98::Container* contRoot, const char* filename)
{
	effectGlobs.explosionCont = Gods98::Container_Load(contRoot, filename, CONTAINER_LWSSTRING, true);
	Gods98::Container_Hide(effectGlobs.explosionCont, true);
}

// <LegoRR.exe @0040c680>
void __cdecl LegoRR::Effect_Spawn_Explosion(OPTIONAL LegoObject* liveObj, OPTIONAL const Point2F* wPos)
{
	Vector3F effectPos = { 0.0f, 0.0f, 0.0f }; // dummy init
	if (liveObj == nullptr) {
		effectPos.x = wPos->x;
		effectPos.y = wPos->y;
	}
	else {
		LegoObject_GetPosition(liveObj, &effectPos.x, &effectPos.y);
	}
	const real32 zPos = Map3D_GetWorldZ(Lego_GetMap(), effectPos.x, effectPos.y);
	effectPos.z = (zPos - 1.0f); // Explosions spawn a little bit into the ground I guess?

	if (effectGlobs.explosionCount < EFFECT_EXPLOSION_MAXCONTAINERS) {
		Gods98::Container* cont = Gods98::Container_Clone(effectGlobs.explosionCont);
		effectGlobs.explosionContTable[effectGlobs.explosionCount] = cont;
		effectGlobs.explosionCount++;

		Gods98::Container_Hide(cont, false);
		Gods98::Container_SetAnimationTime(cont, 0.0f);
		Gods98::Container_SetPosition(cont, nullptr, effectPos.x, effectPos.y, effectPos.z);
		Gods98::Container_SetOrientation(cont, nullptr, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f);
	}
}

// <LegoRR.exe @0040c760>
void __cdecl LegoRR::Effect_Update_Explosion(real32 elapsedWorld)
{
	for (uint32 i = 0; i < effectGlobs.explosionCount; i++) {

		const real32 overrun = Gods98::Container_MoveAnimation(effectGlobs.explosionContTable[i], elapsedWorld);
		/// CHANGE: overrun > 0.0f instead of != 0.0f.
		if (overrun > 0.0f) {
			Gods98::Container_Remove(effectGlobs.explosionContTable[i]);

			// Overwrite removed item with item at the end of the list.
			effectGlobs.explosionContTable[i] = effectGlobs.explosionContTable[effectGlobs.explosionCount - 1];
			effectGlobs.explosionContTable[effectGlobs.explosionCount - 1] = nullptr;
			effectGlobs.explosionCount--;
			i--;
		}
	}
}

// <LegoRR.exe @0040c7d0>
LegoRR::EffectMisc* __cdecl LegoRR::Effect_GetMiscEffectData(MiscEffectType miscEffectType)
{
	switch (miscEffectType) {
	case MISCOBJECT_LAZERHIT:
		return &effectGlobs.lazerHitEffect;
	case MISCOBJECT_PUSHERHIT:
		return &effectGlobs.pusherHitEffect;
	case MISCOBJECT_FREEZERHIT:
		return &effectGlobs.freezerHitEffect;
	case MISCOBJECT_PATHDUST:
		return &effectGlobs.pathDustEffect;
	case MISCOBJECT_LAVAEROSIONSMOKE1:
		return &effectGlobs.lavaErosionSmokeEffects[0];
	case MISCOBJECT_LAVAEROSIONSMOKE2:
		return &effectGlobs.lavaErosionSmokeEffects[1];
	case MISCOBJECT_LAVAEROSIONSMOKE3:
		return &effectGlobs.lavaErosionSmokeEffects[2];
	case MISCOBJECT_LAVAEROSIONSMOKE4:
		return &effectGlobs.lavaErosionSmokeEffects[3];
	case MISCOBJECT_BIRDSCARER:
		return &effectGlobs.birdScarerEffect;
	case MISCOBJECT_UPGRADEEFFECT:
		return &effectGlobs.upgradeEffect;
	default:
		return nullptr;
	}
}

// <LegoRR.exe @0040c850>
bool32 __cdecl LegoRR::Effect_Load_Misc(EffectMisc* effectMisc, Gods98::Container* contRoot, const char* filename)
{
	if (filename == nullptr)
		return false;

	effectMisc->cont = Gods98::Container_Load(contRoot, filename, CONTAINER_LWSSTRING, true);
	if (effectMisc->cont == nullptr) {
		effectMisc->cont = Gods98::Container_Load(contRoot, filename, CONTAINER_LWOSTRING, true);
	}

	if (effectMisc->cont != nullptr) {
		Gods98::Container_Hide(effectMisc->cont, true);
		return true;
	}
	return false;
}

// <LegoRR.exe @0040c8c0>
void __cdecl LegoRR::Effect_Initialise(const Gods98::Config* config, const char* gameName, Gods98::Container* contRoot)
{
	Effect_Load_Misc(&effectGlobs.lazerHitEffect, contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("LazerHit")));

	Effect_Load_Misc(&effectGlobs.pusherHitEffect, contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("PusherHit")));

	Effect_Load_Misc(&effectGlobs.freezerHitEffect, contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("FreezerHit")));

	Effect_Load_Misc(&effectGlobs.pathDustEffect, contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("PathDust")));

	Effect_Load_Misc(&effectGlobs.lavaErosionSmokeEffects[0], contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("LavaErosionSmoke1")));
	Effect_Load_Misc(&effectGlobs.lavaErosionSmokeEffects[1], contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("LavaErosionSmoke2")));
	Effect_Load_Misc(&effectGlobs.lavaErosionSmokeEffects[2], contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("LavaErosionSmoke3")));
	Effect_Load_Misc(&effectGlobs.lavaErosionSmokeEffects[3], contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("LavaErosionSmoke4")));

	Effect_Load_Misc(&effectGlobs.birdScarerEffect, contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("BirdScarer")));

	Effect_Load_Misc(&effectGlobs.upgradeEffect, contRoot,
					 Gods98::Config_GetTempStringValue(config, MiscObjects_ID("UpgradeEffect")));
}

// <LegoRR.exe @0040caa0>
void __cdecl LegoRR::Effect_Update_MiscEffect(EffectMisc* miscEffect, real32 elapsedWorld)
{
	for (uint32 i = 0; i < miscEffect->count; i++) {
		if (miscEffect == &effectGlobs.birdScarerEffect) {
			// Hardcoded sonic blaster effect behaviour to scare units after half a second.
			/// FIXME: We're using a single timer for any number of container effects playing at once!!!!
			miscEffect->birdScarerTimer += elapsedWorld;
			if (miscEffect->birdScarerTimer > (STANDARD_FRAMERATE / 2.0f)) { // 0.5 seconds
				Vector3F effectPos;
				Gods98::Container_GetPosition(miscEffect->contTable[i], nullptr, &effectPos);

				const Point2F effectPos2D = { effectPos.x, effectPos.y };
				LegoObject_ScareUnitsWithBigBang(nullptr, &effectPos2D, legoGlobs.BirdScarerRadius);

				miscEffect->birdScarerTimer = 0.0f;
			}
		}

		const real32 overrun = Gods98::Container_MoveAnimation(miscEffect->contTable[i], elapsedWorld);
		/// CHANGE: overrun > 0.0f instead of != 0.0f.
		if (overrun > 0.0f) {
			Gods98::Container_Remove(miscEffect->contTable[i]);

			// Overwrite removed item with item at the end of the list.
			miscEffect->contTable[i] = miscEffect->contTable[miscEffect->count - 1];
			miscEffect->contTable[miscEffect->count - 1] = nullptr;
			miscEffect->count--;
			i--;
		}
	}
}

// <LegoRR.exe @0040cb70>
void __cdecl LegoRR::Effect_Update_MiscEffectsAll(real32 elapsedWorld)
{
	Effect_Update_MiscEffect(&effectGlobs.lazerHitEffect, elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.pusherHitEffect, elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.freezerHitEffect, elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.pathDustEffect, elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.lavaErosionSmokeEffects[0], elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.lavaErosionSmokeEffects[1], elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.lavaErosionSmokeEffects[2], elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.lavaErosionSmokeEffects[3], elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.birdScarerEffect, elapsedWorld);
	Effect_Update_MiscEffect(&effectGlobs.upgradeEffect, elapsedWorld);
}

// <LegoRR.exe @0040cc10>
bool32 __cdecl LegoRR::Effect_Spawn_Particle(MiscEffectType miscEffectType, const Vector3F* wPos, OPTIONAL const Vector3F* dir)
{
	EffectMisc* miscEffect = Effect_GetMiscEffectData(miscEffectType);

	/// FIX APPLY: Null check BEFORE checking count!!
	if (miscEffect != nullptr && miscEffect->cont != nullptr && miscEffect->count < EFFECT_MISC_MAXCONTAINERS) {
		Gods98::Container* cont = Gods98::Container_Clone(miscEffect->cont);
		miscEffect->contTable[miscEffect->count] = cont;

		Gods98::Container_SetAnimationTime(cont, 0.0f);
		Gods98::Container_SetPosition(cont, nullptr, wPos->x, wPos->y, wPos->z);

		Vector3F effectDir = { 0.0f, 1.0f, 0.0f };
		if (dir != nullptr) {
			effectDir = *dir;
		}
		Gods98::Container_SetOrientation(cont, nullptr, effectDir.x, effectDir.y, effectDir.z, 0.0f, 0.0f, -1.0f);

		/// FIXME: We're using a single timer for any number of container effects playing at once!!!!
		miscEffect->birdScarerTimer = 0.0f;
		miscEffect->count++;

		return true;
	}
	return false;
}

#pragma endregion
