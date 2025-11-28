//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "GLFW/glfw3.h"
#include "InputSystem/EPhysicalInput.h"

// ---------------------- Mapping helpers ----------------------

static EPhysicalInput MapGlfwKeyToEngine(int key)
{
    switch (key)
    {
        case GLFW_KEY_SPACE:        return EPhysicalInput::Key_Space;
        case GLFW_KEY_APOSTROPHE:   return EPhysicalInput::Key_Apostrophe;
        case GLFW_KEY_COMMA:        return EPhysicalInput::Key_Comma;
        case GLFW_KEY_MINUS:        return EPhysicalInput::Key_Minus;
        case GLFW_KEY_PERIOD:       return EPhysicalInput::Key_Period;
        case GLFW_KEY_SLASH:        return EPhysicalInput::Key_Slash;
        case GLFW_KEY_SEMICOLON:    return EPhysicalInput::Key_Semicolon;
        case GLFW_KEY_EQUAL:        return EPhysicalInput::Key_Equal;

        case GLFW_KEY_LEFT_BRACKET:  return EPhysicalInput::Key_LeftBracket;
        case GLFW_KEY_BACKSLASH:     return EPhysicalInput::Key_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return EPhysicalInput::Key_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:  return EPhysicalInput::Key_GraveAccent;

        case GLFW_KEY_ESCAPE:   return EPhysicalInput::Key_Escape;
        case GLFW_KEY_ENTER:    return EPhysicalInput::Key_Enter;
        case GLFW_KEY_TAB:      return EPhysicalInput::Key_Tab;
        case GLFW_KEY_BACKSPACE:return EPhysicalInput::Key_Backspace;
        case GLFW_KEY_INSERT:   return EPhysicalInput::Key_Insert;
        case GLFW_KEY_DELETE:   return EPhysicalInput::Key_Delete;
        case GLFW_KEY_HOME:     return EPhysicalInput::Key_Home;
        case GLFW_KEY_END:      return EPhysicalInput::Key_End;
        case GLFW_KEY_PAGE_UP:  return EPhysicalInput::Key_PageUp;
        case GLFW_KEY_PAGE_DOWN:return EPhysicalInput::Key_PageDown;

        case GLFW_KEY_UP:    return EPhysicalInput::Key_Up;
        case GLFW_KEY_DOWN:  return EPhysicalInput::Key_Down;
        case GLFW_KEY_LEFT:  return EPhysicalInput::Key_Left;
        case GLFW_KEY_RIGHT: return EPhysicalInput::Key_Right;

        case GLFW_KEY_F1:  return EPhysicalInput::Key_F1;
        case GLFW_KEY_F2:  return EPhysicalInput::Key_F2;
        case GLFW_KEY_F3:  return EPhysicalInput::Key_F3;
        case GLFW_KEY_F4:  return EPhysicalInput::Key_F4;
        case GLFW_KEY_F5:  return EPhysicalInput::Key_F5;
        case GLFW_KEY_F6:  return EPhysicalInput::Key_F6;
        case GLFW_KEY_F7:  return EPhysicalInput::Key_F7;
        case GLFW_KEY_F8:  return EPhysicalInput::Key_F8;
        case GLFW_KEY_F9:  return EPhysicalInput::Key_F9;
        case GLFW_KEY_F10: return EPhysicalInput::Key_F10;
        case GLFW_KEY_F11: return EPhysicalInput::Key_F11;
        case GLFW_KEY_F12: return EPhysicalInput::Key_F12;

        case GLFW_KEY_0: return EPhysicalInput::Key_0;
        case GLFW_KEY_1: return EPhysicalInput::Key_1;
        case GLFW_KEY_2: return EPhysicalInput::Key_2;
        case GLFW_KEY_3: return EPhysicalInput::Key_3;
        case GLFW_KEY_4: return EPhysicalInput::Key_4;
        case GLFW_KEY_5: return EPhysicalInput::Key_5;
        case GLFW_KEY_6: return EPhysicalInput::Key_6;
        case GLFW_KEY_7: return EPhysicalInput::Key_7;
        case GLFW_KEY_8: return EPhysicalInput::Key_8;
        case GLFW_KEY_9: return EPhysicalInput::Key_9;

        case GLFW_KEY_A: return EPhysicalInput::Key_A;
        case GLFW_KEY_B: return EPhysicalInput::Key_B;
        case GLFW_KEY_C: return EPhysicalInput::Key_C;
        case GLFW_KEY_D: return EPhysicalInput::Key_D;
        case GLFW_KEY_E: return EPhysicalInput::Key_E;
        case GLFW_KEY_F: return EPhysicalInput::Key_F;
        case GLFW_KEY_G: return EPhysicalInput::Key_G;
        case GLFW_KEY_H: return EPhysicalInput::Key_H;
        case GLFW_KEY_I: return EPhysicalInput::Key_I;
        case GLFW_KEY_J: return EPhysicalInput::Key_J;
        case GLFW_KEY_K: return EPhysicalInput::Key_K;
        case GLFW_KEY_L: return EPhysicalInput::Key_L;
        case GLFW_KEY_M: return EPhysicalInput::Key_M;
        case GLFW_KEY_N: return EPhysicalInput::Key_N;
        case GLFW_KEY_O: return EPhysicalInput::Key_O;
        case GLFW_KEY_P: return EPhysicalInput::Key_P;
        case GLFW_KEY_Q: return EPhysicalInput::Key_Q;
        case GLFW_KEY_R: return EPhysicalInput::Key_R;
        case GLFW_KEY_S: return EPhysicalInput::Key_S;
        case GLFW_KEY_T: return EPhysicalInput::Key_T;
        case GLFW_KEY_U: return EPhysicalInput::Key_U;
        case GLFW_KEY_V: return EPhysicalInput::Key_V;
        case GLFW_KEY_W: return EPhysicalInput::Key_W;
        case GLFW_KEY_X: return EPhysicalInput::Key_X;
        case GLFW_KEY_Y: return EPhysicalInput::Key_Y;
        case GLFW_KEY_Z: return EPhysicalInput::Key_Z;

        case GLFW_KEY_LEFT_SHIFT:    return EPhysicalInput::Key_LeftShift;
        case GLFW_KEY_RIGHT_SHIFT:   return EPhysicalInput::Key_RightShift;
        case GLFW_KEY_LEFT_CONTROL:  return EPhysicalInput::Key_LeftControl;
        case GLFW_KEY_RIGHT_CONTROL: return EPhysicalInput::Key_RightControl;
        case GLFW_KEY_LEFT_ALT:      return EPhysicalInput::Key_LeftAlt;
        case GLFW_KEY_RIGHT_ALT:     return EPhysicalInput::Key_RightAlt;
        case GLFW_KEY_LEFT_SUPER:    return EPhysicalInput::Key_LeftSuper;
        case GLFW_KEY_RIGHT_SUPER:   return EPhysicalInput::Key_RightSuper;

        case GLFW_KEY_KP_0:        return EPhysicalInput::Key_KP_0;
        case GLFW_KEY_KP_1:        return EPhysicalInput::Key_KP_1;
        case GLFW_KEY_KP_2:        return EPhysicalInput::Key_KP_2;
        case GLFW_KEY_KP_3:        return EPhysicalInput::Key_KP_3;
        case GLFW_KEY_KP_4:        return EPhysicalInput::Key_KP_4;
        case GLFW_KEY_KP_5:        return EPhysicalInput::Key_KP_5;
        case GLFW_KEY_KP_6:        return EPhysicalInput::Key_KP_6;
        case GLFW_KEY_KP_7:        return EPhysicalInput::Key_KP_7;
        case GLFW_KEY_KP_8:        return EPhysicalInput::Key_KP_8;
        case GLFW_KEY_KP_9:        return EPhysicalInput::Key_KP_9;
        case GLFW_KEY_KP_DECIMAL:  return EPhysicalInput::Key_KP_Decimal;
        case GLFW_KEY_KP_DIVIDE:   return EPhysicalInput::Key_KP_Divide;
        case GLFW_KEY_KP_MULTIPLY: return EPhysicalInput::Key_KP_Multiply;
        case GLFW_KEY_KP_SUBTRACT: return EPhysicalInput::Key_KP_Subtract;
        case GLFW_KEY_KP_ADD:      return EPhysicalInput::Key_KP_Add;
        case GLFW_KEY_KP_ENTER:    return EPhysicalInput::Key_KP_Enter;
        case GLFW_KEY_KP_EQUAL:    return EPhysicalInput::Key_KP_Equal;
        default: ;
    }

    return EPhysicalInput::Unknown;
}

static EPhysicalInput MapGlfwMouseButtonToEngine(int button)
{
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:   return EPhysicalInput::Mouse_ButtonLeft;
        case GLFW_MOUSE_BUTTON_RIGHT:  return EPhysicalInput::Mouse_ButtonRight;
        case GLFW_MOUSE_BUTTON_MIDDLE: return EPhysicalInput::Mouse_ButtonMiddle;
        case GLFW_MOUSE_BUTTON_4:      return EPhysicalInput::Mouse_Button4;
        case GLFW_MOUSE_BUTTON_5:      return EPhysicalInput::Mouse_Button5;
        case GLFW_MOUSE_BUTTON_6:      return EPhysicalInput::Mouse_Button6;
        case GLFW_MOUSE_BUTTON_7:      return EPhysicalInput::Mouse_Button7;
        case GLFW_MOUSE_BUTTON_8:      return EPhysicalInput::Mouse_Button8;
        default:                       return EPhysicalInput::Unknown;
    }
}

static EPhysicalInput MapGlfwGamepadButtonToEngine(int b)
{
    switch (b)
    {
        case GLFW_GAMEPAD_BUTTON_A:             return EPhysicalInput::Gamepad_ButtonSouth;
        case GLFW_GAMEPAD_BUTTON_B:             return EPhysicalInput::Gamepad_ButtonEast;
        case GLFW_GAMEPAD_BUTTON_X:             return EPhysicalInput::Gamepad_ButtonWest;
        case GLFW_GAMEPAD_BUTTON_Y:             return EPhysicalInput::Gamepad_ButtonNorth;
        case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:   return EPhysicalInput::Gamepad_LeftShoulder;
        case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:  return EPhysicalInput::Gamepad_RightShoulder;
        case GLFW_GAMEPAD_BUTTON_BACK:          return EPhysicalInput::Gamepad_Back;
        case GLFW_GAMEPAD_BUTTON_START:         return EPhysicalInput::Gamepad_Start;
        case GLFW_GAMEPAD_BUTTON_LEFT_THUMB:    return EPhysicalInput::Gamepad_LeftStick;
        case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB:   return EPhysicalInput::Gamepad_RightStick;
        case GLFW_GAMEPAD_BUTTON_DPAD_UP:       return EPhysicalInput::Gamepad_DPadUp;
        case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:    return EPhysicalInput::Gamepad_DPadRight;
        case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:     return EPhysicalInput::Gamepad_DPadDown;
        case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:     return EPhysicalInput::Gamepad_DPadLeft;
        default: ;
    }
    return EPhysicalInput::Unknown;
}

static EPhysicalInput MapGlfwGamepadAxisToEngine(int a)
{
    switch (a)
    {
        case GLFW_GAMEPAD_AXIS_LEFT_X:        return EPhysicalInput::Gamepad_AxisLeftX;
        case GLFW_GAMEPAD_AXIS_LEFT_Y:        return EPhysicalInput::Gamepad_AxisLeftY;
        case GLFW_GAMEPAD_AXIS_RIGHT_X:       return EPhysicalInput::Gamepad_AxisRightX;
        case GLFW_GAMEPAD_AXIS_RIGHT_Y:       return EPhysicalInput::Gamepad_AxisRightY;
        case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER:  return EPhysicalInput::Gamepad_AxisLeftTrigger;
        case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: return EPhysicalInput::Gamepad_AxisRightTrigger;
        default: ;
    }
    return EPhysicalInput::Unknown;
}
