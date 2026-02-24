//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "HotkeyChordConfig.h"

struct FHotkeyConflict;

class IHotkeyBindingEditable
{
public:
    virtual ~IHotkeyBindingEditable() = default;

    virtual bool RebindCommand(const std::string& commandName, const FHotkeyChord& newChord, int slotIndex = 0) = 0;
    virtual bool AddAlternateChord(const std::string& commandName, const FHotkeyChord& chord) = 0;
    virtual bool RemoveChord(const std::string& commandName, int slotIndex) = 0;

    virtual bool ResetCommandToDefault(const std::string& commandName) = 0;
    virtual void ResetAllToDefaults() = 0;

    virtual void ApplyOverrides(const FHotkeyOverrides& overrides) = 0;
    [[nodiscard]] virtual FHotkeyOverrides ExportOverrides() const = 0;

    [[nodiscard]] virtual std::string GetCommandDisplayString(const std::string& commandName) const = 0;

    [[nodiscard]] virtual std::vector<FHotkeyConflict> FindConflicts(
        const FHotkeyChord& chord,
        const std::string& ignoreCommand = "") const = 0;

    [[nodiscard]] virtual const FHotkeyMap& GetHotkeyMap() const = 0;
    [[nodiscard]] virtual const FHotkeyCommand* FindCommandInfo(const std::string& commandName) const = 0;
};