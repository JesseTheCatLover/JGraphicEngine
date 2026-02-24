//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "InputSystem/EPhysicalInput.h"
#include "HotkeyPlatform.h"

struct FHotkeyChord
{
    // Example: Ctrl + Shift + P
    // Store as a normalized set (sorted, unique) for stable comparisons/serialization.
    std::vector<EPhysicalInput> keys;

    // Which platforms this chord applies to. Any == all.
    EHotkeyPlatformMask platforms = EHotkeyPlatformMask::Any;

    // Matching behavior policy
    bool allowExtraModifiers = true;
    bool allowExtraKeys = false;
};

struct FHotkeyCommand
{
    std::string name;        // "Editor.Copy", "Editor.FocusViewport1"
    std::string category;    // "Edit", "Viewport", "Play", "Selection"
    std::string description; // Human-friendly text for settings UI

    // Runtime editable active bindings
    std::vector<FHotkeyChord> chords;

    // Immutable defaults for reset
    std::vector<FHotkeyChord> defaultChords;
};

struct FHotkeyMap
{
    std::vector<FHotkeyCommand> commands;
};

struct FHotkeyOverrideEntry
{
    std::string commandName;
    std::vector<FHotkeyChord> customChords;
};

struct FHotkeyOverrides
{
    std::vector<FHotkeyOverrideEntry> entries;
};