// Panels.cpp : 
//

#include "../Game.h"

#include "Panels.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @005010e0>
LegoRR::Panel_Globs & LegoRR::panelGlobs = *(LegoRR::Panel_Globs*)0x005010e0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Hook function to overwrite Panel maximum radarmap zoom level.
void LegoRR::Panel_RadarMap_ZoomIn(real32 elapsedInterface)
{
	const real32 maxZoom = 30.0f; // OLD: 20.0f;
	legoGlobs.radarZoom = std::min(maxZoom, (legoGlobs.radarZoom + elapsedInterface));
}

/// CUSTOM: Hook function to overwrite Panel minimum radarmap zoom level.
void LegoRR::Panel_RadarMap_ZoomOut(real32 elapsedInterface)
{
	const real32 minZoom = 4.0f; // OLD: 10.0f;
	legoGlobs.radarZoom = std::max(minZoom, (legoGlobs.radarZoom - elapsedInterface));
}



// <LegoRR.exe @0045a2f0>
//void __cdecl LegoRR::Panel_Initialise(void);

// <LegoRR.exe @0045a500>
//void __cdecl LegoRR::Panel_LoadInterfaceButtons_ScrollInfo(void);

// <LegoRR.exe @0045a530>
//void __cdecl LegoRR::Panel_ResetAll(void);

// <LegoRR.exe @0045a5a0>
//void __cdecl LegoRR::Panel_LoadImage(const char* filename, Panel_Type panelType, PanelDataFlags flags);

// <LegoRR.exe @0045a630>
//bool32 __cdecl LegoRR::Panel_GetPanelType(const char* panelName, OUT Panel_Type* panelType);

// <LegoRR.exe @0045a670>
//bool32 __cdecl LegoRR::Panel_TestPointInsideImage(Panel_Type panelType, sint32 screenX, sint32 screenY);

// <LegoRR.exe @0045a6d0>
//bool32 __cdecl LegoRR::Panel_GetButtonType(Panel_Type panelType, const char* buttonName, OUT PanelButton_Type* buttonType);

// <LegoRR.exe @0045a720>
//uint32 __cdecl LegoRR::Panel_PrintF(Panel_Type panelType, Gods98::Font* font, sint32 x, sint32 y, bool32 center, const char* msg, ...);

// <LegoRR.exe @0045a7c0>
//LegoRR::PanelTextWindow* __cdecl LegoRR::Panel_TextWindow_Create(Panel_Type panelType, Gods98::Font* font, const Area2F* rect, uint32 size);

// <LegoRR.exe @0045a850>
//void __cdecl LegoRR::Panel_TextWindow_PrintF(PanelTextWindow* panelWnd, const char* msg, ...);

// <LegoRR.exe @0045a870>
//void __cdecl LegoRR::Panel_TextWindow_Update(PanelTextWindow* textWnd, uint32 posFromEnd, real32 elapsed);

// <LegoRR.exe @0045a8e0>
//void __cdecl LegoRR::Panel_TextWindow_Clear(PanelTextWindow* panelWnd);

// <LegoRR.exe @0045a8f0>
//void __cdecl LegoRR::Panel_TextWindow_GetInfo(PanelTextWindow* panelWnd, OUT uint32* linesCount, OUT uint32* linesCapacity);

// <LegoRR.exe @0045a910>
//void __cdecl LegoRR::Panel_SetSlidingPositions(Panel_Type panelType, sint32 xOpen, sint32 yOpen, sint32 xClosed, sint32 yClosed);

// <LegoRR.exe @0045a9a0>
//void __cdecl LegoRR::Panel_SetPosition(Panel_Type panelType, real32 x, real32 y);

// <LegoRR.exe @0045a9c0>
//void __cdecl LegoRR::Panel_GetPosition(Panel_Type panelType, OUT real32* x, OUT real32* y);

// <LegoRR.exe @0045a9f0>
//void __cdecl LegoRR::Panel_Draw(Panel_Type panelType, real32 elapsedAbs);

// <LegoRR.exe @0045ac80>
//void __cdecl LegoRR::Panel_DrawButtons(Panel_Type panelType);

// <LegoRR.exe @0045ad80>
//void __cdecl LegoRR::Panel_Button_Hide(Panel_Type panelType, PanelButton_Type buttonType, bool32 hide);

// <LegoRR.exe @0045adc0>
//void __cdecl LegoRR::Panel_ToggleOpenClosed(Panel_Type panelType);

// <LegoRR.exe @0045adf0>
//bool32 __cdecl LegoRR::Panel_IsFullyClosed(Panel_Type panelType);

// <LegoRR.exe @0045ae20>
//bool32 __cdecl LegoRR::Panel_IsFullyOpen(Panel_Type panelType);

// <LegoRR.exe @0045ae50>
//bool32 __cdecl LegoRR::Panel_IsSliding(Panel_Type panelType);

// <LegoRR.exe @0045ae70>
//void __cdecl LegoRR::Panel_Button_SetToggleable(Panel_Type panelType, PanelButton_Type buttonType, bool32 toggleable);

// <LegoRR.exe @0045aeb0>
//void __cdecl LegoRR::Panel_Button_SetToggled(Panel_Type panelType, PanelButton_Type buttonType, bool32 toggled);

// <LegoRR.exe @0045aef0>
//void __cdecl LegoRR::Panel_CreateButtons(Panel_Type panelType, uint32 count, const PanelButton_Type* buttonTypes, const Area2F* areas, char** imageNames, char** imageHoverNames, char** imagePressedNames, const ToolTip_Type* toolTips);

// <LegoRR.exe @0045b070>
//bool32 __cdecl LegoRR::Panel_CheckCollision(real32 elapsedAbs, uint32 mouseX, uint32 mouseY, bool32 leftButton, bool32 leftButtonLast, OUT bool32* panelCollision);

// <LegoRR.exe @0045b5d0>
//bool32 __cdecl LegoRR::Panel_CheckButtonCollision(OUT Panel_Type* panelType, OUT PanelButton_Type* panelButton, sint32 mouseX, sint32 mouseY, bool32 leftButton, bool32 leftButtonLast, OPTIONAL OUT bool32* param_7, OPTIONAL OUT bool32* panelCollision);

// <LegoRR.exe @0045b850>
//bool32 __cdecl LegoRR::Panel_TestPointInsideRect(PanelData* panel, const Rect2F* rect, sint32 mouseX, sint32 mouseY);

// <LegoRR.exe @0045b8d0>
//void __cdecl LegoRR::Panel_PriorityList_ResetButtons(void);

// <LegoRR.exe @0045b8e0>
//void __cdecl LegoRR::Panel_PriorityList_HandleButton(PanelButton_Type buttonType);

// <LegoRR.exe @0045ba00>
//void __cdecl LegoRR::Panel_ScrollInfo_Initialise(void);

// <LegoRR.exe @0045bb10>
//void __cdecl LegoRR::Panel_Encyclopedia_Initialise(void);

// <LegoRR.exe @0045bb60>
//bool32 __cdecl LegoRR::Panel_TestPointInsideCircle(sint32 x, sint32 y, sint32 centerX, sint32 centerY, sint32 radius);

// <LegoRR.exe @0045bbc0>
//void __cdecl LegoRR::Panel_RotationControl_NormalizePointRadius(sint32 x, sint32 y, sint32 radius, OUT real32* out_x, OUT real32* out_y);

// <LegoRR.exe @0045bbf0>
//void __cdecl LegoRR::Panel_RotationControl_ClampPointInsideCircle(IN OUT sint32* mouseX, IN OUT sint32* mouseY, sint32 centerX, sint32 centerY, sint32 radius);

// <LegoRR.exe @0045bc90>
//void __cdecl LegoRR::Panel_RotationControl_Initialise(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0045bf90>
//bool32 __cdecl LegoRR::Panel_RotationControl_HandleRotation(sint32 mouseX, sint32 mouseY, real32 elapsedAbs);

// <LegoRR.exe @0045c1e0>
//void __cdecl LegoRR::Panel_RotationControl_HandleButtons(PanelButton_Type buttonType, real32 elapsedAbs);

// <LegoRR.exe @0045c230>
//void __cdecl LegoRR::Panel_Button_GetArea(Panel_Type panelType, PanelButton_Type buttonType, OUT Area2F* area);

// <LegoRR.exe @0045c270>
//void __cdecl LegoRR::Panel_Crystals_Initialise(const char* smallCrystal, const char* usedCrystal, const char* noSmallCrystal);

// <LegoRR.exe @0045c300>
//void __cdecl LegoRR::Panel_Crystals_LoadRewardQuota(const Gods98::Config* config, const char* gameName, const char* levelName);

// <LegoRR.exe @0045c390>
//void __cdecl LegoRR::Panel_Crystals_Draw(uint32 crystals, uint32 usedCrystals, real32 elapsedGame_unused);

// <LegoRR.exe @0045c600>
//void __cdecl LegoRR::Panel_AirMeter_Initialise(const char* airJuiceName, uint32 juiceX, uint32 juiceY, uint32 juiceLength, const char* noAirName, uint32 noAirX, uint32 noAirY);

// <LegoRR.exe @0045c6b0>
//void __cdecl LegoRR::Panel_AirMeter_DrawJuice(Panel_Type panelType, real32 oxygen);

// <LegoRR.exe @0045c760>
//void __cdecl LegoRR::Panel_AirMeter_SetOxygenLow(bool32 o2Low);

// <LegoRR.exe @0045c770>
//void __cdecl LegoRR::Panel_AirMeter_DrawOxygenLow(Panel_Type panelType);

// <LegoRR.exe @0045c7e0>
//void __cdecl LegoRR::Panel_CryOreSideBar_Initialise(const char* sidebarName, uint32 xPos, uint32 yPos, uint32 meterOffset);

// <LegoRR.exe @0045c840>
//void __cdecl LegoRR::Panel_CryOreSideBar_ChangeOreMeter(bool32 increment, uint32 amount);

// <LegoRR.exe @0045c8b0>
//void __cdecl LegoRR::Panel_CryOreSideBar_Draw(void);

// <LegoRR.exe @0045c930>
//bool32 __cdecl LegoRR::Panel_SetCurrentAdvisorFromButton(Panel_Type panelType, PanelButton_Type buttonType, bool32 setFlag2);

// <LegoRR.exe @0045c970>
//bool32 __cdecl LegoRR::Panel_GetAdvisorTypeFromButton(Panel_Type panelType, PanelButton_Type buttonType, OUT Advisor_Type* advisorType);

#pragma endregion
