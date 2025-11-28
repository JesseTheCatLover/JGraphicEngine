//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

/**
 * @brief Engine-wide, backend-independent "physical input code".
 *
 * - Keyboard keys
 * - Mouse buttons & axes
 * - Gamepad buttons & axes
 * - Touch contacts (reserved)
 *
 * Backends (GLFW, Win32, Cocoa, etc.) translate their native codes into EPhysicalInput.
 * Higher-level mappings only ever see these values.
 */
enum class EPhysicalInput : uint16_t
{
    // -----------------------------
    // Special / meta
    // -----------------------------
    Unknown = 0,

    // -----------------------------
    // Ranges (for quick category checks)
    // -----------------------------
    Key_First      = 0x0001,
    Key_Last       = 0x00FF,

    Mouse_First    = 0x0100,
    Mouse_Last     = 0x01FF,

    Gamepad_First  = 0x0200,
    Gamepad_Last   = 0x02FF,

    Touch_First    = 0x0300,
    Touch_Last     = 0x03FF,

    // ============================================================
    // KEYBOARD
    // ============================================================

    // --- Control / Misc ---
    Key_Space          = 0x0001,
    Key_Apostrophe,        // '
    Key_Comma,             // ,
    Key_Minus,             // -
    Key_Period,            // .
    Key_Slash,             // /
    Key_Semicolon,         // ;
    Key_Equal,             // =

    Key_LeftBracket,       // [
    Key_Backslash,         // '\'
    Key_RightBracket,      // ]
    Key_GraveAccent,       // `

    Key_Escape,
    Key_Enter,
    Key_Tab,
    Key_Backspace,
    Key_Insert,
    Key_Delete,
    Key_Home,
    Key_End,
    Key_PageUp,
    Key_PageDown,

    // --- Arrows ---
    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,

    // --- Function keys ---
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    Key_F13,
    Key_F14,
    Key_F15,
    Key_F16,
    Key_F17,
    Key_F18,
    Key_F19,
    Key_F20,
    Key_F21,
    Key_F22,
    Key_F23,
    Key_F24,

    // --- Number row (top) ---
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    // --- Letters ---
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    // --- Modifiers ---
    Key_LeftShift,
    Key_RightShift,
    Key_LeftControl,
    Key_RightControl,
    Key_LeftAlt,
    Key_RightAlt,
    Key_LeftSuper,   // Cmd (macOS) / Win (Windows)
    Key_RightSuper,

    // --- Numpad ---
    Key_KP_0,
    Key_KP_1,
    Key_KP_2,
    Key_KP_3,
    Key_KP_4,
    Key_KP_5,
    Key_KP_6,
    Key_KP_7,
    Key_KP_8,
    Key_KP_9,
    Key_KP_Decimal,
    Key_KP_Divide,
    Key_KP_Multiply,
    Key_KP_Subtract,
    Key_KP_Add,
    Key_KP_Enter,
    Key_KP_Equal,

    // ============================================================
    // MOUSE
    // ============================================================
    Mouse_ButtonLeft      = 0x0100,
    Mouse_ButtonRight,
    Mouse_ButtonMiddle,
    Mouse_Button4,
    Mouse_Button5,
    Mouse_Button6,
    Mouse_Button7,
    Mouse_Button8,

    // Axes (continuous values)
    // X/Y can be either absolute or deltas depending on binding semantics.
    Mouse_AxisX,
    Mouse_AxisY,

    // Scroll wheel (per-frame deltas)
    Mouse_WheelX,
    Mouse_WheelY,

    // Optional: raw deltas (high precision / locked cursor)
    Mouse_DeltaX,
    Mouse_DeltaY,

    // ============================================================
    // GAMEPAD
    // ============================================================
    //
    // These names follow a generic / XInput style:
    //  - South/East/West/North = A/B/X/Y (Xbox), Cross/Circle/Square/Triangle (PS)
    // ============================================================

    // --- Buttons ---
    Gamepad_ButtonSouth   = 0x0200,  // A / Cross
    Gamepad_ButtonEast,             // B / Circle
    Gamepad_ButtonWest,             // X / Square
    Gamepad_ButtonNorth,            // Y / Triangle

    Gamepad_LeftShoulder,
    Gamepad_RightShoulder,
    Gamepad_Back,                   // Select / View
    Gamepad_Start,                  // Start / Menu
    Gamepad_LeftStick,              // L3
    Gamepad_RightStick,             // R3

    Gamepad_DPadUp,
    Gamepad_DPadRight,
    Gamepad_DPadDown,
    Gamepad_DPadLeft,

    // --- Axes ---
    Gamepad_AxisLeftX,
    Gamepad_AxisLeftY,
    Gamepad_AxisRightX,
    Gamepad_AxisRightY,
    Gamepad_AxisLeftTrigger,        // 0..1 or -1..1 depending on backend
    Gamepad_AxisRightTrigger,

    // ============================================================
    // TOUCH / POINTER (generic)
    // ============================================================

    // 10 concurrent touches should be plenty; each has X/Y axes.
    Touch_Contact0       = 0x0300,
    Touch_Contact1,
    Touch_Contact2,
    Touch_Contact3,
    Touch_Contact4,
    Touch_Contact5,
    Touch_Contact6,
    Touch_Contact7,
    Touch_Contact8,
    Touch_Contact9,

    // Optional positional axes per contact (screen-space)
    Touch0_AxisX,
    Touch0_AxisY,
    Touch1_AxisX,
    Touch1_AxisY,
    Touch2_AxisX,
    Touch2_AxisY,
    Touch3_AxisX,
    Touch3_AxisY,
    Touch4_AxisX,
    Touch4_AxisY,
    Touch5_AxisX,
    Touch5_AxisY,
    Touch6_AxisX,
    Touch6_AxisY,
    Touch7_AxisX,
    Touch7_AxisY,
    Touch8_AxisX,
    Touch8_AxisY,
    Touch9_AxisX,
    Touch9_AxisY,

    // ============================================================
    // Count / sentinel
    // ============================================================
    Count
};

inline bool IsKeyboard(EPhysicalInput input)
{
    return input > EPhysicalInput::Unknown &&
           input >= EPhysicalInput::Key_First &&
           input <= EPhysicalInput::Key_Last;
}

inline bool IsMouse(EPhysicalInput input)
{
    return input >= EPhysicalInput::Mouse_First &&
           input <= EPhysicalInput::Mouse_Last;
}

inline bool IsGamepad(EPhysicalInput input)
{
    return input >= EPhysicalInput::Gamepad_First &&
           input <= EPhysicalInput::Gamepad_Last;
}

inline bool IsTouch(EPhysicalInput input)
{
    return input >= EPhysicalInput::Touch_First &&
           input <= EPhysicalInput::Touch_Last;
}
