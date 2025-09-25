#include "EditorApp.h"

#include <../../Engine/Public/Core/JEngine.h>
#include <GLFW/glfw3.h>
#include <EditorContext.h>
#include <ImGuiLayer.h>
#include <Panels/SceneHierarchyPanel.h>

EditorApp::EditorApp(GLFWwindow* window)
{
    m_Context = std::make_unique<EditorContext>(JEngine::Get().GetState());
    m_ImGuiLayer = std::make_unique<ImGuiLayer>(window);
    m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>();
}

EditorApp::~EditorApp() = default;

void EditorApp::BeginFrame()
{
    m_ImGuiLayer->BeginFrame();
}

void EditorApp::RenderPanels()
{
    m_SceneHierarchyPanel->Draw(*m_Context);
}

void EditorApp::EndFrame()
{
    m_ImGuiLayer->EndFrame();
}

void EditorApp::Shutdown()
{
    m_ImGuiLayer->Shutdown();
}
