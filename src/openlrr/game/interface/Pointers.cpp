// Pointers.cpp : 
//

#include "../../engine/core/Utils.h"
#include "../../engine/Graphics.h"

#include "../Game.h"

#include "Pointers.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00501a98>
LegoRR::Pointer_Globs & LegoRR::pointerGlobs = *(LegoRR::Pointer_Globs*)0x00501a98;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0045caf0>
void __cdecl LegoRR::Pointer_Initialise(void)
{
	/// SANITY: Memzero globals.
	std::memset(&pointerGlobs, 0, sizeof(pointerGlobs));

	Pointer_RegisterName(Pointer_Standard);
	Pointer_RegisterName(Pointer_Blank); // Also used for back of flic pointers.
	Pointer_RegisterName(Pointer_Selected);
	Pointer_RegisterName(Pointer_Drill);
	Pointer_RegisterName(Pointer_CantDrill);
	Pointer_RegisterName(Pointer_Clear);
	Pointer_RegisterName(Pointer_Go);
	Pointer_RegisterName(Pointer_CantGo);
	Pointer_RegisterName(Pointer_Teleport);
	Pointer_RegisterName(Pointer_CantTeleport);
	Pointer_RegisterName(Pointer_Reinforce);
	Pointer_RegisterName(Pointer_CantReinforce);
	Pointer_RegisterName(Pointer_RadarPan);
	Pointer_RegisterName(Pointer_TrackObject);
	Pointer_RegisterName(Pointer_Help);
	Pointer_RegisterName(Pointer_CantHelp);
	Pointer_RegisterName(Pointer_PutDown);
	Pointer_RegisterName(Pointer_GetIn);
	Pointer_RegisterName(Pointer_GetOut);
	Pointer_RegisterName(Pointer_TutorialBlockInfo);
	Pointer_RegisterName(Pointer_Okay);
	Pointer_RegisterName(Pointer_NotOkay);
	Pointer_RegisterName(Pointer_CanBuild);
	Pointer_RegisterName(Pointer_CannotBuild);
	Pointer_RegisterName(Pointer_Dynamite);
	Pointer_RegisterName(Pointer_CantDynamite);
	Pointer_RegisterName(Pointer_PickUp);
	Pointer_RegisterName(Pointer_CantPickUp);
	Pointer_RegisterName(Pointer_PickUpOre);
	Pointer_RegisterName(Pointer_LegoManCantDig);
	Pointer_RegisterName(Pointer_VehicleCantDig);
	Pointer_RegisterName(Pointer_LegoManDig);
	Pointer_RegisterName(Pointer_VehicleDig);
	Pointer_RegisterName(Pointer_LegoManCantPickUp);
	Pointer_RegisterName(Pointer_VehicleCantPickUp);
	Pointer_RegisterName(Pointer_LegoManPickUp);
	Pointer_RegisterName(Pointer_VehiclePickUp);
	Pointer_RegisterName(Pointer_LegoManCantGo);
	Pointer_RegisterName(Pointer_VehicleCantGo);
	Pointer_RegisterName(Pointer_LegoManGo);
	Pointer_RegisterName(Pointer_VehicleGo);
	Pointer_RegisterName(Pointer_LegoManClear);
	Pointer_RegisterName(Pointer_VehicleClear);
	Pointer_RegisterName(Pointer_SurfaceType_Immovable);
	Pointer_RegisterName(Pointer_SurfaceType_Hard);
	Pointer_RegisterName(Pointer_SurfaceType_Medium);
	Pointer_RegisterName(Pointer_SurfaceType_Loose);
	Pointer_RegisterName(Pointer_SurfaceType_Soil);
	Pointer_RegisterName(Pointer_SurfaceType_Lava);
	Pointer_RegisterName(Pointer_SurfaceType_Water);
	Pointer_RegisterName(Pointer_SurfaceType_OreSeam);
	Pointer_RegisterName(Pointer_SurfaceType_Lake);
	Pointer_RegisterName(Pointer_SurfaceType_CrystalSeam);
	Pointer_RegisterName(Pointer_SurfaceType_RechargeSeam);
	Pointer_RegisterName(Pointer_CantZoom);
	Pointer_RegisterName(Pointer_Zoom);
}

// <LegoRR.exe @0045cd30>
void __cdecl LegoRR::Pointer_Load(const Gods98::Config* config)
{
	/// NOTE: config is the first item in the array of pointers!!
	for (const Gods98::Config* item = config; item != nullptr; item = Gods98::Config_GetNextItem(item)) {

		const char* pointerName = Gods98::Config_GetItemName(item);
		bool reduced = false;
		if (std::strlen(pointerName) > 0 && pointerName[0] == '!') {
			if (!Gods98::Graphics_IsReduceImages()) {
				pointerName++; // Skip the '!' character to get the real name.
			}
			else {
				reduced = true;
			}
		}

		Pointer_Type pointerType;
		if (!reduced && Pointer_GetType(pointerName, &pointerType)) {
			char* argv[3];
			char buff[1024];
			std::strcpy(buff, Gods98::Config_GetDataString(item));

			const uint32 argc = Gods98::Util_TokeniseSafe(buff, argv, ",", 3);
			if (argc == 1) { // <PointerType>    <bmpPath>
				pointerGlobs.images[pointerType].image = Gods98::Image_LoadBMP(argv[0]);
				pointerGlobs.imageIsFlic[pointerType] = false;

				if (pointerGlobs.images[pointerType].image == nullptr) {
					if (pointerType == Pointer_Standard || pointerType == Pointer_Blank) {
						// These pointers cannot be replaced.
						Error_FatalF(true, "Failed to load %s image: %s, no fallback pointer to use", pointerName, argv[0]);
					}
					else {
						// Failure is OKAY here, because the game will fallback to the standard pointer.
						Error_WarnF(true, "Failed to load %s image: %s, falling back to %s", pointerName, argv[0],
									pointerGlobs.pointerName[Pointer_Standard]);
					}
				}
				/// REFACTOR: Remove isFlic check and move inside block.
				Gods98::Image_SetupTrans(pointerGlobs.images[pointerType].image, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			}
			else { // <PointerType>    <flicPath>,<xOff>,<yOff>
				Error_Fatal((argc < 3), "Not enough commas for flic pointer definition.");

				if (pointerType == Pointer_Standard || pointerType == Pointer_Blank) {
					Error_FatalF(true, "The pointer type %s is special, and must be loaded as an image, not a flic: %s", pointerName, argv[0]);
				}

				const auto userFlags = (Gods98::FlicUserFlags::FLICMEMORY|Gods98::FlicUserFlags::FLICLOOPINGON);
				if (!Gods98::Flic_Setup(argv[0], &pointerGlobs.images[pointerType].flic, userFlags)) {
					Error_FatalF(true, "Failed to load %s flic: %s", pointerName, argv[0]);
				}
				pointerGlobs.imageIsFlic[pointerType] = true;
				pointerGlobs.flicOffsets[pointerType].x = std::atoi(argv[1]);
				pointerGlobs.flicOffsets[pointerType].y = std::atoi(argv[2]);
			}
		}
	}

	/// SANITY: Error checking for things that will explode later.
	bool anyFlics = false;
	for (uint32 i = 0; i < Pointer_Type_Count && !anyFlics; i++) {
		if (pointerGlobs.imageIsFlic[i])
			anyFlics = true;
	}

	// These are special pointers that are required
	if (pointerGlobs.images[Pointer_Standard].image == nullptr) {
		Error_FatalF(true, "No pointer image defined for %s, this is required as a fallback pointer for images.",
					 pointerGlobs.pointerName[Pointer_Standard]);
	}
	if (anyFlics && pointerGlobs.images[Pointer_Blank].image == nullptr) {
		Error_FatalF(true, "No pointer image defined for %s, this is required when using flic pointers.",
					 pointerGlobs.pointerName[Pointer_Blank]);
	}
}

// <LegoRR.exe @0045ce90>
bool32 __cdecl LegoRR::Pointer_GetType(const char* pointerName, OUT Pointer_Type* pointerType)
{
	for (uint32 i = 0; i < Pointer_Type_Count; i++) {
		if (::_stricmp(pointerGlobs.pointerName[i], pointerName) == 0) {
			*pointerType = static_cast<Pointer_Type>(i);
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0045ced0>
Gods98::Image_Flic __cdecl LegoRR::Pointer_GetImage(Pointer_Type pointerType)
{
	return pointerGlobs.images[pointerType];
}

// <LegoRR.exe @0045cee0>
void __cdecl LegoRR::Pointer_SetCurrent_IfTimerFinished(Pointer_Type pointerType)
{
	if (pointerGlobs.timer <= 0.0f) {
		/// FUTURE CHANGE: Check against current pointer type so that we can handle pointer animation framerate.
		if (pointerGlobs.currType != pointerType) {
			pointerGlobs.currType = pointerType;
			// Reset framerate here.
		}
	}
}

// <LegoRR.exe @0045cf00>
void __cdecl LegoRR::Pointer_SetCurrent(Pointer_Type pointerType, real32 timer)
{
	/// FUTURE CHANGE: Check against current pointer type so that we can handle pointer animation framerate.
	if (pointerGlobs.currType != pointerType) {
		pointerGlobs.currType = pointerType;
		// Reset framerate here.
	}
	pointerGlobs.timer = timer;
}

// <LegoRR.exe @0045cf20>
LegoRR::Pointer_Type __cdecl LegoRR::Pointer_GetCurrentType(void)
{
	return pointerGlobs.currType;
}

// <LegoRR.exe @0045cf30>
void __cdecl LegoRR::Pointer_DrawPointer(uint32 mouseX, uint32 mouseY)
{
	Point2F destPos = {
		static_cast<real32>(mouseX),
		static_cast<real32>(mouseY),
	};
	if (!pointerGlobs.imageIsFlic[pointerGlobs.currType]) {
		// Draw a BMP image pointer.
		Gods98::Image* image = pointerGlobs.images[pointerGlobs.currType].image;
		if (image == nullptr) {
			image = pointerGlobs.images[Pointer_Standard].image;
		}
		Gods98::Image_Display(image, &destPos);
	}
	else {
		Gods98::Image* blankImage = pointerGlobs.images[Pointer_Blank].image;
		Gods98::Flic* flic = pointerGlobs.images[pointerGlobs.currType].flic;
		Area2F destArea = {
			destPos.x + pointerGlobs.flicOffsets[pointerGlobs.currType].x,
			destPos.y + pointerGlobs.flicOffsets[pointerGlobs.currType].y,
			static_cast<real32>(Gods98::Flic_GetWidth(flic)),
			static_cast<real32>(Gods98::Flic_GetHeight(flic)),
		};

		// Display the blank pointer that the flic animation is drawn over.
		Gods98::Image_Display(blankImage, &destPos);

		const bool freezeInterface = (legoGlobs.flags1 & GAME1_FREEZEINTERFACE);

		Gods98::Flic_Animate(flic, &destArea, !freezeInterface, true);
	}
}

// <LegoRR.exe @0045d050>
void __cdecl LegoRR::Pointer_Update(real32 elapsedReal)
{
	if (pointerGlobs.timer > 0.0f) {
		pointerGlobs.timer -= elapsedReal;
	}

	/// TOOD: Track frame rate for flic here, since it doesn't do it on its own.
}

#pragma endregion
