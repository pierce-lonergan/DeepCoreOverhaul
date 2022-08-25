// InputButton.cpp : 
//

#include "InputButton.hpp"


/**********************************************************************************
 ******** Class Globals
 **********************************************************************************/

#pragma region InputButtonControl class globals

real32 Gods98::InputButtonControl::_doubleClickTime   = 0.150f;
real32 Gods98::InputButtonControl::_typingRepeatDelay = 0.500f;
real32 Gods98::InputButtonControl::_typingRepeatTime  = 0.125f;

#pragma endregion

/**********************************************************************************
 ******** Class Functions
 **********************************************************************************/

#pragma region InputButtonControl class definition

real32 Gods98::InputButtonControl::GetDoubleClickTime()
{
	return _doubleClickTime;
}
void Gods98::InputButtonControl::SetDoubleClickTime(real32 time)
{
	_doubleClickTime = std::max(0.0f, time);
}

real32 Gods98::InputButtonControl::GetTypingRepeatDelay()
{
	return _typingRepeatDelay;
}
void Gods98::InputButtonControl::SetTypingRepeatDelay(real32 delay)
{
	_typingRepeatDelay = std::max(0.0f, delay);
}

real32 Gods98::InputButtonControl::GetTypingRepeatTime()
{
	return _typingRepeatTime;
}
void Gods98::InputButtonControl::SetTypingRepeatTime(real32 time)
{
	_typingRepeatTime = std::max(0.0f, time);
}


void Gods98::InputButtonControl::CopyStateFrom(const InputButtonControl* src)
{
	m_state           = src->m_state;
	m_disabledState   = src->m_disabledState;
	m_isDoubleClicked = src->m_isDoubleClicked;
	m_isClicked       = src->m_isClicked;
	m_isTyped         = src->m_isTyped;
	m_holdTime        = src->m_holdTime;
	m_repeatTimer     = src->m_repeatTimer;
}

void Gods98::InputButtonControl::UpdateState(real32 elapsed, bool down, bool typed, bool doubleClicked, bool clicked)
{
	// Update the duration of the control state.
	m_holdTime += elapsed;

	m_isDoubleClicked = false;
	m_isClicked       = false;
	m_isTyped         = false;

	// Update the double pressed and typed state.
	if (m_disabledState == InputDisableState::Enabled) {
		m_isDoubleClicked = doubleClicked;
		m_isClicked       = clicked;
		m_isTyped         = typed;
	}
	else if (m_disabledState == InputDisableState::Activate) {
		// Update as if we just entered the down state. Set to previous state to up.
		down = true;
		if (m_state == InputState::Pressed || m_state == InputState::Down) {
			m_state = InputState::Up;
			m_holdTime    =  0.0f;
			m_repeatTimer = -1.0f;
		}
		m_disabledState = InputDisableState::Enabled;
	}

	// Update the control state based on its down state.
	if (down) {
		if (m_disabledState == InputDisableState::Enabled) {
			if (m_state == InputState::Pressed || m_state == InputState::Down) {
				m_state = InputState::Down;

				// Handle repeated typing input:
				if (m_repeatTimer < 0.0f) { // Was set to negative number, assign initial delay.
					m_repeatTimer = GetTypingRepeatDelay();
				}
				m_repeatTimer -= elapsed;
				if (m_repeatTimer <= 0.0f) {
					m_repeatTimer = GetTypingRepeatTime();
					m_isTyped = true;
				}
			}
			else if (m_state == InputState::Released || m_state == InputState::Up) {
				m_state = InputState::Pressed;
				
				// Handle double-click input:
				if (m_holdTime < GetDoubleClickTime()) {
					/// FIXME: Won't this only trigger with a single click?
					//m_isDoubleClicked = true;
					m_isClicked = true;
				}

				m_holdTime    =  0.0f;
				m_repeatTimer = -1.0f; // Assign delay on next update.
			}
		}
		else if (m_disabledState == InputDisableState::DisabledUntilRelease) {
			m_state = InputState::Down;
		}
	}
	else {
		if (m_disabledState == InputDisableState::Enabled) {
			if (m_state == InputState::Released) {
				m_state = InputState::Up;
			}
			else if (m_state == InputState::Pressed || m_state == InputState::Down) {
				m_state = InputState::Released;
				m_holdTime    =  0.0f;
				m_repeatTimer = -1.0f; // Assign delay on next update.
			}
		}
		else if (m_disabledState == InputDisableState::DisabledUntilRelease) {
			m_disabledState = InputDisableState::Enabled;
			m_state         = InputState::Up;
		}
	}
}

// Activates a keybinds and treats its next UpdateState call as being down.
// If immediate is true, then the state will immediately be updated (rather than waiting till next Update).
void Gods98::InputButtonControl::Activate(bool immediate)
{
	/// TODO: Should we also overwrite DisableUntilRelease??
	if (m_disabledState == InputDisableState::Enabled || m_disabledState == InputDisableState::Activate) {
		m_disabledState = InputDisableState::Activate;
		if (immediate) {
			UpdateState(0.0f, true);
		}
	}
}

void Gods98::InputButtonControl::Reset(bool release)
{
	if (release && (m_state == InputState::Pressed || m_state == InputState::Down))
		m_state = InputState::Released;
	else
		m_state = InputState::Up;
	m_isDoubleClicked = false;
	m_isClicked       = false;
	m_isTyped         = false;
	m_holdTime        =  0.0f;
	m_repeatTimer     = -1.0f;
}

void Gods98::InputButtonControl::Enable()
{
	// Activate is a special disable state, the control is already considered enabled in this state.
	if (m_disabledState != InputDisableState::Activate) {
		m_disabledState = InputDisableState::Enabled;
	}
}

void Gods98::InputButtonControl::Disable(bool untilRelease)
{
	// Disabled is a higher priority, so don't lower the priority with untilRelease... maybe?
	if (untilRelease && m_disabledState != InputDisableState::Disabled) {
		if (m_state != InputState::Released && m_state != InputState::Up)
			m_disabledState = InputDisableState::DisabledUntilRelease;
		else
			m_state = InputState::Up;
	}
	else {
		m_state         = InputState::Up;
		m_disabledState	= InputDisableState::Disabled;
	}
	m_isDoubleClicked = false;
	m_isClicked       = false;
	m_isTyped         = false;
	m_holdTime        =  0.0f;
	m_repeatTimer     = -1.0f;
}

#pragma endregion


#pragma region InputButton class definition

void Gods98::InputButton::Update(real32 elapsed)
{
	// Special handling for Always-down input sources. Since Disable untilRelease would break the keybind.
	if (IsAlwaysDown()) {
		if (m_disabledState == InputDisableState::DisabledUntilRelease) {
			//m_disabledState = InputDisableState::Enabled;
			// Up for one tick, ...maybe?
			//m_state = InputState::Up;

			// Up for one tick, then enabled again.
			UpdateState(elapsed, false);
		}
		else {
			UpdateState(elapsed, true);
		}
		return;
	}

	bool down = false;
	bool doubleClicked = false;
	bool clicked = false;
	bool typed = false;

	switch (m_inputType) {
	case InputCodeType::Key:
		{
			down = Input_IsKeyDown(m_keyCode);
		}
		break;

	case InputCodeType::MouseButton:
		{
			switch (m_mouseCode) {
			case MouseButtons::MOUSE_LEFT:
				down = Gods98::mslb();
				doubleClicked = Gods98::INPUT.lDoubleClicked;
				clicked = Gods98::INPUT.lClicked;
				break;
			case MouseButtons::MOUSE_RIGHT:
				down = Gods98::msrb();
				doubleClicked = Gods98::INPUT.rDoubleClicked;
				clicked = Gods98::INPUT.rClicked;
				break;
			}
		}
		break;
	}

	// Flip the down state when using the invertToggle setting.
	if (m_isInvertToggle) down = !down;

	UpdateState(elapsed, down, typed, doubleClicked, clicked);
}

std::string Gods98::InputButton::ToString() const
{
	std::string str = (m_isInvertToggle ? "!" : "");
	const char* name = nullptr;

	switch (m_inputType) {

	case InputCodeType::Key:
		name = Key_GetName(m_keyCode);
		if (name != nullptr) {
			str += name;
		}
		else {
			str += "KEYCODE_";
			str += std::to_string((sint32)m_keyCode);
		}
		break;

	case InputCodeType::MouseButton:
		name = MouseButton_GetName(m_mouseCode);
		if (name != nullptr) {
			str += name;
		}
		else {
			str += "MOUSECODE_";
			str += std::to_string((sint32)m_mouseCode);
		}
		break;

	default:
		// Assign over str since we're not using the invert toggle here.
		if (m_isInvertToggle) {
			str = "ON";
		}
		else {
			str = "OFF";
		}
		break;
	}

	return str;
}

#pragma endregion


#pragma region MultiInputButton class definition

void Gods98::MultiInputButton::CopyStateFrom(const InputButtonControl* src)
{
	InputButtonControl::CopyStateFrom(src); // Perform base copy.

	const MultiInputButton* srcMulti = dynamic_cast<const MultiInputButton*>(src);
	if (srcMulti != nullptr) {
		// How do we handle children for this???
	}
}

void Gods98::MultiInputButton::Update(real32 elapsed)
{
	for (const auto& gameButton : m_childButtons) {
		gameButton->Update(elapsed);
	}

	if (ButtonCount() == 0) {
		UpdateState(elapsed, false);
		return;
	}
	else if (ButtonCount() == 1) {
		UpdateState(elapsed, ButtonAt(0)->IsDown());
		return;
	}

	bool down = false;
	for (size_t i = 0; i < ButtonCount() && !down; i++) {
		down |= ButtonAt(i)->IsDown();
	}
	UpdateState(elapsed, down);
}

std::string Gods98::MultiInputButton::ToString() const
{
	std::string str = "";
	for (size_t i = 0; i < ButtonCount(); i++) {
		if (i > 0) {
			str += "|";
		}
		str += m_childButtons[i]->ToString();
	}
	if (str.empty()) str = "NULL";
	return str;
}

#pragma endregion


#pragma region ComboInputButton class definition

void Gods98::ComboInputButton::CopyStateFrom(const InputButtonControl* src)
{
	InputButtonControl::CopyStateFrom(src); // Perform base copy.

	const ComboInputButton* srcCombo = dynamic_cast<const ComboInputButton*>(src);
	if (srcCombo != nullptr) {
		// How do we handle children for this???
		m_preserveIndex = srcCombo->m_preserveIndex;
		m_comboIndex = srcCombo->m_comboIndex;
	}
}

void Gods98::ComboInputButton::Activate(bool immediate)
{
	if (IsEnabled()) {
		// Ensure these fields are set for updating our down/released state.
		if (State() == InputState::Released || State() == InputState::Up) {
			m_comboIndex = ButtonCount();
			m_preserveIndex = ButtonCount() - 1;
		}
		InputButtonControl::Activate(immediate);
	}
}

void Gods98::ComboInputButton::Update(real32 elapsed)
{
	for (const auto& gameButton : m_childButtons) {
		gameButton->Update(elapsed);
	}

	if (ButtonCount() == 0) {
		UpdateState(elapsed, false);
		return;
	}
	else if (ButtonCount() == 1) {
		UpdateState(elapsed, ButtonAt(0)->IsDown());
		return;
	}

	bool down = true;
	if (State() == InputState::Pressed || State() == InputState::Down) {
		// Check if the combination is still held.
		if (m_isPreserveCombo) {
			down = ButtonAt(m_preserveIndex)->IsDown();
		}
		else {
			for (size_t i = 0; i < ButtonCount() && down; i++) {
				down &= ButtonAt(i)->IsDown();
			}
		}

		if (!down) {
			/// NOTE: DON'T reset combo index back to zero, that'll just make the control a pain to use.
			//m_comboIndex = 0;
			m_preserveIndex = static_cast<size_t>(-1);
		}
	}
	else {
		// Check/update progress for the combination being activated.
		size_t newPreserveIndex = ButtonCount() - 1; // Default to preserving final-listed button.

		if (m_comboOrder == InputComboOrder::Any) {
			for (size_t i = 0; i < ButtonCount() && down; i++) {
				const auto& button = ButtonAt(i);

				down &= button->IsDown();
				if (button->IsPressed()) {
					newPreserveIndex = i; // Any button can be preserved, or default to last button.
				}
			}
		}
		else if (m_comboOrder == InputComboOrder::Final) {

			for (size_t i = 0; i < ButtonCount() - 1 && down; i++) {
				down &= ButtonAt(i)->IsDown();
			}
			down &= ButtonAt(newPreserveIndex)->IsPressed(); // count-1
		}
		else if (m_comboOrder == InputComboOrder::All) {
			// Check that the currently-down buttons are still down.
			for (size_t i = 0; i < m_comboIndex && down; i++) {
				down &= ButtonAt(i)->IsDown();
				if (!down) {
					m_comboIndex = i; // failed at i
					break;
				}
			}
			// Check if the next buttons in the combo are pressed to continue the combo.
			for (size_t i = m_comboIndex; i < ButtonCount() && down; i++) {
				down &= ButtonAt(i)->IsPressed();
				if (down) {
					m_comboIndex = i + 1; // success at i, next is i + 1
				}
			}
		}

		if (down) {
			m_comboIndex = ButtonCount();
			m_preserveIndex = newPreserveIndex;
		}
	}
	UpdateState(elapsed, down);
}

std::string Gods98::ComboInputButton::ToString() const
{
	std::string str = "";
	for (size_t i = 0; i < ButtonCount(); i++) {
		if (i > 0) {
			str += "+";
		}
		str += m_childButtons[i]->ToString();
	}
	if (str.empty()) str = "NULL";
	return str;
}

#pragma endregion
