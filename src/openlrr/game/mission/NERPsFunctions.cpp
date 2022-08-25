// NERPsFunctions.cpp : 
//

#include "../../engine/drawing/DirectDraw.h"
#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "../interface/TextMessages.h"
#include "../Game.h"

#include "NERPsFunctions.h"


using Gods98::Keys;


/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions


// <LegoRR.exe @00456990>
sint32 __cdecl LegoRR::NERPFunc__SetMessage(sint32* stack)
{
	if (NERPsRuntime_IsMessagePermit()) {
		return 0; // Note that this is marked as a void return NERPs function.
	}

	uint32 lineIndex = stack[0];
	if (lineIndex != 0) {
		lineIndex--; // 1-indexed, unless someone messed up and used 0-indexed...
	}

	const char* text = NERPsFile_GetMessageLine(lineIndex);

	if (lineIndex != nerpsfileGlobs.lineIndexArray_7c[nerpsfileGlobs.uint_a0]) {
		if (nerpsfileGlobs.uint_a0 == 8) {
			nerpsfileGlobs.lineIndexArray_7c[1] = nerpsfileGlobs.lineIndexArray_7c[5];
			nerpsfileGlobs.lineIndexArray_7c[2] = nerpsfileGlobs.lineIndexArray_7c[6];
			nerpsfileGlobs.lineIndexArray_7c[3] = nerpsfileGlobs.lineIndexArray_7c[7];
			nerpsfileGlobs.lineIndexArray_7c[4] = nerpsfileGlobs.lineIndexArray_7c[8];
			nerpsfileGlobs.uint_a0 = 4;
		}

		nerpsfileGlobs.lineIndexArray_7c[nerpsfileGlobs.uint_a0 + 1] = lineIndex;
		nerpsfileGlobs.uint_a0++;

		/// TODO: Is this check impossible to hit???
		if (nerpsfileGlobs.uint_a0 != 0) {
			Lego_SetFlags2_80(true);
		}
		if (nerpsfileGlobs.int_a4 != 0) {
			nerpsfileGlobs.int_a4++;
		}
	}

	const sint32 jankUnusedCounter = stack[1];

	Text_SetNERPsMessage(text, jankUnusedCounter); // Non-zero for unused counter.
	nerpsruntimeGlobs.nextArrowDisabled = jankUnusedCounter; // Non-zero for next arrow disabled (probably was how long to disable(?)).
	nerpsBOOL_004a7740 = true;

	if (NERPsRuntime_GetMessageWait()) {
		// Ensure the screen has rendered before going into the endless wait loop.
		Gods98::DirectDraw_Flip();

		// ...What is going on here? An infinite loop until the user presses enter??
		// ...This is really bad.

		while (!Input_IsKeyPressed(Keys::KEY_RETURN)) {
			/// CHANGE: Switch to full-blown update loop for proper mouse input support,
			///         and not freezing the entire dang application.
			// Use extension of Main_LoopUpdate that disables graphics updates, since we don't need that.
			Gods98::Main_LoopUpdate2(false, false);
			//Gods98::Input_ReadKeys();
		}
	}

	return stack[0]; // Note that this is marked as a void return NERPs function.
}


#pragma endregion
