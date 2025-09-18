#pragma once
#include <memory>

struct GLFWwindow;
class EditorContext;
class ImGuiLayer;
class SceneHierarchyPanel;

class EditorApp
{
public:
    EditorApp(GLFWwindow* window);
    ~EditorApp();

    void BeginFrame();
    void RenderPanels();
    void EndFrame();
    void Shutdown();

private:
    std::unique_ptr<EditorContext> m_Context;
    std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
    std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
};
