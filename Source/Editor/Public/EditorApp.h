#pragma once
#include <vector>

#include "Core/IEditorBridge.h"
#include "Core/Memory/SmartPointers.h"

class IEditorPanel;
class DockSpace;
class EditorCore;
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

    TUniquePtr<EditorContext> m_Context;
    TUniquePtr<EditorCore> m_Core;
    TUniquePtr<ImGuiLayer> m_ImGuiLayer;

    TUniquePtr<DockSpace> m_DockSpace;

    std::vector<TUniquePtr<IEditorPanel>> m_Panels;
};
