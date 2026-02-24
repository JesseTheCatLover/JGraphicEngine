//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyles/HotkeyChord/HotkeyTextFormatter.h"

#include <sstream>
#include <vector>

namespace
{
    static bool IsModifier(EPhysicalInput k)
    {
        switch (k)
        {
        case EPhysicalInput::Key_LeftControl:
        case EPhysicalInput::Key_RightControl:
        case EPhysicalInput::Key_LeftShift:
        case EPhysicalInput::Key_RightShift:
        case EPhysicalInput::Key_LeftAlt:
        case EPhysicalInput::Key_RightAlt:
        case EPhysicalInput::Key_LeftSuper:
        case EPhysicalInput::Key_RightSuper:
            return true;
        default:
            return false;
        }
    }

    static std::string PhysicalKeyToText(EPhysicalInput key, EInputPlatform platform, bool useMacSymbols)
    {
        (void)useMacSymbols; // future polish

        switch (key)
        {
        case EPhysicalInput::Key_LeftControl:
        case EPhysicalInput::Key_RightControl:
            return (platform == EInputPlatform::MacOS) ? "Ctrl" : "Ctrl";

        case EPhysicalInput::Key_LeftShift:
        case EPhysicalInput::Key_RightShift:
            return "Shift";

        case EPhysicalInput::Key_LeftAlt:
        case EPhysicalInput::Key_RightAlt:
            return (platform == EInputPlatform::MacOS) ? "Option" : "Alt";

        case EPhysicalInput::Key_LeftSuper:
        case EPhysicalInput::Key_RightSuper:
            return (platform == EInputPlatform::MacOS) ? "Cmd" : "Super";

        case EPhysicalInput::Key_Space: return "Space";
        case EPhysicalInput::Key_Enter: return "Enter";
        case EPhysicalInput::Key_Tab: return "Tab";
        case EPhysicalInput::Key_Escape: return "Esc";
        case EPhysicalInput::Key_Backspace: return "Backspace";
        case EPhysicalInput::Key_Delete: return "Delete";

        // Numbers
        case EPhysicalInput::Key_0: return "0";
        case EPhysicalInput::Key_1: return "1";
        case EPhysicalInput::Key_2: return "2";
        case EPhysicalInput::Key_3: return "3";
        case EPhysicalInput::Key_4: return "4";
        case EPhysicalInput::Key_5: return "5";
        case EPhysicalInput::Key_6: return "6";
        case EPhysicalInput::Key_7: return "7";
        case EPhysicalInput::Key_8: return "8";
        case EPhysicalInput::Key_9: return "9";

        // Letters
        case EPhysicalInput::Key_A: return "A";
        case EPhysicalInput::Key_B: return "B";
        case EPhysicalInput::Key_C: return "C";
        case EPhysicalInput::Key_D: return "D";
        case EPhysicalInput::Key_E: return "E";
        case EPhysicalInput::Key_F: return "F";
        case EPhysicalInput::Key_G: return "G";
        case EPhysicalInput::Key_H: return "H";
        case EPhysicalInput::Key_I: return "I";
        case EPhysicalInput::Key_J: return "J";
        case EPhysicalInput::Key_K: return "K";
        case EPhysicalInput::Key_L: return "L";
        case EPhysicalInput::Key_M: return "M";
        case EPhysicalInput::Key_N: return "N";
        case EPhysicalInput::Key_O: return "O";
        case EPhysicalInput::Key_P: return "P";
        case EPhysicalInput::Key_Q: return "Q";
        case EPhysicalInput::Key_R: return "R";
        case EPhysicalInput::Key_S: return "S";
        case EPhysicalInput::Key_T: return "T";
        case EPhysicalInput::Key_U: return "U";
        case EPhysicalInput::Key_V: return "V";
        case EPhysicalInput::Key_W: return "W";
        case EPhysicalInput::Key_X: return "X";
        case EPhysicalInput::Key_Y: return "Y";
        case EPhysicalInput::Key_Z: return "Z";

        // Function keys (expand later if needed)
        case EPhysicalInput::Key_F1: return "F1";
        case EPhysicalInput::Key_F2: return "F2";
        case EPhysicalInput::Key_F3: return "F3";
        case EPhysicalInput::Key_F4: return "F4";
        case EPhysicalInput::Key_F5: return "F5";
        case EPhysicalInput::Key_F6: return "F6";
        case EPhysicalInput::Key_F7: return "F7";
        case EPhysicalInput::Key_F8: return "F8";
        case EPhysicalInput::Key_F9: return "F9";
        case EPhysicalInput::Key_F10: return "F10";
        case EPhysicalInput::Key_F11: return "F11";
        case EPhysicalInput::Key_F12: return "F12";

        default:
            return "Unknown";
        }
    }

    static bool ChordAppliesToPlatform(const FHotkeyChord& chord, EInputPlatform platform)
    {
        if (chord.platforms == EHotkeyPlatformMask::Any)
            return true;

        return HasAnyFlag(chord.platforms, ToPlatformMask(platform));
    }
}

std::string HotkeyTextFormatter::ChordToString(
    const FHotkeyChord& chord,
    EInputPlatform platform,
    const FHotkeyDisplayOptions& options)
{
    std::ostringstream oss;
    bool first = true;

    for (EPhysicalInput k : chord.keys)
    {
        if (!first)
            oss << "+";
        first = false;
        oss << PhysicalKeyToText(k, platform, options.useMacSymbols);
    }

    return oss.str();
}

std::string HotkeyTextFormatter::CommandToString(
    const FHotkeyCommand& command,
    EInputPlatform platform,
    const FHotkeyDisplayOptions& options)
{
    std::ostringstream oss;
    bool first = true;

    for (const FHotkeyChord& chord : command.chords)
    {
        if (!ChordAppliesToPlatform(chord, platform))
            continue;

        if (chord.keys.empty())
            continue;

        if (!first)
            oss << " / ";
        first = false;
        oss << ChordToString(chord, platform, options);
    }

    if (first)
        return "Unbound";

    return oss.str();
}