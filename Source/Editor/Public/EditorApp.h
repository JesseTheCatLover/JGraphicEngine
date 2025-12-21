#pragma once
#include <vector>

#include "EditorRuntime.h"
#include "Core/IEditorBridge.h"
#include "Core/Memory/SmartPointers.h"

class IEditorPanel;
class DockSpace;
class EditorHost;
struct GLFWwindow;
class EditorContext;
class ImGuiLayer;
class SceneHierarchyPanel;

class EditorApp : public IEditorBridge
{
public:
    EditorApp();
    ~EditorApp();

    void BeginFrame();
    void RenderPanels();
    void EndFrame();

    void Shutdown();

    void OnEngineInitialized(IPlatformSurface* surface) override;
    void OnSceneLoaded(const std::string &sceneName) override;
    void OnRenderOverlay() override;
    void OnTick(float deltaTime) override;

private:
    GLFWwindow* m_Window;

    // Core components
    TUniquePtr<EditorRuntime> m_EditorRuntime;
    TUniquePtr<EditorHost> m_EditorHost;

    // Backend
    TUniquePtr<ImGuiLayer> m_ImGuiLayer;

    // Dock
    TUniquePtr<DockSpace> m_DockSpace;

    // Panel pool
    std::vector<TUniquePtr<IEditorPanel>> m_Panels;
};
