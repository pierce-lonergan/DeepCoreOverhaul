// Interface.cpp : 
//

#include "../../engine/drawing/Draw.h"
#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "../Game.h"
#include "../Shortcuts.hpp"

#include "Interface.h"


using Gods98::Keys;
using Shortcuts::ShortcutID;


/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define INTERFACE_PRERENDER_HOVEROUTLINES		true

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004a3b58>
real32 & LegoRR::g_Interface_TIMER_004a3b58 = *(real32*)0x004a3b58;

// <LegoRR.exe @004a3b5c>
uint32 & LegoRR::g_Interface_UINT_004a3b5c = *(uint32*)0x004a3b5c;

// used by: Interface_FUN_0041ebd0
// <LegoRR.exe @004ddd50>
LegoRR::LegoObject_ID & LegoRR::s_interfaceObjID = *(LegoRR::LegoObject_ID*)0x004ddd50;

// <LegoRR.exe @004ddd58>
LegoRR::Interface_Globs & LegoRR::interfaceGlobs = *(LegoRR::Interface_Globs*)0x004ddd58;


/// CUSTOM: Avoid excess surface locking by pre-rendering highlight outlines on demand.
static std::map<std::pair<uint32, uint32>, Gods98::Image*> _interfaceHoverOutlinesCache;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0041a220>
//Gods98::Font* __cdecl LegoRR::Interface_GetFont(void);

// <LegoRR.exe @0041a230>
//void __cdecl LegoRR::Interface_Initialise(uint32 x_565, uint32 y_18, Gods98::Font* font);

// <LegoRR.exe @0041a590>
//void __cdecl LegoRR::Interface_AddAllMenuItems(void);

// <LegoRR.exe @0041a700>
//void __cdecl LegoRR::Interface_AddMenuItems(Interface_MenuType menuType, uint32 numItems, ...);

// <LegoRR.exe @0041a780>
//void __cdecl LegoRR::Interface_Shutdown(void);

// <LegoRR.exe @0041a850>
//void __cdecl LegoRR::Interface_ClearStates(void);

// <LegoRR.exe @0041a8c0>
//void __cdecl LegoRR::Interface_ResetMenu(void);

// <LegoRR.exe @0041a8f0>
//bool32 __cdecl LegoRR::Interface_GetMenuItemType(const char* menuItemName, OUT Interface_MenuItemType* menuItemType);

// <LegoRR.exe @0041a930>
//void __cdecl LegoRR::Interface_InitBuildItems(void);

// <LegoRR.exe @0041aa30>
//void __cdecl LegoRR::Interface_LoadBuildItems(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0041acd0>
//void __cdecl LegoRR::Interface_LoadMenuItems(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0041af00>
//void __cdecl LegoRR::Interface_LoadItemPanels(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0041b0e0>
//void __cdecl LegoRR::Interface_LoadBackButton(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0041b1a0>
//void __cdecl LegoRR::Interface_LoadPlusMinusImages(const char* plusName, const char* minusName);

// <LegoRR.exe @0041b200>
//void __cdecl LegoRR::Interface_OpenMenu(Interface_MenuType menuType, const Point2I* blockPos);

// <LegoRR.exe @0041b230>
//void __cdecl LegoRR::_Interface_OpenMenu_SelectBlock(Interface_MenuType menuType, const Point2I* blockPos);

// <LegoRR.exe @0041b2f0>
//sint32 __cdecl LegoRR::Interface_FUN_0041b2f0(Interface_MenuType interfaceMenuType);

// <LegoRR.exe @0041b3a0>
//bool32 __cdecl LegoRR::Interface_MenuTypeHasBackButton(Interface_MenuType interfaceMenuType);

// <LegoRR.exe @0041b3c0>
//void __cdecl LegoRR::Interface_FUN_0041b3c0(void);

// <LegoRR.exe @0041b5b0>
//bool32 __cdecl LegoRR::Interface_FUN_0041b5b0(Interface_MenuType menuIcon, void* callback, void* context);

// <LegoRR.exe @0041b730>
//bool32 __cdecl LegoRR::Interface_Callback_FUN_0041b730(Interface_MenuItemType menuIcon, uint32 param_2, sint32 param_3, real32* param_4);

// <LegoRR.exe @0041b860>
//void __cdecl LegoRR::Interface_FUN_0041b860(real32 elapsedAbs);

// <LegoRR.exe @0041b8e0>
//bool32 __cdecl LegoRR::Interface_GetFlashingState(void);

// <LegoRR.exe @0041b8f0>
//LegoRR::Interface_MenuItemType __cdecl LegoRR::Interface_GetPrimaryUnit_PowerIcon(IN OUT Interface_MenuItemType* menuIcon);

// <LegoRR.exe @0041b940>
//void __cdecl LegoRR::Interface_FUN_0041b940(real32 elapsedAbs);

// <LegoRR.exe @0041b9c0>
//void __cdecl LegoRR::Interface_SetSFX_004df1f4(SFX_ID sfxType);

// <LegoRR.exe @0041b9d0>
//bool32 __cdecl LegoRR::Interface_DoSomethingWithRenameReplace(uint32 param_1, uint32 param_2, sint32 param_3, sint32 param_4, sint32 param_5);

// <LegoRR.exe @0041c0f0>
//bool32 __cdecl LegoRR::Interface_FUN_0041c0f0(uint32 param_1, uint32 param_2, OUT undefined4* param_3, OUT undefined4* param_4, OUT undefined4* param_5);

// <LegoRR.exe @0041c240>
//bool32 __cdecl LegoRR::Interface_Callback_FUN_0041c240(Interface_MenuItemType menuIcon, LegoObject_Type objType, LegoObject_ID objID, uint32* param_4);

// <LegoRR.exe @0041c370>
void __cdecl LegoRR::Interface_DoF2InterfaceKeyAction(void)
{
	/// KEYBIND: [F2]+(any)  "F2 configurable interface actions."
	if ((interfaceGlobs.flags & INTERFACE_GLOB_FLAG_UNK_4) &&
		(interfaceGlobs.flags & INTERFACE_GLOB_FLAG_UNK_80) &&
		Shortcut_IsDown(ShortcutID::InterfaceActionModifier))
	{
		Interface_FUN_0041b5b0(interfaceGlobs.currMenuType, Interface_CallbackDoMenuIconKeyAction, nullptr);
	}
}

// <LegoRR.exe @0041c3a0>
bool32 __cdecl LegoRR::Interface_CallbackDoMenuIconKeyAction(Interface_MenuItemType menuIcon, LegoObject_Type objType, LegoObject_ID objID)
{
	Keys keyID = Keys::KEY_NONE;
	if (menuIcon == Interface_MenuItem_Build) {
		if (objType == LegoObject_Vehicle) {
			keyID = interfaceGlobs.vehicleItemF2Keys[objID];
		}
		else if (objType == LegoObject_Building) {
			keyID = interfaceGlobs.buildingItemF2Keys[objID];
		}
	}
	else {
		keyID = interfaceGlobs.menuItemF2keys[menuIcon];
	}

	if (Input_IsKeyPressed(keyID)) {
		Interface_SetDat_004decd8_004decdc(menuIcon, objType, objID);
		Interface_DoAction_FUN_0041dbd0(menuIcon);

		// Prevent other debug shortcuts from triggering.
		Shortcuts::shortcutManager.Disable(true);
		// Re-activate the interface modifier shortcut.
		Shortcut_Activate(ShortcutID::InterfaceActionModifier, true);
	}
	return false;
}

// <LegoRR.exe @0041c420>
//void __cdecl LegoRR::Interface_FUN_0041c420(Interface_MenuItemType menuIcon, LegoObject_Type objType, LegoObject_ID objID, real32* param_4);

// <LegoRR.exe @0041c610>
//bool32 __cdecl LegoRR::Interface_FUN_0041c610(Interface_MenuItemType menuIcon, sint32 param_2, sint32 param_3, bool32 param_4, bool32 param_5);

// <LegoRR.exe @0041c6e0>
//void __cdecl LegoRR::Interface_FUN_0041c6e0(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041c730>
//void __cdecl LegoRR::Interface_FUN_0041c730(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041c820>
//bool32 __cdecl LegoRR::Interface_FUN_0041c820(Interface_MenuItemType menuIcon, bool32 param_2);

// <LegoRR.exe @0041c880>
//bool32 __cdecl LegoRR::Interface_FUN_0041c880(LegoObject_Type objType, LegoObject_ID objID, bool32 param_3);

// <LegoRR.exe @0041c920>
//void __cdecl LegoRR::Interface_FUN_0041c920(Interface_MenuItemType menuIcon, bool32 setFlag8);

// <LegoRR.exe @0041c950>
//void __cdecl LegoRR::Interface_FUN_0041c950(LegoObject_Type objType, LegoObject_ID objID, bool32 param_3);

// <LegoRR.exe @0041c990>
//bool32 __cdecl LegoRR::Interface_FUN_0041c990(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041c9a0>
//bool32 __cdecl LegoRR::Interface_FUN_0041c9a0(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041c9e0>
//Gods98::Image* __cdecl LegoRR::Interface_FUN_0041c9e0(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041ca20>
//Gods98::Image* __cdecl LegoRR::Interface_GetBuildImageByObjectType(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041cac0>
//Gods98::Image* __cdecl LegoRR::Interface_GetObjectBuildImage(LegoObject_Type objType, LegoObject_ID objID, bool32 param_3);

// <LegoRR.exe @0041cbb0>
//void __cdecl LegoRR::Interface_SetDat_004decd8_004decdc(Interface_MenuItemType menuIcon, LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041cc10>
//void __cdecl LegoRR::Interface_FUN_0041cc10(const Point2F* point, uint32 unkWidth, uint32 unkHeight);

// <LegoRR.exe @0041cc60>
void __cdecl LegoRR::Interface_DrawHoverOutline(const Area2F* area)
{
	if (!(legoGlobs.flags1 & GAME1_FREEZEINTERFACE)) {
		/// OPTIMISE: Draw pre-rendered hover outlines to avoid locking the back surface every frame.
		static constexpr const uint32 THICKNESS = 2;

		const ColourRGBF colour = { 0.0f, 1.0f, 0.0f };

		const Point2F destPos = {
			area->x - THICKNESS,
			area->y - THICKNESS,
		};

		Gods98::Image* prerendered = nullptr;

		// Note: Change define to false to disable pre-rendered drawing.
		#if INTERFACE_PRERENDER_HOVEROUTLINES
		/// NOTE: +1 is added because of how the line drawing was setup, the area dimensions are actually 1 less than the intended hover outline dimensions.
		const auto sizePair = std::make_pair(static_cast<uint32>(std::max(0.0f, area->width))  + THICKNESS * 2 + 1,
											 static_cast<uint32>(std::max(0.0f, area->height)) + THICKNESS * 2 + 1);

		auto it = _interfaceHoverOutlinesCache.find(sizePair);
		if (it != _interfaceHoverOutlinesCache.end()) {
			// We've already pre-rendered an image for this size, use that to avoid needlessly locking a surface.
			prerendered = it->second;
			Gods98::Image_Display(prerendered, &destPos);
			return;
		}

		// We haven't pre-rendered an outline for this size yet, generate it now.
		prerendered = Gods98::Image_CreateNew(sizePair.first, sizePair.second);
		ColourRGBF trans;
		Gods98::Image_FindTransColour(prerendered, &colour, 1, &trans);
		Gods98::Image_SetupTrans2(prerendered, trans.r, trans.g, trans.b,
								  trans.r, trans.g, trans.b, false); // Don't truncate to 16-bit colour.

		_interfaceHoverOutlinesCache[sizePair] = prerendered;
		#endif

		Gods98::Draw_Begin(prerendered);
		Error_Warn(!Gods98::Draw_IsLocked(), "Interface_DrawHoverOutline: Draw_Begin() failed to lock the surface");
		if (prerendered != nullptr) {
			Gods98::Draw_SetTranslation(Point2F { -destPos.x, -destPos.y }); // Translate to 0,0.
		}

		Point2F start = {
			area->x,
			area->y,
		};
		Point2F end = {
			(area->x + area->width),
			(area->y + area->height),
		};

		// Draw for rect expanded by 1 pixel.
		// Then draw for rect expanded by 2 pixels, for a 2-pixel-width rectangle outline.

		#if true
		/// REFACTOR: Draw all sets of lines in one Draw call instead of two.

		Point2F rectLinesFrom[THICKNESS*4] = { 0.0f }; // dummy inits
		Point2F rectLinesTo[THICKNESS*4] = { 0.0f };
		
		for (uint32 i = 0; i < THICKNESS; i++) {
			start.x -= 1.0f;
			start.y -= 1.0f;
			end.x   += 1.0f;
			end.y   += 1.0f;

			// 0 ____ 1
			//  |    |
			//  |____|
			// 3      2

			rectLinesFrom[(i*4) + 0] = Point2F { start.x, start.y };
			rectLinesFrom[(i*4) + 1] = Point2F {   end.x, start.y };
			rectLinesFrom[(i*4) + 2] = Point2F {   end.x,   end.y };
			rectLinesFrom[(i*4) + 3] = Point2F { start.x,   end.y };
			for (uint32 j = 0; j < 4; j++) {
				rectLinesTo[(i*4) + j] = rectLinesFrom[(i*4) + ((j+1)%4)];
			}
		}
		Gods98::Draw_LineListEx(rectLinesFrom, rectLinesTo, THICKNESS*4, colour.r, colour.g, colour.b, Gods98::DrawEffect::None);

		#else
		Point2F rectLines[5] = { 0.0f }; // dummy init

		for (uint32 i = 0; i < 2; i++) {
			start.x -= 1.0f;
			start.y -= 1.0f;
			end.x   += 2.0f;
			end.y   += 2.0f;

			// 0 ____ 1
			//  |    |
			//  |____|
			// 3      2

			rectLines[0] = Point2F { start.x, start.y };
			rectLines[1] = Point2F {   end.x, start.y };
			rectLines[2] = Point2F {   end.x,   end.y };
			rectLines[3] = Point2F { start.x,   end.y };
			rectLines[4] = rectLines[0];

			Gods98::Draw_LineListEx(rectLines, rectLines + 1, 4, colour.r, colour.g, colour.b, Gods98::DrawEffect::None);
		}
		#endif

		Gods98::Draw_End();

		if (prerendered != nullptr) {
			Gods98::Image_Display(prerendered, &destPos);
		}
	}
}

// <LegoRR.exe @0041cdd0>
//void __cdecl LegoRR::Interface_ChangeMenu_IfVehicleMounted_IsLiveObject(LegoObject* liveObj);

// <LegoRR.exe @0041ce50>
//void __cdecl LegoRR::Interface_ChangeMenu_IfPrimarySelectedVehicle_IsLiveObject(LegoObject* liveObj);

// <LegoRR.exe @0041ceb0>
//void __cdecl LegoRR::Interface_BackToMain_IfSelectedWall_IsBlockPos(const Point2I* blockPos);

// <LegoRR.exe @0041cee0>
//void __cdecl LegoRR::Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(const Point2I* blockPos);

// <LegoRR.exe @0041cf10>
//void __cdecl LegoRR::Interface_BackToMain_IfSelectedRubble_IsBlockPos(const Point2I* blockPos);

// <LegoRR.exe @0041cf40>
//void __cdecl LegoRR::Interface_BackToMain_IfUnitIsSelected(LegoObject* liveObj);

// <LegoRR.exe @0041cf70>
//bool32 __cdecl LegoRR::Interface_HandleMenuItem(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041dbd0>
//bool32 __cdecl LegoRR::Interface_DoAction_FUN_0041dbd0(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041e680>
//void __cdecl LegoRR::Interface_BackToMain(void);

// <LegoRR.exe @0041e6a0>
//void __cdecl LegoRR::Interface_SetIconFlash(Interface_MenuItemType menuIcon, bool32 flash);

// <LegoRR.exe @0041e6d0>
//void __cdecl LegoRR::Interface_SetSubmenuIconFlash(LegoObject_Type objType, LegoObject_ID objID, bool32 flash);

// <LegoRR.exe @0041e710>
//LegoRR::LegoObject* __cdecl LegoRR::Interface_GetPrimarySelectedUnit(void);

// <LegoRR.exe @0041e720>
//sint32 __cdecl LegoRR::Interface_SetIconClicked(Interface_MenuItemType menuIcon, sint32 clickedCount);

// <LegoRR.exe @0041e740>
//sint32 __cdecl LegoRR::Interface_GetIconClicked(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041e750>
//sint32 __cdecl LegoRR::Interface_SetSubmenuIconClicked(LegoObject_Type objType, LegoObject_ID objID, sint32 clickedCount);

// <LegoRR.exe @0041e790>
//sint32 __cdecl LegoRR::Interface_GetSubmenuIconClicked(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041e7c0>
//bool32 __cdecl LegoRR::Interface_SetAdvisorPointToFashingIcon(Interface_MenuItemType menuIcon, bool32 setFlag40);

// <LegoRR.exe @0041e800>
//LegoRR::Advisor_Type __cdecl LegoRR::Interface_GetAdvisorType_FromIcon(Interface_MenuItemType menuIcon);

// <LegoRR.exe @0041e8c0>
//bool32 __cdecl LegoRR::Interface_GetObjectBool_FUN_0041e8c0(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041e900>
//void __cdecl LegoRR::Interface_SetScrollParameters(uint32 xEnd, uint32 yEnd, uint32 xStart, uint32 yStart, real32 timerFloat_750);

// <LegoRR.exe @0041e980>
//void __cdecl LegoRR::_Interface_OpenMenu_SetNext(Interface_MenuType menuType, OPTIONAL const Point2I* blockPos);

// <LegoRR.exe @0041e9f0>
//void __cdecl LegoRR::Interface_FUN_0041e9f0(real32 elapsedAbs);

// <LegoRR.exe @0041eb60>
//void __cdecl LegoRR::Interface_DrawTeleportQueueNumber(LegoObject_Type objType, LegoObject_ID objID, const Point2F* screenPt);

// <LegoRR.exe @0041ebd0>
//void __cdecl LegoRR::Interface_FUN_0041ebd0(real32 xScreen, real32 yScreen);

// <LegoRR.exe @0041ed90>
//void __cdecl LegoRR::Interface_SetFloatTo25_004df1ec_AndUnsetFlags800_004df1f8(void);

// <LegoRR.exe @0041edb0>
//bool32 __cdecl LegoRR::Interface_FUN_0041edb0(uint32 param_1, uint32 param_2, real32 x, real32 y, OUT LegoObject_Type* objType, OUT LegoObject_ID* objID, OUT uint32* objLevel);

// <LegoRR.exe @0041f030>
//bool32 __cdecl LegoRR::Interface_HasTeleporterForObject(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041f0c0>
//bool32 __cdecl LegoRR::Interface_Callback_HasObject(LegoObject* liveObj, void* search);

// <LegoRR.exe @0041f160>
//bool32 __cdecl LegoRR::Interface_HasUpgradeStation(void);

// <LegoRR.exe @0041f1a0>
//bool32 __cdecl LegoRR::Interface_HasStatsFlags2(StatsFlags2 statsFlags2);

// <LegoRR.exe @0041f1e0>
//bool32 __cdecl LegoRR::Interface_HasToolStore(void);

// <LegoRR.exe @0041f220>
//bool32 __cdecl LegoRR::Interface_HasObjectOfTypeID(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0041f270>
//bool32 __cdecl LegoRR::Interface_HasObjectWithAbilities(LegoObject_AbilityFlags abilityFlags);

// <LegoRR.exe @0041f2c0>
//bool32 __cdecl LegoRR::Interface_Callback_ReqestDigBlock(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @0041f2f0>
//bool32 __cdecl LegoRR::Interface_ReqestDigBlock(const Point2I* blockPos);

// <LegoRR.exe @0041f310>
//bool32 __cdecl LegoRR::Interface_Callback_RequestReinforceBlock(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @0041f330>
//bool32 __cdecl LegoRR::Interface_RequestReinforceBlock(const Point2I* blockPos);

// <LegoRR.exe @0041f350>
//bool32 __cdecl LegoRR::Interface_DoSelectedUnits_Callback(LegoObject_RunThroughListsCallback callback, void* data);

// <LegoRR.exe @0041f3a0>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_IsCarryingButNotStoring(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f3c0>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_IsEnergyBelowMax(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f3e0>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_IsHealthBelowMax(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f400>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_FUN_0041f400(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f520>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_SetFlags4_8_HealthM1(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f540>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_GoEatIfEnergyBelowMax(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f570>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_RequestRepairIfHealthBelowMax(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f5a0>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_DoesNotHaveToolEquipped(LegoObject* liveObj, LegoObject_ToolType toolType);

// <LegoRR.exe @0041f5c0>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_GoGetToolIfNotEquipped(LegoObject* liveObj, LegoObject_ToolType toolType);

// <LegoRR.exe @0041f5f0>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_FUN_0041f5f0(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0041f650>
//bool32 __cdecl LegoRR::Interface_CheckPrimaryUnitHasAbility(LegoObject_AbilityFlags abilityFlag);

// <LegoRR.exe @0041f670>
//bool32 __cdecl LegoRR::Interface_Block_FUN_0041f670(const Point2I* blockPos);

// <LegoRR.exe @0041f750>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_HasToolEquipped_2(LegoObject* liveObj, LegoObject_ToolType toolType);

// <LegoRR.exe @0041f770>
//bool32 __cdecl LegoRR::Interface_ObjectCallback_PlaceBirdScarerIfEquipped(LegoObject* liveObj, void* unused);

#pragma endregion
