// DamageText.h : 
//

#pragma once

#include "../../engine/gfx/Containers.h"
#include "../../engine/gfx/Mesh.h"

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define DAMAGETEXT_DURATION			(STANDARD_FRAMERATE * 0.6f) // 0.6 seconds (15.0 standard units).
#define DAMAGETEXT_MAXCHARS			4		// Max number of digit and minus symbol characters per damage text.
#define DAMAGETEXT_MAXVALUE			999		// Cannot have more than (MAXCHARS - 1) digits.
#define DAMAGETEXT_DIGITS			10		// Number of digit textures in array, must be 10 or more.
#define DAMAGETEXT_MAXSHOWN			10		// Number of damage texts that can be shown at once.
											// Not to be confused with digits.

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum DamageTextFlags : uint32 // [LegoRR/DamageFont.c|flags:0x4|type:uint]
{
	DAMAGETEXT_FLAG_NONE = 0,
	DAMAGETEXT_FLAG_USED = 0x1,
};
flags_end(DamageTextFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct DamageTextData // [LegoRR/DamageFont.c|struct:0x20]
{
	/*00,4*/	Gods98::Container* cont;
	/*04,4*/	Gods98::Mesh* mesh; // Mesh with MAXCHARS groups.
	/*08,4*/	uint32 charCount; // Number of mesh groups currently being used.
	/*0c,c*/	Vector3F offsetPos;
	/*18,4*/	real32 timer; // Count-up timer to DAMAGETEXT_DURATION.
	/*1c,4*/	DamageTextFlags flags;
	/*20*/
};
assert_sizeof(DamageTextData, 0x20);


struct DamageText_Globs // [LegoRR/DamageFont.c|struct:0x16c|tags:GLOBS]
{
	/*000,28*/	Gods98::Mesh_Texture* fontTextDigitsTable[DAMAGETEXT_DIGITS];
	/*028,4*/	Gods98::Mesh_Texture* fontTextMinus;
	/*02c,140*/	DamageTextData table[DAMAGETEXT_MAXSHOWN];
	/*16c*/
};
assert_sizeof(DamageText_Globs, 0x16c);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004b9a58>
extern DamageText_Globs & damageTextGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0040a300>
//#define DamageText_RemoveAll ((void (__cdecl* )(void))0x0040a300)
void __cdecl DamageText_RemoveAll(void);

// <LegoRR.exe @0040a330>
//#define DamageText_LoadTextures ((void (__cdecl* )(const char* dirName, const char* fileBaseName))0x0040a330)
void __cdecl DamageText_LoadTextures(const char* dirName, const char* fileBaseName);

// <LegoRR.exe @0040a3e0>
//#define DamageText_ShowNumber ((void (__cdecl* )(LegoObject* liveObj, uint32 displayNumber))0x0040a3e0)
void __cdecl DamageText_ShowNumber(LegoObject* liveObj, uint32 displayNumber);

// <LegoRR.exe @0040a4f0>
//#define DamageText_GetNextFree ((DamageTextData* (__cdecl* )(void))0x0040a4f0)
DamageTextData* __cdecl DamageText_GetNextFree(void);

// <LegoRR.exe @0040a510>
//#define DamageText_SetNumber ((void (__cdecl* )(DamageTextData* damageText, uint32 displayNumber))0x0040a510)
void __cdecl DamageText_SetNumber(DamageTextData* damageText, uint32 displayNumber);

// <LegoRR.exe @0040a670>
//#define DamageText_MeshRenderCallback ((void (__cdecl* )(Gods98::Mesh* mesh, DamageTextData* damageText, Gods98::Viewport* view))0x0040a670)
void __cdecl DamageText_MeshRenderCallback(Gods98::Mesh* mesh, void* pDamageText, Gods98::Viewport* view);

// <LegoRR.exe @0040a940>
//#define DamageText_UpdateAll ((void (__cdecl* )(real32 elapsedInterface))0x0040a940)
void __cdecl DamageText_UpdateAll(real32 elapsedInterface);

// <LegoRR.exe @0040a970>
//#define DamageText_Update ((void (__cdecl* )(DamageTextData* damageText, real32 elapsedInterface))0x0040a970)
void __cdecl DamageText_Update(DamageTextData* damageText, real32 elapsedInterface);

// <LegoRR.exe @0040aa10>
//#define DamageText_CanShow ((bool32 (__cdecl* )(LegoObject* liveObj))0x0040aa10)
bool32 __cdecl DamageText_CanShow(LegoObject* liveObj);

#pragma endregion

}
