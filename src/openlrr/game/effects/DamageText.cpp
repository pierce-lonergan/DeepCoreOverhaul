// DamageText.cpp : 
//

#include "../../engine/core/Files.h"
#include "../../engine/core/Maths.h"
#include "../../engine/gfx/Mesh.h"

#include "../object/Object.h"
#include "../object/Stats.h"
#include "../Game.h"
#include "DamageText.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004b9a58>
LegoRR::DamageText_Globs & LegoRR::damageTextGlobs = *(LegoRR::DamageText_Globs*)0x004b9a58;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0040a300>
void __cdecl LegoRR::DamageText_RemoveAll(void)
{
	for (uint32 i = 0; i < DAMAGETEXT_MAXSHOWN; i++) {
		DamageTextData* damageText = &damageTextGlobs.table[i];
		if (damageText->flags & DAMAGETEXT_FLAG_USED) {
			damageText->flags &= ~DAMAGETEXT_FLAG_USED;
			Gods98::Container_Hide(damageText->cont, true);
		}
	}
}

// <LegoRR.exe @0040a330>
void __cdecl LegoRR::DamageText_LoadTextures(const char* dirName, const char* fileBaseName)
{
	char baseDir[FILE_MAXPATH];
	char name[FILE_MAXPATH];

	// Use <= to include '-' at index 10.
	for (uint32 i = 0; i <= DAMAGETEXT_DIGITS; i++) {
		std::sprintf(baseDir, "%s\\", dirName);
		std::sprintf(name, "%s%i.bmp", (fileBaseName ? fileBaseName : ""), i);

		if (i < DAMAGETEXT_DIGITS) {
			damageTextGlobs.fontTextDigitsTable[i] = Gods98::Mesh_LoadTexture(baseDir, name, nullptr, nullptr);
		}
		else {
			// 10, minus uses the filename a000_10.bmp.
			damageTextGlobs.fontTextMinus = Gods98::Mesh_LoadTexture(baseDir, name, nullptr, nullptr);
		}
	}
}

// <LegoRR.exe @0040a3e0>
void __cdecl LegoRR::DamageText_ShowNumber(LegoObject* liveObj, uint32 displayNumber)
{
	const uint32 FACE_DATA[2 * 3] = {
		0, 2, 1,
		0, 3, 2,
	};

	if (!DamageText_CanShow(liveObj))
		return; // Don't show damage numbers for this object.

	/// FIX APPLY: Support damage value of 999 (originally comparison cut off 999 and stopped at 998).
	/// TODO: Consider allowing damage above the cap and just showing the cap like many RPGs do.
	if (displayNumber <= 0 || displayNumber > DAMAGETEXT_MAXVALUE)
		return; // Number isn't supported.


	// Note that the GetNextFree function automatically marks the damageText as used,
	//  so we need to make sure we use it.
	DamageTextData* damageText = DamageText_GetNextFree();
	if (damageText != nullptr) {
		// We have an unused damage text structure.

		// Assign default values.
		damageText->timer = 0.0f;
		damageText->offsetPos = Vector3F {
			0.0f,
			0.0f,
			-StatsObject_GetCollHeight(liveObj), // Negative collHeight.
		};

		// Setup the container.
		if (damageText->cont == nullptr) {
			// This has never been used before. Create a container using the object as a parent.
			// Then add a mesh to the container.
			Gods98::Container* objCont = LegoObject_GetActivityContainer(liveObj);
			damageText->cont = Gods98::Container_Create(objCont);

			// Combination flag of: (MESH_TRANSFORM_FLAG_PARENTPOS|MESH_RENDER_FLAG_ALPHATRANS)
			const Gods98::Mesh_RenderFlags renderFlags = Gods98::Mesh_RenderFlags::MESH_RENDERFLAGS_LWOALPHA;

			//auto frame = damageText->cont->activityFrame;
			auto frame = Gods98::Container_GetActivityFrame(damageText->cont);

			damageText->mesh = Gods98::Mesh_CreateOnFrame(frame, DamageText_MeshRenderCallback,
														  renderFlags, damageText, Gods98::Mesh_Type::Norm);

			// Add mesh groups for the max number of supported characters.
			for (uint32 i = 0; i < DAMAGETEXT_MAXCHARS; i++) {
				Gods98::Mesh_AddGroup(damageText->mesh, 4, 2, 3, FACE_DATA);
			}
		}
		else {
			// This has been used before. Change the container's parent to the object and unhide it.
			Gods98::Container* objCont = LegoObject_GetActivityContainer(liveObj);
			Gods98::Container_SetParent(damageText->cont, objCont);
			Gods98::Container_Hide(damageText->cont, false);
		}

		// Update the mesh groups to display this number.
		DamageText_SetNumber(damageText, displayNumber);
	}
}

// <LegoRR.exe @0040a4f0>
LegoRR::DamageTextData* __cdecl LegoRR::DamageText_GetNextFree(void)
{
	for (uint32 i = 0; i < DAMAGETEXT_MAXSHOWN; i++) {
		DamageTextData* damageText = &damageTextGlobs.table[i];
		if (!(damageText->flags & DAMAGETEXT_FLAG_USED)) {
			damageText->flags |= DAMAGETEXT_FLAG_USED;
			return damageText;
		}
	}
	return nullptr;
}

// <LegoRR.exe @0040a510>
void __cdecl LegoRR::DamageText_SetNumber(DamageTextData* damageText, uint32 displayNumber)
{
	// Colourize the character mesh groups based on how much damage is being displayed.
	for (uint32 i = 0; i < DAMAGETEXT_MAXCHARS; i++) {
		ColourRGBF col; // Diffuse and Emissive colour.
		if (displayNumber < 5) {
			col = ColourRGBF { 0.0f, 1.0f, 0.0f };  // Lime green    [1,4]
		}
		else if (displayNumber < 10) {
			col = ColourRGBF { 1.0f, 1.0f, 0.0f };  // Bright yellow [5,9]
		}
		else {
			col = ColourRGBF { 1.0f, 0.25f, 0.0f }; // Dark orange   [10,999]
		}
		Gods98::Mesh_SetGroupColour(damageText->mesh, i, col.r, col.g, col.b, Gods98::Mesh_Colour::Diffuse);
		Gods98::Mesh_SetGroupColour(damageText->mesh, i, col.r, col.g, col.b, Gods98::Mesh_Colour::Emissive);
	}

	/// CHANGE: Allow damage above the cap and just show the cap like many RPGs do.
	if (displayNumber > DAMAGETEXT_MAXVALUE) {
		displayNumber = DAMAGETEXT_MAXVALUE;
	}

	// Count how many digits are in the number.
	/// CHANGE: Support 0 as a number.
	/// SANITY: Use uint64 so that we can support up to the same number of digits as INT_MAX.
	uint32 digitCount = 1;
	uint64 tempNumber = 10;
	while (tempNumber <= displayNumber) {
		digitCount++;
		tempNumber *= 10;
	}

	/// SANITY: Enforce cap on number of digits in-case something is defined incorrectly.
	if (digitCount > (DAMAGETEXT_MAXCHARS - 1)) {
		Error_WarnF(true, "Trying to display damage text with %i digits, but a max of %i digits are supported.",
						  digitCount, (DAMAGETEXT_MAXCHARS - 1));
		digitCount = (DAMAGETEXT_MAXCHARS - 1);
	}

	// Assign digits to each character mesh group and unhide the groups.
	// Earlier groups are for lower digits. So groups are displayed right to left.
	tempNumber = displayNumber;
	for (uint32 i = 0; i < digitCount; i++) {
		static_assert(DAMAGETEXT_DIGITS >= 10, "Unexpected number of DamageText digits!");
		const uint64 digit = (tempNumber % 10); // Get the current place digit.

		// C6385: Incorrectly complaining because we assign displayNumber to a max of DAMAGETEXT_MAXVALUE.
		Gods98::Mesh_SetGroupTexture(damageText->mesh, i, damageTextGlobs.fontTextDigitsTable[digit]);
		Gods98::Mesh_HideGroup(damageText->mesh, i, false);

		tempNumber /= 10; // Move to the next digit.
	}

	// Set the next character mesh group to the minus symbol.
	Gods98::Mesh_SetGroupTexture(damageText->mesh, digitCount, damageTextGlobs.fontTextMinus);
	/// FIX APPLY: Unhide the minus sign, which may be hidden or unhidden
	///             depending on the last usage of this damageText.
	// Don't show minus sign if zero.
	Gods98::Mesh_HideGroup(damageText->mesh, digitCount, (displayNumber == 0));

	// Hide all remaining unused character mesh groups.
	for (uint32 i = digitCount + 1; i < DAMAGETEXT_MAXCHARS; i++) {
		Gods98::Mesh_HideGroup(damageText->mesh, i, true);
	}

	damageText->charCount = digitCount + 1; // +1 for minus symbol.
}

// <LegoRR.exe @0040a670>
void __cdecl LegoRR::DamageText_MeshRenderCallback(Gods98::Mesh* mesh, void* pDamageText, Gods98::Viewport* view)
{
	DamageTextData* damageText = reinterpret_cast<DamageTextData*>(pDamageText);

	const Point2F TEXT_COORDS[4] = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
	};

	Gods98::Container* lightCont = Lego_GetCurrentViewLight();
	Gods98::Container* viewCont = Gods98::Viewport_GetCamera(view);

	Gods98::Container_SetOrientation(damageText->cont, nullptr, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

	Vector3F dir, up;
	Gods98::Container_GetOrientation(viewCont, nullptr, &dir, &up);
	Vector3F wPos;
	Gods98::Container_GetPosition(lightCont, nullptr, &wPos);

	const real32 timerDelta = (damageText->timer / DAMAGETEXT_DURATION);
	const real32 timerScalar = ((timerDelta * 2.0f) + 1.0f) * 3.0f; // Range [3.0f,9.0f] from start to finish.

	Vector3F cross;
	Gods98::Maths_Vector3DCrossProduct(&cross, &up, &dir);
	Gods98::Maths_Vector3DScale(&cross, &cross, timerScalar);

	Gods98::Maths_Vector3DScale(&up, &up, timerScalar);

	// Scalar used to shift the position of each character (starting with the right-most character).
	real32 charScalar = 1.0f - (static_cast<real32>(damageText->charCount) / 2.0f);

	for (uint32 i = 0; i < damageText->charCount; i++) {
		Vector3F vertPoses[4];

		// Use negative charScalar here, since we're doing things right to left.
		Gods98::Maths_RayEndPoint(&vertPoses[0], &damageText->offsetPos, &cross, -charScalar);
		charScalar += 1.0f; // Shift the next character.

		Gods98::Maths_Vector3DAdd(&vertPoses[1], &vertPoses[0], &cross);
		Gods98::Maths_Vector3DAdd(&vertPoses[2], &vertPoses[1], &up);
		Gods98::Maths_Vector3DAdd(&vertPoses[3], &vertPoses[0], &up);

		Gods98::Mesh_SetVertices_PointNormalAt(damageText->mesh, i, 0, 4, vertPoses, &wPos,
											   reinterpret_cast<real32(*)[2]>(const_cast<Point2F*>(TEXT_COORDS)));
	}
}

// <LegoRR.exe @0040a940>
void __cdecl LegoRR::DamageText_UpdateAll(real32 elapsedInterface)
{
	for (uint32 i = 0; i < DAMAGETEXT_MAXSHOWN; i++) {
		DamageTextData* damageText = &damageTextGlobs.table[i];
		if (damageText->flags & DAMAGETEXT_FLAG_USED) {
			DamageText_Update(damageText, elapsedInterface);
		}
	}
}

// <LegoRR.exe @0040a970>
void __cdecl LegoRR::DamageText_Update(DamageTextData* damageText, real32 elapsedInterface)
{
	if (damageText->timer < DAMAGETEXT_DURATION) {
		// Damage text is still displaying.

		// Update the 3D offset position of the damage text.
		damageText->offsetPos.x += 0.0f;
		damageText->offsetPos.y += 0.0f;
		damageText->offsetPos.z -= (elapsedInterface * 1.0f); // Move 25 units up per second.

		// Fade the text away until it's completely invisible.
		const real32 alpha = 1.0f - (damageText->timer / DAMAGETEXT_DURATION);
		for (uint32 i = 0; i < damageText->charCount; i++) {
			Gods98::Mesh_SetGroupMaterialValues(damageText->mesh, i, alpha, Gods98::Mesh_Colour::Alpha);
		}

		// Update the timer. Note that this is done after using the timer in the alpha calculation.
		damageText->timer += elapsedInterface;
	}
	else {
		// Damage text is finished displaying, remove this.
		damageText->flags &= ~DAMAGETEXT_FLAG_USED;
		Gods98::Container_Hide(damageText->cont, true);
	}
}

// <LegoRR.exe @0040aa10>
bool32 __cdecl LegoRR::DamageText_CanShow(LegoObject* liveObj)
{
	// Object cannot be dynamite or sonic blaster, and DontShowDamage should not be set.
	return (liveObj->type != LegoObject_Dynamite && liveObj->type != LegoObject_OohScary &&
			!(StatsObject_GetStatsFlags2(liveObj) & STATS2_DONTSHOWDAMAGE));
}

#pragma endregion
