//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Input/EditorInputBootstrap.h"
#include "Core/Memory/SmartPointers.h"
#include "InputSystem/InputSubsystem.h"
#include "InputSystem/MappingStyles/Composite/CompositeInputMappingStyle.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyChordStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeySerialization.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyPlatformUtils.h"
#include "Input/EditorInputDefaults.h"


bool InstallEditorInputMapping(InputSubsystem& inputSubsystem, const char* userHotkeyOverridesPath)
{
    // 1) Build defaults
    FActionAxisMap axisMap = BuildEditorAxisMap();
    FHotkeyMap hotkeyDefaults = BuildEditorDefaultHotkeys();

    // 2) Detect platform
    const EInputPlatform platform = DetectInputPlatform();

    // 3) Create hotkey style and apply user overrides
    auto hotkeyStyle = MakeUnique<HotkeyChordStyle>(hotkeyDefaults, platform);

    if (userHotkeyOverridesPath && userHotkeyOverridesPath[0] != '\0')
    {
        FHotkeyOverrides overrides;
        if (LoadHotkeyOverridesFromFile(userHotkeyOverridesPath, overrides))
        {
            hotkeyStyle->ApplyOverrides(overrides);
        }
    }

    // 4) Build composite (axis + hotkeys)
    auto composite = MakeUnique<CompositeInputMappingStyle>();
    composite->AddStyle(MakeUnique<ActionAxisStyle>(axisMap));
    composite->AddStyle(std::move(hotkeyStyle));

    // 5) Install
    inputSubsystem.SetMappingStyle(std::move(composite));
    return true;
}
