//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorInput/EditorInputBootstrap.h"
#include "Core/Memory/SmartPointers.h"
#include "InputSystem/InputSubsystem.h"
#include "InputSystem/MappingStyles/Composite/CompositeInputMappingStyle.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyChordStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeySerialization.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyPlatformUtils.h"
#include "EditorInput/EditorInputDefaults.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"


bool EditorInputBootstrap::InstallEditorInputMapping(InputSubsystem& inputSubsystem)
{
    // 1) Build defaults in-memory (Source of Truth)
    FActionAxisMap axisMap = EditorInputDefaults::BuildEditorAxisMap();
    FHotkeyMap hotkeyDefaults = EditorInputDefaults::BuildEditorDefaultHotkeys();

    // 2) Detect platform
    const EInputPlatform platform = DetectInputPlatform();
    auto hotkeyStyle = MakeUnique<HotkeyChordStyle>(hotkeyDefaults, platform);

    // 3) Resolve OS User Config Path (e.g., %APPDATA%/RedleafEngine)
    std::string userConfigDir = UFileSystem::GetUserConfigDirectory();
    std::string hotkeyUserPath = UPath::Join(userConfigDir, "EditorHotkeys.User.json");

    // 4) Load and apply only the overrides
    FHotkeyOverrides overrides;
    if (LoadHotkeyOverridesFromFile(hotkeyUserPath, overrides))
    {
        hotkeyStyle->ApplyOverrides(overrides);
    }
    else
    {
        // Ensure the directory exists so the engine's Hotkey Editor UI can save to it later without failing
        UFileSystem::CreateDirectory(userConfigDir);
    }

    // 5) Build composite (axis + hotkeys)
    auto composite = MakeUnique<CompositeInputMappingStyle>();
    composite->AddStyle(MakeUnique<ActionAxisStyle>(axisMap));
    composite->AddStyle(std::move(hotkeyStyle));

    // 6) Install
    inputSubsystem.SetMappingStyle(std::move(composite));
    return true;
}
