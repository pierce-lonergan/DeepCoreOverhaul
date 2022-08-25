// InputButton.hpp : Custom input button state handling classes for the engine.
//

#pragma once

#include "../../common.h"
#include "../geometry.h"

#include "Input.h"
#include "Keys.h"
#include "MouseButtons.h"



namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

// The pressed state of an InputButtonControl class.
enum class InputState
{
	Up,       // Current: up,   Previous: up
	Released, // Current: up,   Previous: down
	Down,     // Current: down, Previous: down
	Pressed,  // Current: down, Previous: up
};


// How an InputButtonControl class can be disabled.
enum class InputDisableState
{
	Enabled,              // The button is enabled.
	Activate,   // The button is enabled, and will be forced into the pressed state next tick.
	Disabled,             // The button must be explicitly re-enabled.
	DisabledUntilRelease, // The button is pressed down, once its released then it will be re-enabled.
};


// The type of input code used by an InputButton class.
enum class InputCodeType
{
	None,          // Invalid.
	Key,           // A keyboard key.
	MouseButton,   // A mouse button.
	//GamePadButton, // A button on a controller.
};


// The type of an InputButtonBase class.
enum class InputButtonType
{
	None,   // Invalid.
	Source, // An input button source.
	Multi,  // A group of buttons that can all activate.
	Combo,  // A combination of buttons needed to activate.
};


// The required order for button presses in a ComboInputButton class.
enum class InputComboOrder
{
	Any,   // Any button can be pressed in any order.
	Final, // Only the final button needs to be pressed last.
	All,   // All buttons need to be pressed in order.
};

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

#pragma endregion

/**********************************************************************************
 ******** Classes
 **********************************************************************************/

#pragma region Classes

// A basic control for an on-off state (button) input.
class InputButtonControl
{
private:
	static real32 _doubleClickTime;   // = 0.150f;
	static real32 _typingRepeatDelay; // = 0.500f;
	static real32 _typingRepeatTime;  // = 0.125f;
public:
	// Gets the time before a double click opportunity expires (in seconds).
	static real32 GetDoubleClickTime();
	// Sets the time before a double click opportunity expires (in seconds).
	static void SetDoubleClickTime(real32 time);

	// Gets the initial delay before a held key repeats typing inputs (in seconds).
	static real32 GetTypingRepeatDelay();
	// Sets the initial delay before a held key repeats typing inputs (in seconds).
	static void SetTypingRepeatDelay(real32 delay);

	// Gets the time between repeated typing inputs (in seconds).
	static real32 GetTypingRepeatTime();
	// Sets the time between repeated typing inputs (in seconds).
	static void SetTypingRepeatTime(real32 time);

protected:
	// The pressed state of the control.
	InputState m_state;
	// The disabled state of the control.
	InputDisableState m_disabledState;
	// True if the control was double clicked.
	bool m_isDoubleClicked;
	// True if the control was clicked.
	bool m_isClicked;
	// True if the control was typed.
	bool m_isTyped;
	// The time the key has been held for since being pressed, or released (in standard framerate units).
	real32 m_holdTime;
	// Countdown timer for repeated typing input.
	real32 m_repeatTimer;

public:
	inline InputButtonControl()
		: m_state(InputState::Up), m_disabledState(InputDisableState::Enabled),
		m_isDoubleClicked(false), m_isClicked(false), m_isTyped(false),
		m_holdTime(0.0f), m_repeatTimer(-1.0f)
	{
	}

protected:
	void UpdateState(real32 elapsed, bool down, bool typed = false, bool doubleClicked = false, bool clicked = false);

public:
	// Not indended for use yet. Kind of broken/jank.
	virtual void CopyStateFrom(const InputButtonControl* src);

public:
	// Activates a keybinds and treats its next UpdateState call as being down.
	// If immediate is true, then the state will immediately be updated (rather than waiting till next Update).
	virtual void Activate(bool immediate = false);

	virtual void Reset(bool release = false);

	virtual void Enable();

	virtual void Disable(bool untilRelease = false);

	inline InputState State() const { return m_state; }

	inline InputDisableState DisableState() const { return m_disabledState; }

	inline real32 HoldTime() const { return m_holdTime; }

	inline bool IsEnabled()  const { return m_disabledState == InputDisableState::Enabled || m_disabledState == InputDisableState::Activate; }

	inline bool IsDisabled() const { return m_disabledState != InputDisableState::Enabled && m_disabledState != InputDisableState::Activate; }

	// Evaluates the previous 'down' state of the input control.
	inline bool GetPreviousState() const { return (m_state == InputState::Down || m_state == InputState::Released); }
	// Evaluates the current 'down' state of the input control.
	inline bool GetCurrentState()  const { return (m_state == InputState::Down || m_state == InputState::Pressed); }


	inline bool IsDoubleClicked()   const { return IsEnabled() && m_isDoubleClicked; }

	inline bool IsClicked()         const { return IsEnabled() && m_isClicked; }

	inline bool IsTyped()           const { return IsEnabled() && m_isTyped; }


	inline bool IsDown()			const { return IsEnabled() && (m_state == InputState::Pressed || m_state == InputState::Down); }

	inline bool IsUp()				const { return !IsEnabled() || (m_state == InputState::Released || m_state == InputState::Up); }

	inline bool IsPreviousDown()	const { return IsEnabled() && (m_state == InputState::Released || m_state == InputState::Down); }

	inline bool IsPreviousUp()		const { return !IsEnabled() || (m_state == InputState::Pressed || m_state == InputState::Up); }

	inline bool IsPressed()			const { return IsEnabled() && (m_state == InputState::Pressed); }

	inline bool IsReleased()		const { return IsEnabled() && (m_state == InputState::Released); }

	inline bool IsHeld()			const { return IsEnabled() && (m_state == InputState::Down); }

	inline bool IsUntouched()		const { return !IsEnabled() || (m_state == InputState::Up); }

	inline bool IsChanged()			const { return IsEnabled() && (m_state == InputState::Released || m_state == InputState::Pressed); }

	inline bool IsUnchanged()		const { return !IsEnabled() || (m_state == InputState::Up || m_state == InputState::Down); }
};


// A base wrapper around InputButtonControl that manages how the down state is determined.
class InputButtonBase : public InputButtonControl
{
public:
	virtual InputButtonType ButtonType() const = 0;

	virtual bool IsContainer() const = 0;
	virtual size_t ButtonCount() const = 0;
	virtual std::shared_ptr<InputButtonBase> ButtonAt(size_t index) = 0;
	virtual const std::shared_ptr<InputButtonBase>& ButtonAt(size_t index) const = 0;

	virtual void Update(real32 elapsed) = 0;

	virtual std::string ToString() const = 0;
};


// An input button source, that gets the down state from a real control.
class InputButton : public InputButtonBase
{
protected:
	InputCodeType m_inputType;		// Whether this uses a key code, mouse code, or is invalid.
	Keys m_keyCode;				// The key code when InputCodeType::Key
	MouseButtons m_mouseCode;
	bool m_isInvertToggle;		// True if the down state is inverted during Update.

public:
	// Pass true for an 'always-down' button (which cannot use Disable untilRelease).
	inline InputButton(bool invertToggle = false)
		: m_inputType(InputCodeType::None), m_keyCode(Keys::KEY_NONE), m_mouseCode(MouseButtons::MOUSE_NONE),
		m_isInvertToggle(invertToggle)
	{
	}
	inline InputButton(Keys keyCode, bool invertToggle = false)
		: m_inputType(InputCodeType::Key), m_keyCode(keyCode), m_mouseCode(MouseButtons::MOUSE_NONE),
		m_isInvertToggle(invertToggle)
	{
	}
	inline InputButton(MouseButtons mouseCode, bool invertToggle = false)
		: m_inputType(InputCodeType::MouseButton), m_keyCode(Keys::KEY_NONE), m_mouseCode(mouseCode),
		m_isInvertToggle(invertToggle)
	{
	}

public:
	inline InputCodeType GetCodeType() const { return m_inputType; }
	inline Gods98::Keys GetKeyCode() const { return m_keyCode; }
	inline Gods98::MouseButtons GetMouseCode() const { return m_mouseCode; }
	inline bool IsInvertToggle() const { return m_isInvertToggle; }

	// Input source that is never down.
	inline bool IsNeverDown() const { return m_inputType == InputCodeType::None && !m_isInvertToggle; }
	// Input source that is always down. Cannot use Disable untilRelease.
	inline bool IsAlwaysDown() const { return m_inputType == InputCodeType::None && m_isInvertToggle; }

	inline InputButtonType ButtonType() const override { return InputButtonType::Source; }

	inline bool IsContainer() const override { return false; }
	inline size_t ButtonCount() const override { return 0; }
	inline std::shared_ptr<InputButtonBase> ButtonAt(size_t index) override { throw std::out_of_range("index"); }
	inline const std::shared_ptr<InputButtonBase>& ButtonAt(size_t index) const override { throw std::out_of_range("index"); }

	void Update(real32 elapsed) override;

	std::string ToString() const override;
};


// A multi-source button, that requires pressing any child buttons to activate.
class MultiInputButton : public InputButtonBase
{
protected:
	std::vector<std::shared_ptr<InputButtonBase>> m_childButtons;

public:
	inline MultiInputButton(const std::vector<std::shared_ptr<InputButtonBase>>& childButtons)
		: m_childButtons(childButtons)
	{
	}

	inline MultiInputButton(std::vector<std::shared_ptr<InputButtonBase>>&& childButtons)
		: m_childButtons(std::move(childButtons))
	{
	}

public:
	inline InputButtonType ButtonType() const override { return InputButtonType::Multi; }

	inline bool IsContainer() const override { return true; }
	inline size_t ButtonCount() const override { return m_childButtons.size(); }
	inline std::shared_ptr<InputButtonBase> ButtonAt(size_t index) override { return m_childButtons[index]; }
	inline const std::shared_ptr<InputButtonBase>& ButtonAt(size_t index) const override { return m_childButtons[index]; }

	void CopyStateFrom(const InputButtonControl* src) override;

	void Update(real32 elapsed) override;

	std::string ToString() const override;
};


// A combination button, that requires pressing all child buttons to activate.
class ComboInputButton : public InputButtonBase
{
protected:
	std::vector<std::shared_ptr<InputButtonBase>> m_childButtons;
	InputComboOrder m_comboOrder; // The required order for button presses.
	bool m_isPreserveCombo; // Only the final button needs to remain down after the combo is complete.

	size_t m_comboIndex;    // Buttons up to excluding this index have been pressed in order (for InputComboOrder::All).
	size_t m_preserveIndex; // This was the final button pressed before the combo was complete (defaults to count-1).

public:
	inline ComboInputButton(const std::vector<std::shared_ptr<InputButtonBase>>& childButtons, InputComboOrder comboOrder = InputComboOrder::Final, bool preserveCombo = true)
		: m_childButtons(childButtons), m_comboOrder(comboOrder), m_isPreserveCombo(preserveCombo),
		m_comboIndex(0), m_preserveIndex(static_cast<size_t>(-1))
	{
	}

	inline ComboInputButton(std::vector<std::shared_ptr<InputButtonBase>>&& childButtons, InputComboOrder comboOrder = InputComboOrder::Final, bool preserveCombo = true)
		: m_childButtons(std::move(childButtons)), m_comboOrder(comboOrder), m_isPreserveCombo(preserveCombo),
		m_comboIndex(0), m_preserveIndex(static_cast<size_t>(-1))
	{
	}

public:
	inline InputComboOrder GetComboOrder() const { return m_comboOrder; }
	inline bool IsPreserveCombo() const { return m_isPreserveCombo; }

	inline InputButtonType ButtonType() const override { return InputButtonType::Combo; }

	inline bool IsContainer() const override { return true; }
	inline size_t ButtonCount() const override { return m_childButtons.size(); }
	inline std::shared_ptr<InputButtonBase> ButtonAt(size_t index) override { return m_childButtons[index]; }
	inline const std::shared_ptr<InputButtonBase>& ButtonAt(size_t index) const override { return m_childButtons[index]; }

	void CopyStateFrom(const InputButtonControl* src) override;

	void Activate(bool immediate = false) override;

	void Update(real32 elapsed) override;

	std::string ToString() const override;
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals


#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

#pragma endregion

}

