//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Memory/SmartPointers.h"

#include <vector>
#include <string>

class IProjectLaunchUI;
class IRenderBackend;
class IPlatformWindow;
class FRenderView;

class IEditorBridge
{
public:
    virtual ~IEditorBridge() = default;
    virtual void OnEngineInitialized(IPlatformWindow* window) = 0;
    virtual void OnSceneLoaded(const std::string& sceneName) = 0;
    virtual void OnRenderOverlay(float deltaTime) = 0;
    virtual void OnTick(float deltaTime) = 0;
};
