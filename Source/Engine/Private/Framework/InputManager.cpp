//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Framework/InputManager.h"
#include "InputSystem/JInputSystem.h"

bool InputManager::Initialize(JInputSystem *system)
{
    if (!system)
        return false;

    m_InputSystem = system;
    m_Cache.clear();

    return true;
}

void InputManager::Tick(float deltaTime)
{
}

InputChannelHandle InputManager::GetChannelHandle(const std::string &name)
{
    auto it = m_Cache.find(name);
    if (it != m_Cache.end())
        return it->second;

    InputChannelHandle handle = m_InputSystem->FindChannelIdByName(name);
    m_Cache[name] = handle;
    return handle;
}

bool InputManager::GetActionDown(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return false;

    FActionStateBool st = m_InputSystem->GetBoolChannel(handle);
    return st.pressed;
}

bool InputManager::GetActionUp(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return false;

    FActionStateBool st = m_InputSystem->GetBoolChannel(handle);
    return st.released;
}

bool InputManager::GetActionHeld(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return false;

    FActionStateBool st = m_InputSystem->GetBoolChannel(handle);
    return st.held;
}

float InputManager::GetAxis(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return 0.0f;

    FActionStateAxis1D st = m_InputSystem->GetAxis1DChannel(handle);
    return st.value;
}

FVector2 InputManager::GetAxis2D(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return FVector2(0.f);

    FActionStateAxis2D st = m_InputSystem->GetAxis2DChannel(handle);
    return {st.x, st.y};
}