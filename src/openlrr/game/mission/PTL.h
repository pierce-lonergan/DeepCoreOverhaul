// PTL.h : 
//

#pragma once

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct Message_Event;

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define PTL_MAXPROPERTIES		40

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct PTL_Property // [LegoRR/PTL.c|struct:0x8] Property loaded from a level's PTL config file (contains lookup index for actions)
{
	/*0,4*/	Message_Type fromType; // The original "posted" event message type.
	/*4,4*/	Message_Type toType;   // The output "translated" event message type.
	/*8*/
};
assert_sizeof(PTL_Property, 0x8);


struct PTL_Globs // [LegoRR/PTL.c|struct:0x144|tags:GLOBS]
{
	/*000,140*/	PTL_Property table[PTL_MAXPROPERTIES];
	/*140,4*/	uint32 count;
	/*144*/
};
assert_sizeof(PTL_Globs, 0x144);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00556be0>
extern PTL_Globs & ptlGlobs;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0045daa0>
bool32 __cdecl PTL_Initialise(const char* fname, const char* gameName);

// <LegoRR.exe @0045db30>
void __cdecl PTL_TranslateEvent(IN OUT Message_Event* message);

#pragma endregion

}
