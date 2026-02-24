//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "HotkeyChordConfig.h"

struct FHotkeyDisplayOptions
{
    bool useMacSymbols = false; // set true later if we want ⌘⌥⇧
};

class HotkeyTextFormatter
{
public:
    static std::string ChordToString(
        const FHotkeyChord& chord,
        EInputPlatform platform,
        const FHotkeyDisplayOptions& options = {});

    static std::string CommandToString(
        const FHotkeyCommand& command,
        EInputPlatform platform,
        const FHotkeyDisplayOptions& options = {});
};