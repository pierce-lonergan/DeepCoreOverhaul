// Shortcuts.hpp : Bindings for all game commands, so that the user can reconfigure them.
//                 This is not planned to be the final name, may be changed to Shortcuts or Commands.
//

#pragma once

#include "../engine/core/Config.h"
#include "../engine/core/Maths.h"
#include "../engine/core/Utils.h"
#include "../engine/input/InputButton.hpp"


namespace Shortcuts
{; // !<---

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

// Load this relative to the exeDir.
#define SHORTCUTS_FILENAME		"Settings\\Shortcuts.cfg"

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum ShortcutID
{
	None, // dummy

	// Modifier to activate interface menu actions defined in Lego.cfg.
	InterfaceActionModifier, // [F2]
	// Adds units to existing selection instead of clearing the old selection.
	AddSelectionModifier, // [LShift] | [RShift]

	// Toggles healthbars and bubbles (Object Info) above all units.
	ToggleObjectInfo, // [Space]
	// Pauses the game but does not unpause (while held).
	EscapePause, // [Esc]
	// Pauses and unpauses the game.
	TogglePause, // [P]
	
	// Begins renaming a valid selected unit.
	RenameUnit, // [Return]
	// NOT REBINDABLE: Accepts renaming selected unit.
	//AcceptRenameUnit, // [Return] | [Esc]

	// Ends message wait (triggered by a NERPs script). This state freezes the game
	// until the specified key is pressed. However, vanilla levels never use this.
	NERPsEndMessageWait, // [Return]

	// Moves forwards in any first-person view (while held).
	FPMoveForward, // [Up]
	// oves backwards in any first-person view (while held).
	FPMoveBackward, // [Down]
	// Turns left in any first-person view (while held).
	FPTurnLeft, // [Left]
	// Turns right in any first-person view (while held).
	FPTurnRight, // [Right]
	// Strafes (moves) left in any first-person view (while held).
	FPStrafeLeft, // [Z]
	// Strafes (moves) right in any first-person view (while held).
	FPStrafeRight, // [X]

	// Tilts camera upwards in topdown view (while held).
	CameraTiltUp, // [Up]
	// Tilts camera downwards in topdown view (while held).
	CameraTiltDown, // [Down]
	// Rotates the camera left (clockwise) in topdown view (while held).
	CameraTurnLeft, // [Left]
	// Rotates the camera right (counter clockwise) in topdown view (while held).
	CameraTurnRight, // [Right]
	// Zooms the camera in in topdown view (while held).
	CameraZoomIn, // [=]
	// Zooms the camera out in topdown view (while held).
	CameraZoomOut, // [-]

	// Exits the program forcefully (while held).
	Debug_HardExit, // [Esc]+[Space]
	// Exits the program naturally by cleaning up game data and then closing (while held).
	Debug_SoftExit, // [Esc]+[Return]

	// Toggles between Radar Map/Track Object View.
	SwitchRadarMode, // [Tab]
	// Toggles NoNERPs. Level scripting and win conditions will be disabled.
	Debug_ToggleNoNERPs, // [F12]
	// Disables build dependencies. Buildings and vehicles can be teleporting down without
	// meeting the requirements.
	Debug_ToggleBuildDependencies, // [F11]
	// Toggles inverted lighting. This freezes the graphics when used with dgVoodoo,
	// but pressing the key again will unfreeze it.
	Debug_ToggleInvertLighting, // [F10]
	// Toggles topdown mouse spotlight effects and flickering.
	Debug_ToggleSpotlightEffects, // [F9]

	/// NEW: Switches between debug overlays for ListSets, NERPs, or nothing.
	Debug_SwitchDebugOverlay, // [F8]

	// Toggles level fallins. These are random fallins, and not related to ones
	// specified in a fallin map.
	Debug_ToggleFallins, // [F6]
	// Toggles noclip for first-person mode.In this mode you can walk through other units and hug walls.
	Debug_ToggleFPNoClip, // [LCtrl]+[Return]

	// Destroys wall at mousepoint.
	Debug_DestroyWall, // [Numpad Dec]
	// Destroys wall connections at mousepoint (while held).
	Debug_DestroyWallConnection, // [Numpad 3]

	// Sets game speed to 300%.
	MaxGameSpeed, // [Numpad 7]
	/// NEW: Sets game speed to 100%.
	DefaultGameSpeed, // NULL
	/// NEW: Sets game speed to 33%.
	MinGameSpeed, // NULL
	/// NEW: Sets game speed to 0%.
	Debug_FreezeGameSpeed, // NULL
	// Decreases game speed by 25% (per second, while held).
	DecreaseGameSpeed, // [Numpad 8]
	// Increases game speed by 25% (per second, while held).
	IncreaseGameSpeed, // [Numpad 9]

	// Commands selected unit to drop live dynamite.
	Debug_CommandDropDynamite, // [C]
	// Registers selected vehicle for Debug_CommandGetInRegisteredVehicle.
	Debug_CommandRegisterVehicle, // [K]
	// Commands selected unit to get in registered vehicle.
	Debug_CommandGetInRegisteredVehicle, // [K]

	// (???) Related to unused Water for block at mousepoint.
	Debug_Unknown_Water, // [W]
	// (???) AI task gather action with argument 5.
	Debug_Unknown_Gather, // [Backspace]
	// (???) Electric fence action for selected building.
	Debug_Unknown_ElectricFence, // [H]

	// Stops current routing for the primary selected unit.
	Debug_StopRouting, // [N]

	// Toggles power Off/On for currently selected building.
	Debug_ToggleSelfPowered, // [End]

	// Ends current advisor animation.
	Debug_EndAdvisorAnim, // (!LShift)+[U]
	// Begins advisor animation "Advisor_Anim_Point_N".
	Debug_BeginAdvisorAnim, // [LShift]+[U]

	// Places an electric fence at mousepoint.
	Edit_PlaceElectricFence, // [J]
	// Creates a spiderweb at mousepoint.
	Edit_PlaceSpiderWeb, // [H]

	/// NEW: Places or removes a power path at mousepoint.
	Edit_PlacePath, // NULL
	/// NEW: Places one crystal at mousepoint.
	Edit_PlaceCrystal, // NULL
	/// NEW: Places one ore piece at mousepoint.
	Edit_PlaceOre, // NULL
	/// NEW: Increases the number of stored crystals by 1.
	Cheat_IncreaseCrystalsStored, // NULL
	/// NEW: Increases the number of stored ore by 1.
	Cheat_IncreaseOreStored, // NULL

	// Toggles music ON/OFF.
	ToggleMusic, // [M]
	// Toggles sound ON/OFF.
	ToggleSound, // [S]

	// Decreases oxygen level by 1% (per tick, while held).
	Debug_DecreaseOxygen, // [O]
	/// NEW: Increases oxygen level by 1% (per tick, while held).
	Debug_IncreaseOxygen, // NULL
	// Creates a landslide at mousepoint in the North direction.
	Debug_CreateLandslide, // [A]

	// Causes unit at mousepoint to slip.
	Debug_TripUnit, // [A]
	// Bumps unit at mousepoint in east-northeast direction.
	Debug_BumpUnit, // [B]
	// Damages all selected units by 40 HP.
	Debug_DamageUnit, // [F]
	/// NEW: Heals all selected units to max HP and energy.
	Cheat_HealUnit, // NULL

	// Commands any unit to get a Sonic Blaster from the Tool Store and place at mousepoint.
	Debug_CommandPlaceSonicBlaster, // [LShift]+[A]
	// Commands selected unit to eat.
	Debug_CommandEat, // [Z]

	// Shakes the camera screen for 1 second.
	Debug_ShakeScreen, // [Z]

	// Triggers CrystalFound InfoMessage at block (1,1).
	Debug_InterfaceCrystalFoundMessage, // [Y]

	// Makes a monster or slug emerge from a valid spawn wall/hole at mousepoint.
	Debug_EmergeMonster, // [E]
	// ( ! ) Commands selected monster to gather a boulder at the nearest wall.
	Debug_CommandGatherBoulder, // [W]

	// Changes to first-person Eye view for selected unit.
	ChangeViewFP1, // [1]
	// Changes to first-person Shoulder view for selected unit.
	ChangeViewFP2, // [2]
	// Changes to top-down view.
	ChangeViewTop, // [3]
	// Tracks the selected unit in the radar.
	TrackUnit, // [4]
	/// NEW: Deletes all selected units.
	Edit_DestroyUnits, // NULL
	/// NEW: Levels up selected units to max level, and gives selected mini-figures all abilities.
	Cheat_MaxOutUnit,

	// Toggles selected unit's Carry upgrade model (visual only).
	Debug_ToggleUpgradeCarry, // [5]
	// Toggles selected unit's Scan upgrade model (visual only).
	Debug_ToggleUpgradeScan,  // [6]
	// Toggles selected unit's Speed upgrade model (visual only).
	Debug_ToggleUpgradeSpeed, // [7]
	// Toggles selected unit's Drill upgrade model (visual only).
	Debug_ToggleUpgradeDrill, // [8]

	// Decreases top-down spotlight "SpotPenumbra" by 0.02 (per tick, while held).
	Debug_DecreaseSpotlightPenumbra, // (!LShift)+[5]
	// Increases top-down spotlight "SpotPenumbra" by 0.02 (per tick, while held).
	Debug_IncreaseSpotlightPenumbra, // [LShift]+[5]
	// Decreases top-down spotlight "SpotUmbra" by 0.02 (per tick, while held).
	Debug_DecreaseSpotlightUmbra, // (!LShift)+[6]
	// Increases top-down spotlight "SpotUmbra" by 0.02 (per tick, while held).
	Debug_IncreaseSpotlightUmbra, // [LShift]+[6]
	// Decreases 3D sound roll-off factor by 0.05 (per tick, while held).
	Debug_DecreaseSound3DRollOffFactor, // [9]
	// Increases 3D sound roll-off factor by 0.05 (per tick, while held).
	Debug_IncreaseSound3DRollOffFactor, // [0]

	// Toggles frame rate monitor.
	Debug_ToggleFPSMonitor, // [LCtrl]+[F]
	// Toggles memory monitor.
	Debug_ToggleMemoryMonitor, // [LCtrl]+[G]

	// Instantly wins the level and goes to rewards screen.
	Debug_WinLevelInstant, // [L]
	// Fails the level.
	Debug_LoseLevel, // [LCtrl]+[D]
	// Wins the level.
	Debug_WinLevel, // [LCtrl]+[S]
	// Fails the level with reason: "Too many crystals stolen".
	Debug_LoseLevelCrystals, // [RCtrl]+[S]
	// Toggles free unrestricted camera movement.
	Debug_ToggleFreeCameraMovement, // [Numpad 0]


	// Requires the cheat "FP Controls" to be enabled:
	/// NEW: Moves the primary selected unit forwards in topdown view (while held).
	Cheat_TopdownFPMoveForward, // [Up]
	/// NEW: Moves the primary selected unit backwards in topdown view (while held).
	Cheat_TopdownFPMoveBackward, // [Down]
	/// NEW: Turns the primary selected unit left in topdown view (while held).
	Cheat_TopdownFPTurnLeft, // [Left]
	/// NEW: Turns the primary selected unit right in topdown view (while held).
	Cheat_TopdownFPTurnRight, // [Right]
	/// NEW: Strafes (moves) the primary selected unit left in topdown view (while held).
	Cheat_TopdownFPStrafeLeft, // [Z]
	/// NEW: Strafes (moves) the primary selected unit right in topdown view (while held).
	Cheat_TopdownFPStrafeRight, // [X]

	/// NEW: Hold down while selecting units to include enemy creatures.
	Edit_SelectMonstersModifier, // [T]
	/// NEW: Hold down while selecting units to include crystals and ore.
	Edit_SelectResourcesModifier, // [R]

	/// NEW: Toggles the power of a building, or the sleeping state of a monster.
	Edit_TogglePower,
	/// NEW: Freezes unit at mousepoint in a block of ice for 10 seconds.
	Cheat_FreezeUnit,
		
	/// NEW: Spawns dynamite at mousepoint and begins the tickdown.
	Cheat_PlaceDynamite,
	/// NEW: Spawns dynamite at mousepoint that immediately explodes.
	Cheat_PlaceDynamiteInstant,
	/// NEW: Spawns a sonic blaster at mousepoint and begins the tickdown.
	Cheat_PlaceSonicBlaster,
	/// NEW: Spawns a sonic blaster at mousepoint that immediately goes off.
	Cheat_PlaceSonicBlasterInstant,
	/// NEW: Causes an explosion at the all selected units' positions and kills them in the process.
	Cheat_KamikazeUnit,

	/// NEW: Cycle the camera to the next building (or minifigure if no buildings) in the level.
	CameraCycleNextUnit,


	/// NEW: Reloads the configuration file, allowing to change command bindings on the fly.
	ReloadKeyBinds, // [Ctrl]+[Shift]+[K]


	// The number of keybind IDs (including None).
	// Define keybinds *after* this enum if they're not ready to be registered yet during ShortcutManager::Initialise().
	Count,

};

#pragma endregion

/**********************************************************************************
 ******** Classes
 **********************************************************************************/

#pragma region class ShortcutInfo 

class ShortcutInfo
{
private:
	std::string name; // String representation of this keybind (string value of shortcutID).
	ShortcutID shortcutID; // Enum representation of this keybind.
	ShortcutID fallbackID; // Fallback to whatever this keybind is assigned to.
	std::string defaultText; // When fallbackID is None, use this to construct the default keybind when not defined in a config file.
	std::shared_ptr<Gods98::InputButtonBase> button;

public:
	inline ShortcutInfo()
		: name(""), shortcutID(ShortcutID::None), fallbackID(ShortcutID::None), defaultText(""), button(std::make_shared<Gods98::InputButton>())
	{
	}

	inline ShortcutInfo(const std::string& name, ShortcutID shortcutID, ShortcutID fallbackID)
		: name(name), shortcutID(shortcutID), fallbackID(fallbackID), defaultText(""), button(std::make_shared<Gods98::InputButton>())
	{
	}

	inline ShortcutInfo(const std::string& name, ShortcutID shortcutID, const char* defaultText)
		: name(name), shortcutID(shortcutID), fallbackID(ShortcutID::None), defaultText((defaultText)?defaultText:""), button(std::make_shared<Gods98::InputButton>())
	{
	}

	inline ShortcutInfo(const std::string& name, ShortcutID shortcutID, const std::string& defaultText)
		: name(name), shortcutID(shortcutID), fallbackID(ShortcutID::None), defaultText(defaultText), button(std::make_shared<Gods98::InputButton>())
	{
	}

public:
	inline const std::string& Name() const { return name; }
	inline ShortcutID ID() const { return shortcutID; }
	inline std::string ToString() const { return "[" + name + "] = " + button->ToString(); }

	inline std::shared_ptr<Gods98::InputButtonBase> Button() { return button; }
	inline const std::shared_ptr<Gods98::InputButtonBase>& Button() const { return button; }

	friend class ShortcutManager;
};

#pragma endregion


#pragma region class ShortcutManager 

class ShortcutManager
{
private:
	std::vector<ShortcutInfo> shortcutInfos;

public:
	inline ShortcutManager() : shortcutInfos() {}

public:
	bool Initialise();

	void Shutdown();

	// Load the Shortcuts configuration file.
	bool Load();
	// Load the default Shortcuts.
	void LoadDefaults();

	// Update KeyBind states during the game loop. Returns true if ReloadKeyBinds was triggered.
	bool Update(real32 elapsed, bool checkForReload = true);

	// Individually resets/temporarily disables all keybinds.
	void Reset(bool release = false);

	// Individually re-enables all keybinds.
	void Enable();
	// Individually disables all keybinds.
	void Disable(bool untilRelease = false);

private:
	// Nullify all ShortcutInfo buttons, so that they can be reassigned during Load or AssignDefaults.
	void ClearAssignments();
	// Assign defaults to all ShortcutInfos with null buttons.
	void AssignDefaults();

	bool Parse(ShortcutInfo& shortcutInfo, const char* text);

public:
	inline size_t Count() const { return shortcutInfos.size(); }
	inline ShortcutInfo& operator[](ShortcutID id) { return shortcutInfos[static_cast<size_t>(id)]; }
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

extern ShortcutManager shortcutManager;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Shortcut_Get(id) Shortcuts::shortcutManager[id]

#define Shortcut_GetButton(id) Shortcuts::shortcutManager[id].Button()

#define Shortcut_IsDown(id) Shortcut_GetButton(id)->IsDown()

#define Shortcut_IsUp(id) Shortcut_GetButton(id)->IsUp()

#define Shortcut_IsPreviousDown(id) Shortcut_GetButton(id)->IsPreviousDown()

#define Shortcut_IsPreviousUp(id) Shortcut_GetButton(id)->IsPreviousUp()

#define Shortcut_IsPressed(id) Shortcut_GetButton(id)->IsPressed()

#define Shortcut_IsReleased(id) Shortcut_GetButton(id)->IsReleased()

#define Shortcut_IsHeld(id) Shortcut_GetButton(id)->IsHeld()

#define Shortcut_IsUntouched(id) Shortcut_GetButton(id)->IsUntouched()

#define Shortcut_IsChanged(id) Shortcut_GetButton(id)->IsChanged()

#define Shortcut_IsUnchanged(id) Shortcut_GetButton(id)->IsUnchanged()


#define Shortcut_IsDoubleClicked(id) Shortcut_GetButton(id)->IsDoubleClicked()

#define Shortcut_IsClicked(id) Shortcut_GetButton(id)->IsClicked()

#define Shortcut_IsTyped(id) Shortcut_GetButton(id)->IsTyped()


#define Shortcut_HoldTime(id) Shortcut_GetButton(id)->HoldTime()

#define Shortcut_IsEnabled(id) Shortcut_GetButton(id)->IsEnabled()

#define Shortcut_IsDisabled(id) Shortcut_GetButton(id)->IsDisabled()


// In the future, activating debug commands and cheats from the system menu can be done with this.
#define Shortcut_Activate(id, immediate) Shortcut_GetButton(id)->Activate(immediate)

#define Shortcut_Enable(id) Shortcut_GetButton(id)->Enable()

#define Shortcut_Disable(id, untilRelease) Shortcut_GetButton(id)->Disable(untilRelease)

#define Shortcut_Reset(id, release) Shortcut_GetButton(id)->Reset(release)

#pragma endregion

}
