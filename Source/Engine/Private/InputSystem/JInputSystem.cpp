//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JInputSystem.h"
#include <algorithm>

JInputSystem::JInputSystem()
{
}

void JInputSystem::Initialize(IInputBackend *backend)
{
    m_Backend = backend;
    m_Events.clear();
    m_DevicesState.clear();
    m_PrevDevicesState.clear();
    m_KeyCurrent.assign(512, 0);
    m_KeyPrevious.assign(512, 0);
}

void JInputSystem::Shutdown()
{
    m_Backend = nullptr;
    m_MappingStyle.reset();
    m_Events.clear();
    m_DevicesState.clear();
    m_PrevDevicesState.clear();
    m_Channels.clear();
    m_NameToHandle.clear();
    m_BoolCallbacks.clear();
    m_Axis1DCallbacks.clear();
    m_Axis2DCallbacks.clear();
    m_NextCallbackHandle = 1;
}

InputCallbackHandle JInputSystem::RegisterBoolCallback(
    const std::string& channelName,
    EInputEventPhase phase,
    FBoolActionCallback cb)
{
    if (!m_MappingStyle || !cb)
        return INVALID_INPUT_CALLBACK;

    InputChannelHandle handle = FindChannelIdByName(channelName);
    if (handle == INVALID_CHANNEL_HANDLE)
        return INVALID_INPUT_CALLBACK; // or allow and resolve later; your call

    FBoolCallbackEntry entry;
    entry.handle       = m_NextCallbackHandle++;
    entry.channelName  = channelName;
    entry.channelHandle = handle;
    entry.phase        = phase;
    entry.callback     = std::move(cb);

    m_BoolCallbacks.push_back(std::move(entry));
    return entry.handle;
}

InputCallbackHandle JInputSystem::RegisterAxis1DCallback(
    const std::string& channelName,
    EInputEventPhase phase,
    FAxis1DActionCallback cb)
{
    if (!m_MappingStyle || !cb)
        return INVALID_INPUT_CALLBACK;

    InputChannelHandle handle = FindChannelIdByName(channelName);
    if (handle == INVALID_CHANNEL_HANDLE)
        return INVALID_INPUT_CALLBACK;

    FAxis1DCallbackEntry entry;
    entry.handle        = m_NextCallbackHandle++;
    entry.channelName   = channelName;
    entry.channelHandle = handle;
    entry.phase         = phase;
    entry.callback      = std::move(cb);

    m_Axis1DCallbacks.push_back(std::move(entry));
    return entry.handle;
}

InputCallbackHandle JInputSystem::RegisterAxis2DCallback(
    const std::string& channelName,
    EInputEventPhase phase,
    FAxis2DActionCallback cb)
{
    if (!m_MappingStyle || !cb)
        return INVALID_INPUT_CALLBACK;

    InputChannelHandle handle = FindChannelIdByName(channelName);
    if (handle == INVALID_CHANNEL_HANDLE)
        return INVALID_INPUT_CALLBACK;

    FAxis2DCallbackEntry entry;
    entry.handle        = m_NextCallbackHandle++;
    entry.channelName   = channelName;
    entry.channelHandle = handle;
    entry.phase         = phase;
    entry.callback      = std::move(cb);

    m_Axis2DCallbacks.push_back(std::move(entry));
    return entry.handle;
}

void JInputSystem::UnregisterCallback(InputCallbackHandle handle)
{
    if (handle == INVALID_INPUT_CALLBACK)
        return;

    auto removeByHandle = [handle](auto& vec)
    {
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [handle](const auto& e) { return e.handle == handle; }),
            vec.end());
    };

    removeByHandle(m_BoolCallbacks);
    removeByHandle(m_Axis1DCallbacks);
    removeByHandle(m_Axis2DCallbacks);
}

bool JInputSystem::IsKeyDown(uint32_t keyCode) const
{
    if (keyCode >= static_cast<uint32_t>(m_KeyCurrent.size()))
        return false;
    return m_KeyCurrent[keyCode] != 0;
}

bool JInputSystem::WasKeyPressed(uint32_t keyCode) const
{
    if (keyCode >= static_cast<uint32_t>(m_KeyCurrent.size()))
        return false;
    return m_KeyCurrent[keyCode] != 0 && m_KeyPrevious[keyCode] == 0;
}

bool JInputSystem::WasKeyReleased(uint32_t keyCode) const
{
    if (keyCode >= static_cast<uint32_t>(m_KeyCurrent.size()))
        return false;
    return m_KeyCurrent[keyCode] == 0 && m_KeyPrevious[keyCode] != 0;
}

void JInputSystem::ProcessEvents()
{
    // 1) copy keys
    m_KeyPrevious = m_KeyCurrent;

    // 2) ensure device list exists
    //    or grow on demand based on event.deviceID

    for (const FRawInputEvent& e : m_Events)
    {
        // update m_KeyCurrent for KeyDown/KeyUp
        // update matching FInputDeviceState.buttons/axes based on e.code/value
    }
}

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

    // 5) Fire callbacks
    if (m_MappingStyle)
        DispatchCallbacks();
}
void JInputSystem::DispatchCallbacks()
{
    if (!m_MappingStyle)
        return;

    const size_t channelCount = m_Channels.size();

    for (size_t i = 0; i < channelCount; ++i)
    {
        const FInputChannelDesc& desc = m_Channels[i];
        InputChannelHandle handle = desc.handle;

        switch (desc.type)
        {
        case EInputChannelType::Bool:
        {
            FActionStateBool st = m_MappingStyle->GetBoolState(handle);

            for (const auto& entry : m_BoolCallbacks)
            {
                // skip callbacks not bound to this channel, or invalid
                if (entry.channelHandle != handle ||
                    entry.channelHandle == INVALID_CHANNEL_HANDLE)
                    continue;

                switch (entry.phase)
                {
                case EInputEventPhase::Pressed:
                    if (st.pressed)
                        entry.callback(handle, st);
                    break;
                case EInputEventPhase::Released:
                    if (st.released)
                        entry.callback(handle, st);
                    break;
                case EInputEventPhase::Held:
                    if (st.held)
                        entry.callback(handle, st);
                    break;
                default:
                    break;
                }
            }
        }
        break;

        case EInputChannelType::Axis1D:
        {
            FActionStateAxis1D st = m_MappingStyle->GetAxis1DState(handle);

            for (const auto& entry : m_Axis1DCallbacks)
            {
                if (entry.channelHandle != handle ||
                    entry.channelHandle == INVALID_CHANNEL_HANDLE)
                    continue;

                switch (entry.phase)
                {
                case EInputEventPhase::AxisChanged:
                    if (st.delta != 0.0f)
                        entry.callback(handle, st);
                    break;
                case EInputEventPhase::Held:
                    if (st.value != 0.0f)
                        entry.callback(handle, st);
                    break;
                default:
                    break;
                }
            }
        }
        break;

        case EInputChannelType::Axis2D:
        {
            FActionStateAxis2D st = m_MappingStyle->GetAxis2DState(handle);

            for (const auto& entry : m_Axis2DCallbacks)
            {
                if (entry.channelHandle != handle ||
                    entry.channelHandle == INVALID_CHANNEL_HANDLE)
                    continue;

                switch (entry.phase)
                {
                case EInputEventPhase::AxisChanged:
                    if (st.dx != 0.0f || st.dy != 0.0f)
                        entry.callback(handle, st);
                    break;
                case EInputEventPhase::Held:
                    if (st.x != 0.0f || st.y != 0.0f)
                        entry.callback(handle, st);
                    break;
                default:
                    break;
                }
            }
        }
        break;
        }
    }
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
        return INVALID_CHANNEL_HANDLE;
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

    // Channels changed – bump version
    ++m_ChannelVersion;

    // Update callback entries to match new handles
    for (auto& entry : m_BoolCallbacks)
        entry.channelHandle = FindChannelIdByName(entry.channelName);

    for (auto& entry : m_Axis1DCallbacks)
        entry.channelHandle = FindChannelIdByName(entry.channelName);

    for (auto& entry : m_Axis2DCallbacks)
        entry.channelHandle = FindChannelIdByName(entry.channelName);
}
