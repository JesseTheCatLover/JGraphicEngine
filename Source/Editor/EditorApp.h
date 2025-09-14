//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "EditorContext.h"
#include "ImGuiLayer.h"
#include "GLFW/glfw3.h"
#include "Panels/SceneHierarchyPanel.h"

class EditorContext;

class EditorApp
{
public:
    EditorApp(GLFWwindow* window);
    void BeginFrame();
    void RenderPanels();
    void EndFrame();
    void Shutdown();

private:
    ImGuiLayer m_ImGuiLayer;
    EditorContext m_Context;
    SceneHierarchyPanel m_SceneHierarchyPanel;
};
