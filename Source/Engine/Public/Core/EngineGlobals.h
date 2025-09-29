//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JEngine.h"

// Global engine pointer
extern JEngine* GEngine;

inline SceneManager* GetSceneManager()
{
    return (GEngine) ? GEngine->GetService<SceneManager>() : nullptr;
}

inline PostProcessManager* GetPostProcessManager()
{
    return (GEngine) ? GEngine->GetService<PostProcessManager>() : nullptr;
}
