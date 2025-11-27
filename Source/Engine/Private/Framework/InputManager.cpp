//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Framework/InputManager.h"
#include "InputSystem/JInputSystem.h"

bool InputManager::Initialize(JInputSystem *system)
{
    if (!system)
        return false;

    m_InputSystem = system;
    m_Cache.clear();
    m_LastChannelVersion = m_InputSystem->GetChannelVersion();

    return true;
}

InputChannelHandle InputManager::GetChannelHandle(const std::string &name)
{
    // 1) If channels changed, clear cache
    if (m_InputSystem)
    {
        uint32_t currentVersion = m_InputSystem->GetChannelVersion();
        if (currentVersion != m_LastChannelVersion)
        {
            m_Cache.clear();
            m_LastChannelVersion = currentVersion;
        }
    }

    // 2) Normal cached lookup
    auto it = m_Cache.find(name);
    if (it != m_Cache.end())
        return it->second;

    InputChannelHandle handle = m_InputSystem->FindChannelIdByName(name);
    m_Cache[name] = handle;
    return handle;
}
InputCallbackHandle InputManager::BindAction(
    const std::string& name,
    EInputEventPhase phase,
    FBoolActionCallback cb)
{
    if (!m_InputSystem)
        return INVALID_INPUT_CALLBACK;

    return m_InputSystem->RegisterBoolCallback(name, phase, std::move(cb));
}

InputCallbackHandle InputManager::BindAxis1D(
    const std::string& name,
    EInputEventPhase phase,
    FAxis1DActionCallback cb)
{
    if (!m_InputSystem)
        return INVALID_INPUT_CALLBACK;

    return m_InputSystem->RegisterAxis1DCallback(name, phase, std::move(cb));
}

InputCallbackHandle InputManager::BindAxis2D(
    const std::string& name,
    EInputEventPhase phase,
    FAxis2DActionCallback cb)
{
    if (!m_InputSystem)
        return INVALID_INPUT_CALLBACK;

    return m_InputSystem->RegisterAxis2DCallback(name, phase, std::move(cb));
}

void InputManager::Unbind(InputCallbackHandle handle)
{
    if (!m_InputSystem || handle == INVALID_INPUT_CALLBACK)
        return;

    m_InputSystem->UnregisterCallback(handle);
}

bool InputManager::GetActionDown(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == INVALID_CHANNEL_HANDLE)
        return false;

    FActionStateBool st = m_InputSystem->GetBoolChannel(handle);
    return st.pressed;
}

bool InputManager::GetActionUp(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == INVALID_CHANNEL_HANDLE)
        return false;

    FActionStateBool st = m_InputSystem->GetBoolChannel(handle);
    return st.released;
}

bool InputManager::GetActionHeld(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == INVALID_CHANNEL_HANDLE)
        return false;

    FActionStateBool st = m_InputSystem->GetBoolChannel(handle);
    return st.held;
}

float InputManager::GetAxis(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == INVALID_CHANNEL_HANDLE)
        return 0.0f;

    FActionStateAxis1D st = m_InputSystem->GetAxis1DChannel(handle);
    return st.value;
}

FVector2 InputManager::GetAxis2D(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == INVALID_CHANNEL_HANDLE)
        return FVector2(0.f);

    FActionStateAxis2D st = m_InputSystem->GetAxis2DChannel(handle);
    return {st.x, st.y};
}