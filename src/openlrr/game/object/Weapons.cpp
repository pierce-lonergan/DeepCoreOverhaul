// Weapons.cpp : 
//

#include "../../engine/core/Maths.h"
#include "../../engine/core/Utils.h"
#include "../../engine/gfx/Mesh.h"
#include "../../engine/input/Input.h"

#include "../Game.h"
#include "../audio/SFX.h"
#include "../effects/Effects.h"
#include "../front/Reward.h"
#include "BezierCurve.h"
#include "Collision.h"
#include "Object.h"
#include "Stats.h"
#include "Weapons.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00504870>
LegoRR::Weapon_Globs & LegoRR::weaponGlobs = *(LegoRR::Weapon_Globs*)0x00504870;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0046ee40>
bool32 __cdecl LegoRR::Weapon_Initialise(const Gods98::Config* config, const char* gameName)
{
	/// SANITY: Memzero all weapons data before using.
	std::memset(&weaponGlobs, 0, sizeof(weaponGlobs));

	weaponGlobs.config = config;
	weaponGlobs.weaponCount = 0;

	const Gods98::Config* arrayFirst = Gods98::Config_FindArray(config, Lego_ID("WeaponTypes"));
	if (arrayFirst == nullptr) {
		return false;
	}

	for (const Gods98::Config* item = arrayFirst; item != nullptr; item = Gods98::Config_GetNextItem(item)) {
		weaponGlobs.weaponCount++;
	}

	const uint32 allocSize1 = weaponGlobs.weaponCount * sizeof(WeaponStats);
	weaponGlobs.weaponStatsList = (WeaponStats*)Gods98::Mem_Alloc(allocSize1);
	if (weaponGlobs.weaponStatsList == nullptr) {
		return false;
	}

	/// FIX APPLY: ZERO OUT MEMORY SO THAT WE DON'T GET THE DISCHARGE GLITCH WITH POWER BEAM.
	std::memset(weaponGlobs.weaponStatsList, 0, allocSize1);

	for (uint32 i = 0; i< weaponGlobs.weaponCount; i++) {
		WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[i];

		for (uint32 objType = 0; objType < LegoObject_Type_Count; objType++) {
			for (uint32 objID = 0; objID < LegoObject_ID_Count; objID++) {
				for (uint32 objLevel = 0; objLevel < OBJECT_MAXLEVELS; objLevel++) {
					weaponStats->objectCoefs[objType][objID][objLevel] = -1.0f;
				}
			}
		}
		/// FIX APPLY: ZERO OUT MEMORY SO THAT WE DON'T GET THE DISCHARGE GLITCH WITH POWER BEAM.
		weaponStats->isSlowDeath = false;
		weaponStats->slowDeathInitialCoef = 0.0f;
		weaponStats->slowDeathDuration = 0.0f;
		weaponStats->rechargeTime = 0.0f;
		weaponStats->damage = 0.0f;
		weaponStats->dischargeRate = 0.0f;
		weaponStats->ammo = 0;
		weaponStats->weaponRange = 150.0f;
		for (uint32 surfaceType = 0; surfaceType < Lego_SurfaceType_Count; surfaceType++) {
			weaponStats->wallDestroyTimes[surfaceType] = 5.0f;
		}
	}

	const uint32 allocSize2 = weaponGlobs.weaponCount * sizeof(char*);
	weaponGlobs.weaponNameList = (char**)Gods98::Mem_Alloc(allocSize2);

	/// SANITY: Memzero
	std::memset(weaponGlobs.weaponNameList, 0, allocSize2);

	if (weaponGlobs.weaponNameList != nullptr) {

		const Gods98::Config* item = arrayFirst;
		for (uint32 i = 0; item != nullptr; i++, item = Gods98::Config_GetNextItem(item)) {
			WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[i];

			weaponGlobs.weaponNameList[i] = Gods98::Util_StrCpy(Gods98::Config_GetItemName(item));

			char* stringParts[16];
			char surfaceNameBuff[128];

			const Gods98::Config* statFirst = Gods98::Config_FindArray(config, Lego_ID("WeaponTypes", Gods98::Config_GetItemName(item)));
			for (const Gods98::Config* stat = statFirst; stat != nullptr; stat = Gods98::Config_GetNextItem(stat)) {

				if (::_stricmp(Gods98::Config_GetItemName(stat), "SlowDeath") == 0) {
					/// FIX APPLY: Don't modify the config strings. BAD! NO TOUCH!
					char* str = Gods98::Util_StrCpy(Gods98::Config_GetDataString(stat));
					const uint32 argc = Gods98::Util_Tokenise(str, stringParts, ":");
					Error_Fatal(argc < 2, "Not enough parts in WeaponTypes SlowDeath");

					weaponStats->isSlowDeath = true;
					weaponStats->slowDeathInitialCoef = std::atof(stringParts[0]);
					weaponStats->slowDeathDuration = std::atof(stringParts[1]);

					Gods98::Mem_Free(str);
				}
				else if (::_stricmp(Gods98::Config_GetItemName(stat), "RechargeTime") == 0) {
					weaponStats->rechargeTime = std::atof(Gods98::Config_GetDataString(stat));
				}
				else if (::_stricmp(Gods98::Config_GetItemName(stat), "DefaultDamage") == 0) {
					weaponStats->damage = std::atof(Gods98::Config_GetDataString(stat));
				}
				else if (::_stricmp(Gods98::Config_GetItemName(stat), "DischargeRate") == 0) {
					weaponStats->dischargeRate = std::atof(Gods98::Config_GetDataString(stat));
				}
				else if (::_stricmp(Gods98::Config_GetItemName(stat), "Ammo") == 0) {
					/// FIX APPLY: Ammo was assigned to zero whenever a check for its name failed (after surfaceTypes, before objectCoefs)
					weaponStats->ammo = std::atoi(Gods98::Config_GetDataString(stat));
				}
				else if (::_stricmp(Gods98::Config_GetItemName(stat), "WeaponRange") == 0) {
					weaponStats->weaponRange = std::atof(Gods98::Config_GetDataString(stat));
				}
				else {
					uint32 surfaceType;
					for (surfaceType = 0; surfaceType < Lego_SurfaceType_Count; surfaceType++) {
						std::sprintf(surfaceNameBuff, "WallDestroyTime_%s", legoGlobs.surfaceName[surfaceType]);
						if (::_stricmp(Gods98::Config_GetItemName(stat), surfaceNameBuff) == 0) {
							weaponStats->wallDestroyTimes[surfaceType] = std::atof(Gods98::Config_GetDataString(stat));
							break;
						}
					}

					// Key wasn't a surface type, the only other thing it can be is object coefs.
					if (surfaceType == Lego_SurfaceType_Count) {
						LegoObject_Type objType = LegoObject_None;
						LegoObject_ID objID = (LegoObject_ID)0;
						/// FIX APPLY: Don't infinite loop when failing to parse an object name...
						if (Lego_GetObjectByName(Gods98::Config_GetItemName(stat), &objType, &objID, nullptr)) {
							const uint32 objLevelCount = Stats_GetLevels(objType, objID);

							/// FIX APPLY: Don't modify the config strings. BAD! NO TOUCH!
							char* str = Gods98::Util_StrCpy(Gods98::Config_GetDataString(stat));
							const uint32 argc = Gods98::Util_Tokenise(str, stringParts, ":");
							Error_Fatal(argc < objLevelCount, "Not enough levels in WeaponTypes object coefs");

							for (uint32 objLevel = 0; objLevel < objLevelCount; objLevel++) {
								weaponStats->objectCoefs[objType][objID][objLevel] = std::atof(stringParts[objLevel]);
							}

							Gods98::Mem_Free(str);
						}
					}
				}
			}
		}
		return true;


		//Gods98::Mem_Free(weaponGlobs.weaponNameList);
	}
	Gods98::Mem_Free(weaponGlobs.weaponStatsList);

	return false;
}

#pragma endregion
