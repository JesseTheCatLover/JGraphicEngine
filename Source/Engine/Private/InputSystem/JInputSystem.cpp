//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JInputSystem.h"
#include <algorithm>
#include <iostream>

namespace
{
    // Helper to find/create a device state entry
    FInputDeviceState& GetOrCreateDevice(
        std::vector<FInputDeviceState>& devices,
        EInputDeviceType type,
        int index,
        size_t buttonCount,
        size_t axisCount)
    {
        for (auto& d : devices)
        {
            if (d.type == type && d.index == index)
                return d;
        }

        FInputDeviceState dev;
        dev.type = type;
        dev.index = index;
        dev.buttons.assign(buttonCount, 0.0f);
        dev.axes.assign(axisCount, 0.0f);

        devices.push_back(std::move(dev));
        return devices.back();
    }

    constexpr size_t KEYBOARD_BUTTONS   = 512;
    constexpr size_t MOUSE_BUTTONS      = 8;
    constexpr size_t MOUSE_AXES         = 4;  // e.g. [0]=wheelY, [1]=wheelX, [2]=deltaX, [3]=deltaY
    constexpr size_t GAMEPAD_BUTTONS    = 32;
    constexpr size_t GAMEPAD_AXES       = 8;
}

JInputSystem::JInputSystem()
{
}

bool JInputSystem::Initialize(IInputBackend *backend)
{
    if (!backend)
    {
        std::cerr << "[JInputSystem]: failed to initialize input backend." << std::endl;
        return false;
    }
    m_Backend = backend;
    m_Events.clear();
    m_DevicesState.clear();
    m_PrevDevicesState.clear();
    m_KeyCurrent.assign(512, 0);
    m_KeyPrevious.assign(512, 0);
    return true;
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
    // 1) Copy keys
    m_KeyPrevious = m_KeyCurrent;

    // 2) Clear per-frame mouse axes
    for (auto& dev : m_DevicesState)
    {
        if (dev.type == EInputDeviceType::Mouse)
        {
            for (float& a : dev.axes)
                a = 0.0f;
        }
    }

    // 3) Apply raw events to device state + key arrays
    for (const FRawInputEvent& e : m_Events)
    {
        switch (e.type)
        {
        // ---------------- KEYBOARD ----------------
        case ERawInputType::KeyDown:
        case ERawInputType::KeyUp:
        {
            // Update key arrays (keyboard only)
            uint32_t key = e.code;
            if (key < m_KeyCurrent.size())
                m_KeyCurrent[key] = (e.type == ERawInputType::KeyDown) ? 1u : 0u;

            // Update keyboard device 0
            FInputDeviceState& kb = GetOrCreateDevice(m_DevicesState, EInputDeviceType::Keyboard, /*index*/ 0,
                                  KEYBOARD_BUTTONS, /*axes*/ 0);

            if (key < kb.buttons.size())
                kb.buttons[key] = (e.type == ERawInputType::KeyDown) ? 1.0f : 0.0f;
        }
        break;

        // ---------------- MOUSE BUTTONS ----------------
        case ERawInputType::MouseButtonDown:
        case ERawInputType::MouseButtonUp:
        {
            FInputDeviceState& mouse = GetOrCreateDevice(m_DevicesState, EInputDeviceType::Mouse, /*index*/ 0,
                                  MOUSE_BUTTONS, MOUSE_AXES);

            uint32_t btn = e.code; // backend: 0=left,1=right,2=middle,...
            if (btn < mouse.buttons.size())
                mouse.buttons[btn] = (e.type == ERawInputType::MouseButtonDown) ? 1.0f : 0.0f;
        }
        break;

        // ---------------- MOUSE WHEEL ----------------
        case ERawInputType::MouseWheel:
        {
            FInputDeviceState& mouse =
                GetOrCreateDevice(m_DevicesState, EInputDeviceType::Mouse, /*index*/ 0,
                                  MOUSE_BUTTONS, MOUSE_AXES);

            // Convention: e.code = axis index (0 = vertical, 1 = horizontal)
            uint32_t axisIndex = e.code;
            if (axisIndex < mouse.axes.size())
            {
                // Wheel is usually a delta per-frame:
                mouse.axes[axisIndex] += e.value;
            }
        }
        break;

        // ---------------- MOUSE MOVE ----------------
        case ERawInputType::MouseMove:
        {
            FInputDeviceState& mouse =
                GetOrCreateDevice(m_DevicesState, EInputDeviceType::Mouse, /*index*/ 0,
                                  MOUSE_BUTTONS, MOUSE_AXES);

            // Convention: backend sends two events:
            //   - one with code=2, value=deltaX
            //   - one with code=3, value=deltaY
            uint32_t axisIndex = e.code;
            if (axisIndex < mouse.axes.size())
            {
                mouse.axes[axisIndex] += e.value; // accumulate per frame
            }
        }
        break;

        // ---------------- GAMEPAD BUTTONS ----------------
        case ERawInputType::GamepadButtonDown:
        case ERawInputType::GamepadButtonUp:
        {
            // deviceID here can be GLFW joystick index
            FInputDeviceState& pad =
                GetOrCreateDevice(m_DevicesState, EInputDeviceType::Gamepad, e.deviceID,
                                  GAMEPAD_BUTTONS, GAMEPAD_AXES);

            uint32_t btn = e.code;
            if (btn < pad.buttons.size())
                pad.buttons[btn] = (e.type == ERawInputType::GamepadButtonDown) ? 1.0f : 0.0f;
        }
        break;

        // ---------------- GAMEPAD AXES ----------------
        case ERawInputType::GamepadAxis:
        {
            FInputDeviceState& pad =
                GetOrCreateDevice(m_DevicesState, EInputDeviceType::Gamepad, e.deviceID,
                                  GAMEPAD_BUTTONS, GAMEPAD_AXES);

            uint32_t axisIndex = e.code;
            if (axisIndex < pad.axes.size())
                pad.axes[axisIndex] = e.value; // Expected normalized -1..1 or 0..1
        }
        break;

        // ---------------- TEXT INPUT ----------------
        case ERawInputType::TextInput:
            // Usually handled by UI/text system; ignore for devices.
            break;
        }
    }
}

void JInputSystem::SetMappingStyle(TUniquePtr<IInputMappingStyle> style)
{
    m_MappingStyle = std::move(style);
    RebuildChannels();
}

void JInputSystem::Tick(float deltaTime)
{
    if (!m_Backend)
    {
        std::cerr << "[JInputSystem]: no backend provided, cannot tick." << std::endl;
        return;
    }

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
