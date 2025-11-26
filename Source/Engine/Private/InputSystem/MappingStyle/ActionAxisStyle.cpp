//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyle/ActionAxisStyle.h"

ActionAxisStyle::ActionAxisStyle(const FActionAxisMap &configMap)
: m_ConfigMap(configMap)
{
}

void ActionAxisStyle::BuildChannels(std::vector<FInputChannelDesc> &outChannels)
{
    outChannels.clear();
    outChannels.reserve(m_ConfigMap.actions.size());

    for (size_t i = 0; i < m_ConfigMap.actions.size(); ++i)
    {
        const FActionAxisSlot& slot = m_ConfigMap.actions[i];

        FInputChannelDesc desc;
        desc.handle = static_cast<InputChannelHandle>(i); // index == handle
        desc.name = slot.name;
        desc.type = slot.type;

        outChannels.push_back(desc);
    }

    const size_t channelCount = outChannels.size();
    m_BoolStates.resize(channelCount);
    m_Axis1DStates.resize(channelCount);
    m_Axis2DStates.resize(channelCount);
}

void ActionAxisStyle::UpdateChannels(float deltaTime, const std::vector<FInputDeviceState>& devices,
    std::vector<float>& channelData)
{
    const size_t channelCount = m_ConfigMap.actions.size();
    for (size_t i = 0; i < channelCount; ++i)
    {
        const FActionAxisSlot& slot = m_ConfigMap.actions[i];

        switch (slot.type)
        {
        case EInputChannelType::Bool:
        {
            // previous held state
            FActionStateBool& state = m_BoolStates[i];
            const bool wasHeld = state.held;

            bool heldNow = false;
            for (const FInputBinding& binding : slot.bindings)
            {
                float v = GetBindingValue(binding, devices);
                if (v > 0.5f) // pressed threshold
                {
                    heldNow = true;
                    break; // OR of all bindings
                }
            }

            state.held = heldNow;
            state.pressed = ( heldNow && !wasHeld );
            state.released = (!heldNow &&  wasHeld);
        }
        break;

        case EInputChannelType::Axis1D:
        {
            FActionStateAxis1D& state = m_Axis1DStates[i];
            const float prevValue = state.value;

            float valueNow = 0.0f;
            for (const FInputBinding& binding : slot.bindings)
            {
                valueNow += GetBindingValue(binding, devices);
            }

            state.value = valueNow;
            state.delta = valueNow - prevValue;
        }
        break;

        case EInputChannelType::Axis2D:
        {
            FActionStateAxis2D& state = m_Axis2DStates[i];
            const float prevX = state.x;
            const float prevY = state.y;

            // Simple version:
            // - assume first binding contributes to X
            // - second binding contributes to Y
            // You can refine this later with an explicit “component” in FInputBinding.
            float xNow = 0.0f;
            float yNow = 0.0f;

            if (!slot.bindings.empty())
                xNow = GetBindingValue(slot.bindings[0], devices);
            if (slot.bindings.size() > 1)
                yNow = GetBindingValue(slot.bindings[1], devices);

            state.x  = xNow;
            state.y  = yNow;
            state.dx = xNow - prevX;
            state.dy = yNow - prevY;
        }
        break;
        }
    }
}

float ActionAxisStyle::GetBindingValue(const FInputBinding& binding, const std::vector<FInputDeviceState>& devices) const
{
    // find matching device
    const FInputDeviceState* dev = nullptr;
    for (auto& d : devices)
    {
        if (d.type == binding.deviceType && d.index == binding.deviceIndex)
        {
            dev = &d;
            break;
        }
    }
    if (!dev)
        return 0.0f;

    float raw = 0.0f;

    switch (dev->type)
    {
        case EInputDeviceType::Keyboard:
        case EInputDeviceType::Gamepad:
            if (binding.code >= 0 &&
                static_cast<size_t>(binding.code) < dev->buttons.size())
            {
                raw = dev->buttons[binding.code];
            }
            break;

        case EInputDeviceType::Mouse:
            // for now, assume mouse buttons use `buttons`,
            // axes (wheel, delta) use `axes`, you can refine this later
            if (binding.code >= 0 &&
                static_cast<size_t>(binding.code) < dev->buttons.size())
            {
                raw = dev->buttons[binding.code];
            }
            break;
    }

    // dead zone
    if (std::abs(raw) < binding.deadZone)
        raw = 0.0f;

    if (binding.invert)
        raw = -raw;

    raw *= binding.scale;

    return raw;
}

// We’ll keep one state array per type, sized to #channels.
// Index == InputChannelHandle
static size_t CountBoolChannels(const FActionAxisMap& map)
{
    size_t c = 0;
    for (auto& slot : map.actions)
        if (slot.type == EInputChannelType::Bool) ++c;
    return c;
}

FActionStateBool ActionAxisStyle::GetBoolState(InputChannelHandle handle) const
{
    if (handle >= m_BoolStates.size())
        return FActionStateBool{};
    return m_BoolStates[handle];
}

FActionStateAxis1D ActionAxisStyle::GetAxis1DState(InputChannelHandle handle) const
{
    if (handle >= m_Axis1DStates.size())
        return FActionStateAxis1D{};
    return m_Axis1DStates[handle];
}

FActionStateAxis2D ActionAxisStyle::GetAxis2DState(InputChannelHandle handle) const
{
    if (handle >= m_Axis2DStates.size())
        return FActionStateAxis2D{};
    return m_Axis2DStates[handle];
}

