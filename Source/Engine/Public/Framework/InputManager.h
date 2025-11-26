//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_map>
#include "Core/Math/FVector2.h"
#include "InputSystem/InputChannels.h"

class JInputSystem;

class InputManager
{
    friend class JEngine;

private:
    static JInputSystem* s_InputSystem;
    static std::unordered_map<std::string, InputChannelHandle> s_Cache;

public:
    static bool GetActionDown (const std::string& name);
    static bool GetActionUp (const std::string& name);
    static bool GetActionHeld (const std::string& name);

    static float GetAxis(const std::string& name);
    static FVector2 GetAxis2D(const std::string& name);

private:
    static void Initialize(JInputSystem* system);
    static void Shutdown();

    static void Tick(float dt);

    // Helper
    static InputChannelHandle GetChannelHandle(const std::string& name);
};
