//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "InputSystem/MappingStyles/ActionAxis/ActionAxisConfig.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyChordConfig.h"

namespace EditorInputDefaults
{
    FActionAxisMap BuildEditorAxisMap();
    FHotkeyMap BuildEditorDefaultHotkeys();
}