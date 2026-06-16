//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorAPI/Surface/SurfaceAPI.h"

#include "Rendering/IPlatformSurface.h"
#include "Rendering/IPlatformWindow.h"


EditorSurfaceAPI::EditorSurfaceAPI(EngineContext &ctx, IPlatformSurface &surface, InputManager& inputManager)
    : m_Context(ctx), m_PlatformSurface(surface), m_InputManager(inputManager)
{
}

std::string EditorSurfaceAPI::OpenFileDialog(const char *filterList, const char *defaultPath)
{
    return m_PlatformSurface.OpenFileDialog(filterList, defaultPath);
}

std::vector<std::string> EditorSurfaceAPI::OpenFileDialogMultiple(const char *filterList, const char *defaultPath)
{
    return m_PlatformSurface.OpenFileDialogMultiple(filterList, defaultPath);
}

std::string EditorSurfaceAPI::OpenFolderDialog(const char *defaultPath)
{
    return m_PlatformSurface.OpenFolderDialog(defaultPath);
}

std::string EditorSurfaceAPI::SaveFileDialog(const char *filterList, const char *defaultPath, const char *defaultName)
{
    return m_PlatformSurface.SaveFileDialog(filterList, defaultPath, defaultName);
}

TSharedPtr<IPlatformWindow> EditorSurfaceAPI::CreateWindow(const FWindowDesc &desc)
{
    m_PlatformSurface.CreateWindow(desc, false);
}

void EditorSurfaceAPI::DestroyWindow(const TSharedPtr<IPlatformWindow> &window)
{
    m_PlatformSurface.DestroyWindow(window);
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
