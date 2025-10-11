//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>

struct GLFWwindow;

class IEditorBridge
{
public:
    virtual ~IEditorBridge() = default;
    virtual void OnEngineInitialized(GLFWwindow* window) = 0;
    virtual void OnSceneLoaded(const std::string& sceneName) = 0;
    virtual void OnRenderOverlay() = 0;
    virtual void OnTick(float deltaTime) = 0;
};
