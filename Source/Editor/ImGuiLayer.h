//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "GLFW/glfw3.h"

class ImGuiLayer
{
public:
    ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();

    void BeginFrame();
    void EndFrame();
    void Shutdown();

private:
    GLFWwindow* m_Window;
};
