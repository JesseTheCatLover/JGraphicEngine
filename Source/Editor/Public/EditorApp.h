#pragma once
#include <memory>

#include "Core/IEditorBridge.h"

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

    void OnEngineInitialized(GLFWwindow* window) override;
    void OnSceneLoaded(const std::string &sceneName) override;
    void OnRenderOverlay() override;
    void OnTick(float deltaTime) override;

private:
    GLFWwindow* m_Window;

    std::unique_ptr<EditorContext> m_Context;
    std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
    std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
};
