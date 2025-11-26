//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JInputSystem.h"

void JInputSystem::SetMappingStyle(TUniquePtr<IInputMappingStyle> style)
{
    m_MappingStyle = std::move(style);
    RebuildChannels();
}

void JInputSystem::Tick(float deltaTime)
{
    // 1) Get raw events from OS
    m_Events.clear();
    m_Backend->FetchEvents(m_Events);

    // 2) Save previous device states
    m_PrevDevicesState = m_DevicesState;

    // 3) Apply events to produce current device state
    ProcessEvents();

    // 4) Let mapping style turn devices into channelData
    if (m_MappingStyle)
        m_MappingStyle->UpdateChannels(deltaTime, m_DevicesState, m_ChannelData);

    // 5) Build per-frame ActionState* views from channelData
}

FActionStateBool JInputSystem::GetBoolChannel(InputChannelHandle handle) const
{
    if (!m_MappingStyle)
        return FActionStateBool{};
    return m_MappingStyle->GetBoolState(handle);
}

FActionStateAxis1D JInputSystem::GetAxis1DChannel(InputChannelHandle handle) const
{
    if (!m_MappingStyle)
        return FActionStateAxis1D{};
    return m_MappingStyle->GetAxis1DState(handle);
}

FActionStateAxis2D JInputSystem::GetAxis2DChannel(InputChannelHandle handle) const
{
    if (!m_MappingStyle)
        return FActionStateAxis2D{};
    return m_MappingStyle->GetAxis2DState(handle);
}

InputChannelHandle JInputSystem::FindChannelIdByName(const std::string& name) const
{
    auto it = m_NameToHandle.find(name);
    if (it == m_NameToHandle.end())
        return static_cast<InputChannelHandle>(-1); // or some INVALID_HANDLE
    return it->second;
}

void JInputSystem::RebuildChannels()
{
    m_Channels.clear();
    m_NameToHandle.clear();
    m_ChannelData.clear();

    if (!m_MappingStyle)
        return;

    m_MappingStyle->BuildChannels(m_Channels);

    for (const FInputChannelDesc& desc : m_Channels)
    {
        m_NameToHandle[desc.name] = desc.handle;
    }

    // Optionally resize m_ChannelData
}
