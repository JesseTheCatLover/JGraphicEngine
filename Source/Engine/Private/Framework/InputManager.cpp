//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Framework/InputManager.h"
#include "InputSystem/JInputSystem.h"

JInputSystem* InputManager::s_InputSystem = nullptr;
std::unordered_map<std::string, InputChannelHandle> InputManager::s_Cache;

void InputManager::Initialize(JInputSystem *system)
{
    s_InputSystem = system;
    s_Cache.clear();
}

InputChannelHandle InputManager::GetChannelHandle(const std::string &name)
{
    auto it = s_Cache.find(name);
    if (it != s_Cache.end())
        return it->second;

    InputChannelHandle handle = s_InputSystem->FindChannelIdByName(name);
    s_Cache[name] = handle;
    return handle;
}

bool InputManager::GetActionDown(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return false;

    FActionStateBool st = s_InputSystem->GetBoolChannel(handle);
    return st.pressed;
}

bool InputManager::GetActionUp(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return false;

    FActionStateBool st = s_InputSystem->GetBoolChannel(handle);
    return st.released;
}

bool InputManager::GetActionHeld(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return false;

    FActionStateBool st = s_InputSystem->GetBoolChannel(handle);
    return st.held;
}

float InputManager::GetAxis(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return 0.0f;

    FActionStateAxis1D st = s_InputSystem->GetAxis1DChannel(handle);
    return st.value;
}

FVector2 InputManager::GetAxis2D(const std::string& name)
{
    InputChannelHandle handle = GetChannelHandle(name);
    if (handle == static_cast<InputChannelHandle>(-1))
        return FVector2(0.f);

    FActionStateAxis2D st = s_InputSystem->GetAxis2DChannel(handle);
    return {st.x, st.y};
}