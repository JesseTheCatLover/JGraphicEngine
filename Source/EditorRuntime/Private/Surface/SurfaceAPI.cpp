//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Surface/SurfaceAPI.h"

#include "Rendering/IPlatformSurface.h"


EditorSurfaceAPI::EditorSurfaceAPI(EngineContext &ctx, IPlatformSurface &surface)
    : m_Context(ctx), m_PlatformSurface(surface)
{
}

void EditorSurfaceAPI::SetCursorDisabled()
{
    m_PlatformSurface.SetCursorDisabled();
}

void EditorSurfaceAPI::SetCursorHidden()
{
    m_PlatformSurface.SetCursorHidden();
}

void EditorSurfaceAPI::SetCursorVisible()
{
    m_PlatformSurface.SetCursorVisible();
}
