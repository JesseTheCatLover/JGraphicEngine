//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

#include "InputSystem/MappingStyles/IInputMappingStyle.h"
#include "Core/Memory/SmartPointers.h"
#include "InputSystem/MappingStyles/HotkeyChord/IHotkeyBindingEditable.h"

// A style that combines multiple mapping styles into one global channel space.
class CompositeInputMappingStyle final : public IInputMappingStyle, public IHotkeyBindingEditable
{
public:
    CompositeInputMappingStyle() = default;
    ~CompositeInputMappingStyle() override = default;

    // Add child styles before installing / before BuildChannels()
    void AddStyle(TUniquePtr<IInputMappingStyle> style);

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

    // IHotkeyBindingEditable (forward to child that supports hotkeys, if any)
    bool RebindCommand(const std::string& commandName, const FHotkeyChord& newChord, int slotIndex = 0) override;
    bool AddAlternateChord(const std::string& commandName, const FHotkeyChord& chord) override;
    bool RemoveChord(const std::string& commandName, int slotIndex) override;

    bool ResetCommandToDefault(const std::string& commandName) override;
    void ResetAllToDefaults() override;

    void ApplyOverrides(const FHotkeyOverrides& overrides) override;
    [[nodiscard]] FHotkeyOverrides ExportOverrides() const override;

    [[nodiscard]] std::string GetCommandDisplayString(const std::string& commandName) const override;

    [[nodiscard]] std::vector<FHotkeyConflict> FindConflicts(
        const FHotkeyChord& chord,
        const std::string& ignoreCommand = "") const override;

    [[nodiscard]] const FHotkeyMap& GetHotkeyMap() const override;
    [[nodiscard]] const FHotkeyCommand* FindCommandInfo(const std::string& commandName) const override;

private:
    struct FChildEntry
    {
        TUniquePtr<IInputMappingStyle> style;
        std::vector<FInputChannelDesc> localChannels;
    };

    struct FGlobalChannelRoute
    {
        uint32_t childIndex = 0;
        InputChannelHandle childHandle = INVALID_CHANNEL_HANDLE;
        EInputChannelType type = EInputChannelType::Bool;
    };

    std::vector<FChildEntry> m_Children;

    // Global merged channels and routing
    std::vector<FInputChannelDesc> m_GlobalChannels;
    std::vector<FGlobalChannelRoute> m_GlobalRoutes; // index == global handle
    std::unordered_map<std::string, InputChannelHandle> m_NameToGlobal;

    // Cache of child hotkey interface (first one found)
    IHotkeyBindingEditable* m_HotkeyEditable = nullptr;
    const IHotkeyBindingEditable* m_HotkeyEditableConst = nullptr;

    // Used only when no hotkey child exists but interface is called.
    FHotkeyMap m_EmptyHotkeyMap;

private:
    void RefreshHotkeyInterfaceCache();
    [[nodiscard]] bool IsDuplicateChannelName(const std::string& name) const;
};