// PTL.cpp : 
//

#include "../../engine/core/Config.h"
#include "../../engine/core/Errors.h"
#include "../../engine/core/Memory.h"
#include "../../engine/core/Utils.h"

#include "../object/Object.h"
#include "../Game.h"

#include "../interface/Messages.h"
#include "PTL.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00556be0>
LegoRR::PTL_Globs & LegoRR::ptlGlobs = *(LegoRR::PTL_Globs*)0x00556be0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0045daa0>
bool32 __cdecl LegoRR::PTL_Initialise(const char* fname, const char* gameName)
{
    ptlGlobs.count = 0;

    Gods98::Config* ptl = Gods98::Config_Load(fname);
    if (ptl != nullptr) {

        const Gods98::Config* arrayFirst = Gods98::Config_FindArray(ptl, gameName);
        for (const Gods98::Config* prop = arrayFirst; prop != nullptr; prop = Gods98::Config_GetNextItem(prop)) {
            Error_Fatal(ptlGlobs.count >= PTL_MAXPROPERTIES, "Too many PTL properties");

            ptlGlobs.table[ptlGlobs.count].fromType = Message_ParsePTLName(Gods98::Config_GetItemName(prop));
            ptlGlobs.table[ptlGlobs.count].toType   = Message_ParsePTLName(Gods98::Config_GetDataString(prop));
            ptlGlobs.count++;
        }

        Gods98::Config_Free(ptl);
        return true;
    }
    return false;
}

// <LegoRR.exe @0045db30>
void __cdecl LegoRR::PTL_TranslateEvent(IN OUT Message_Event* message)
{
    for (uint32 i = 0; i < ptlGlobs.count; i++) {
        if (message->type == ptlGlobs.table[i].fromType) {
            message->type = ptlGlobs.table[i].toType;
            return;
        }
    }
    // No translation, use the original event message type.
}


#pragma endregion
