//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JEngine.h"

// Global engine pointer
extern JEngine* GEngine;

inline SceneManager* GetSceneManager()
{
    return (GEngine ? GEngine->GetSceneManager() : nullptr);
}

inline PostProcessManager* GetPostProcessManager()
{
    return (GEngine ? GEngine->GetPostProcessManager() : nullptr);
}

inline InputManager* GetInputManager()
{
    return (GEngine ? GEngine->GetInputManager() : nullptr);
}

inline DebugDraw* GetDebugDraw()
{
    return (GEngine ? GEngine->GetDebugDraw() : nullptr);
}