//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "InputSystem/MappingStyles/IInputMappingStyle.h"
#include "InputSystem/FInputDeviceState.h"
#include "HotkeyChordConfig.h"
#include "HotkeyTextFormatter.h"
#include "HotkeyPlatform.h"
#include "IHotkeyBindingEditable.h"

class HotkeyChordStyle : public IInputMappingStyle, public IHotkeyBindingEditable
{
public:
    HotkeyChordStyle(FHotkeyMap map, EInputPlatform platform);

    // IInputMappingStyle
    void BuildChannels(std::vector<FInputChannelDesc>& outChannels) override;
    void UpdateChannels(
        float dt,
        const std::vector<FInputDeviceState>& devices,
        const std::vector<FInputDeviceState>& prevDevices,
        std::vector<float>& channelData) override;

    [[nodiscard]] FActionStateBool   GetBoolState(InputChannelHandle handle) const override;
    [[nodiscard]] FActionStateAxis1D GetAxis1DState(InputChannelHandle handle) const override;
    [[nodiscard]] FActionStateAxis2D GetAxis2DState(InputChannelHandle handle) const override;

    // ---------- Runtime editing ----------
    bool RebindCommand(const std::string& commandName, const FHotkeyChord& newChord, int slotIndex = 0) override;
    bool AddAlternateChord(const std::string& commandName, const FHotkeyChord& chord) override;
    bool RemoveChord(const std::string& commandName, int slotIndex) override;

    bool ResetCommandToDefault(const std::string& commandName) override;
    void ResetAllToDefaults() override;

    // ---------- Persistence helpers ----------
    void ApplyOverrides(const FHotkeyOverrides& overrides) override;
    [[nodiscard]] FHotkeyOverrides ExportOverrides() const override;

    // ---------- UI ----------
    [[nodiscard]] std::string GetCommandDisplayString(const std::string& commandName) const override;
    [[nodiscard]] const FHotkeyMap& GetMap() const { return m_Map; }

    // ---------- Conflict detection ----------
    [[nodiscard]] std::vector<FHotkeyConflict> FindConflicts(
        const FHotkeyChord& chord,
        const std::string& ignoreCommand = "") const override;

    [[nodiscard]] const FHotkeyMap& GetHotkeyMap() const override { return m_Map; }
    [[nodiscard]] const FHotkeyCommand* FindCommandInfo(const std::string& commandName) const;

private:
    FHotkeyMap m_Map;
    EInputPlatform m_Platform;

    std::vector<FActionStateBool> m_BoolStates;
    std::unordered_map<std::string, InputChannelHandle> m_NameToHandle;

private:
    static void NormalizeChord(FHotkeyChord& chord, bool bCanonicalize = true);
    static bool ChordsEqual(const FHotkeyChord& a, const FHotkeyChord& b);

    [[nodiscard]] bool ChordAppliesToCurrentPlatform(const FHotkeyChord& chord) const;

    [[nodiscard]] const FInputDeviceState* FindKeyboard(const std::vector<FInputDeviceState>& devices) const;
    [[nodiscard]] bool IsKeyDown(const FInputDeviceState* keyboard, EPhysicalInput key) const;
    [[nodiscard]] bool IsChordHeld(const FHotkeyChord& chord, const std::vector<FInputDeviceState>& devices) const;

    [[nodiscard]] bool HasNewPressInChord(
        const FHotkeyChord& chord,
        const std::vector<FInputDeviceState>& devices,
        const std::vector<FInputDeviceState>& prevDevices) const;

    [[nodiscard]] bool AnyExtraKeyDown(
        const FHotkeyChord& chord,
        const std::vector<FInputDeviceState>& devices,
        bool modifiersOnly) const;

    FHotkeyCommand* FindCommandMutable(const std::string& name);
    const FHotkeyCommand* FindCommand(const std::string& name) const;
};