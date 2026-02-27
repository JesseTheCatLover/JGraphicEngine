//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyles/Composite/CompositeInputMappingStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/FHotkeyConflict.h"

#include <iostream>

void CompositeInputMappingStyle::AddStyle(TUniquePtr<IInputMappingStyle> style)
{
    if (!style)
        return;

    FChildEntry entry;
    entry.style = std::move(style);
    m_Children.push_back(std::move(entry));

    RefreshHotkeyInterfaceCache();
}

void CompositeInputMappingStyle::BuildChannels(std::vector<FInputChannelDesc>& outChannels)
{
    m_GlobalChannels.clear();
    m_GlobalRoutes.clear();
    m_NameToGlobal.clear();

    InputChannelHandle nextGlobalHandle = 0;

    // Build local channels for each child, then merge
    for (uint32_t childIdx = 0; childIdx < static_cast<uint32_t>(m_Children.size()); ++childIdx)
    {
        FChildEntry& child = m_Children[childIdx];
        child.localChannels.clear();

        if (!child.style)
            continue;

        child.style->BuildChannels(child.localChannels);

        for (const FInputChannelDesc& localDesc : child.localChannels)
        {
            if (IsDuplicateChannelName(localDesc.name))
            {
                std::cerr << "[CompositeInputMappingStyle]: Duplicate channel name '" << localDesc.name
                          << "' detected across child styles. Skipping duplicate.\n";
                continue;
            }

            FInputChannelDesc globalDesc;
            globalDesc.handle = nextGlobalHandle;
            globalDesc.name = localDesc.name;
            globalDesc.type = localDesc.type;

            m_GlobalChannels.push_back(globalDesc);
            m_NameToGlobal[globalDesc.name] = globalDesc.handle;

            FGlobalChannelRoute route;
            route.childIndex = childIdx;
            route.childHandle = localDesc.handle;
            route.type = localDesc.type;

            m_GlobalRoutes.push_back(route);

            ++nextGlobalHandle;
        }
    }

    outChannels = m_GlobalChannels;
}

void CompositeInputMappingStyle::UpdateChannels(
    float dt,
    const std::vector<FInputDeviceState>& devices,
    const std::vector<FInputDeviceState>& prevDevices,
    std::vector<float>& channelData)
{
    // channelData is currently unused by our child styles; preserve interface
    for (FChildEntry& child : m_Children)
    {
        if (!child.style)
            continue;

        child.style->UpdateChannels(dt, devices, prevDevices, channelData);
    }
}

FActionStateBool CompositeInputMappingStyle::GetBoolState(InputChannelHandle handle) const
{
    if (handle >= m_GlobalRoutes.size())
        return {};

    const FGlobalChannelRoute& route = m_GlobalRoutes[handle];
    if (route.childIndex >= m_Children.size())
        return {};

    const FChildEntry& child = m_Children[route.childIndex];
    if (!child.style)
        return {};

    return child.style->GetBoolState(route.childHandle);
}

FActionStateAxis1D CompositeInputMappingStyle::GetAxis1DState(InputChannelHandle handle) const
{
    if (handle >= m_GlobalRoutes.size())
        return {};

    const FGlobalChannelRoute& route = m_GlobalRoutes[handle];
    if (route.childIndex >= m_Children.size())
        return {};

    const FChildEntry& child = m_Children[route.childIndex];
    if (!child.style)
        return {};

    return child.style->GetAxis1DState(route.childHandle);
}

FActionStateAxis2D CompositeInputMappingStyle::GetAxis2DState(InputChannelHandle handle) const
{
    if (handle >= m_GlobalRoutes.size())
        return {};

    const FGlobalChannelRoute& route = m_GlobalRoutes[handle];
    if (route.childIndex >= m_Children.size())
        return {};

    const FChildEntry& child = m_Children[route.childIndex];
    if (!child.style)
        return {};

    return child.style->GetAxis2DState(route.childHandle);
}

std::vector<std::string> CompositeInputMappingStyle::ConsumeTriggeredCommands()
{
    return m_HotkeyEditable ? m_HotkeyEditable->ConsumeTriggeredCommands()
                            : std::vector<std::string>{};
}

// ---------------- IHotkeyBindingEditable forwarding ----------------

bool CompositeInputMappingStyle::RebindCommand(const std::string& commandName, const FHotkeyChord& newChord, int slotIndex)
{
    return m_HotkeyEditable ? m_HotkeyEditable->RebindCommand(commandName, newChord, slotIndex) : false;
}

bool CompositeInputMappingStyle::AddAlternateChord(const std::string& commandName, const FHotkeyChord& chord)
{
    return m_HotkeyEditable ? m_HotkeyEditable->AddAlternateChord(commandName, chord) : false;
}

bool CompositeInputMappingStyle::RemoveChord(const std::string& commandName, int slotIndex)
{
    return m_HotkeyEditable ? m_HotkeyEditable->RemoveChord(commandName, slotIndex) : false;
}

bool CompositeInputMappingStyle::ResetCommandToDefault(const std::string& commandName)
{
    return m_HotkeyEditable ? m_HotkeyEditable->ResetCommandToDefault(commandName) : false;
}

void CompositeInputMappingStyle::ResetAllToDefaults()
{
    if (m_HotkeyEditable)
        m_HotkeyEditable->ResetAllToDefaults();
}

void CompositeInputMappingStyle::ApplyOverrides(const FHotkeyOverrides& overrides)
{
    if (m_HotkeyEditable)
        m_HotkeyEditable->ApplyOverrides(overrides);
}

FHotkeyOverrides CompositeInputMappingStyle::ExportOverrides() const
{
    return m_HotkeyEditableConst ? m_HotkeyEditableConst->ExportOverrides() : FHotkeyOverrides{};
}

std::string CompositeInputMappingStyle::GetCommandDisplayString(const std::string& commandName) const
{
    return m_HotkeyEditableConst ? m_HotkeyEditableConst->GetCommandDisplayString(commandName) : std::string{};
}

std::vector<FHotkeyConflict> CompositeInputMappingStyle::FindConflicts(
    const FHotkeyChord& chord,
    const std::string& ignoreCommand) const
{
    return m_HotkeyEditableConst ? m_HotkeyEditableConst->FindConflicts(chord, ignoreCommand)
                                 : std::vector<FHotkeyConflict>{};
}

const FHotkeyMap& CompositeInputMappingStyle::GetHotkeyMap() const
{
    return m_HotkeyEditableConst ? m_HotkeyEditableConst->GetHotkeyMap() : m_EmptyHotkeyMap;
}

const FHotkeyCommand* CompositeInputMappingStyle::FindCommandInfo(const std::string& commandName) const
{
    return m_HotkeyEditableConst ? m_HotkeyEditableConst->FindCommandInfo(commandName) : nullptr;
}

// ---------------- Helpers ----------------

void CompositeInputMappingStyle::RefreshHotkeyInterfaceCache()
{
    m_HotkeyEditable = nullptr;
    m_HotkeyEditableConst = nullptr;

    for (FChildEntry& child : m_Children)
    {
        if (!child.style)
            continue;

        if (IHotkeyBindingEditable* editable = dynamic_cast<IHotkeyBindingEditable*>(child.style.get()))
        {
            m_HotkeyEditable = editable;
            m_HotkeyEditableConst = editable;
            return;
        }
    }
}

bool CompositeInputMappingStyle::IsDuplicateChannelName(const std::string& name) const
{
    return m_NameToGlobal.find(name) != m_NameToGlobal.end();
}