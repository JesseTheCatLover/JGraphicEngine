//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class InputManager;
enum class ECursorMode;
class IPlatformSurface;
class EngineContext;

class EditorSurfaceAPI
{
private:
    EngineContext& m_Context;
    IPlatformSurface& m_PlatformSurface;
    InputManager& m_InputManager;

public:
    EditorSurfaceAPI(EngineContext& ctx, IPlatformSurface& surface, InputManager& inputManager);

    void SetCursorDisabled();
    void SetCursorHidden();
    void SetCursorVisible();

    InputManager& GetInputManager() const { return m_InputManager; }
};
