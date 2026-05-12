//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Surface/SurfaceAPI.h"

#include "Rendering/IPlatformSurface.h"
#include "Rendering/IPlatformWindow.h"


EditorSurfaceAPI::EditorSurfaceAPI(EngineContext &ctx, IPlatformSurface &surface, InputManager& inputManager)
    : m_Context(ctx), m_PlatformSurface(surface), m_InputManager(inputManager)
{
}

void EditorSurfaceAPI::SetCursorDisabled()
{
    m_PlatformSurface.GetEffectiveInputWindow()->SetCursorDisabled();
}

void EditorSurfaceAPI::SetCursorHidden()
{
    m_PlatformSurface.GetEffectiveInputWindow()->SetCursorHidden();
}

void EditorSurfaceAPI::SetCursorVisible()
{
    m_PlatformSurface.GetEffectiveInputWindow()->SetCursorVisible();
}
