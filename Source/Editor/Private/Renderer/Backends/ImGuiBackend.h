//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <GLFW/glfw3.h>

#include "IEditorUIBackend.h"

class ImGuiBackend : public IEditorUIBackend
{
public:
    ImGuiBackend(GLFWwindow* window);
    ~ImGuiBackend();

    void BeginFrame() override;
    void EndFrame() override;
    void Shutdown() override;

private:
    GLFWwindow* m_Window;

    void SetupFonts();
    void SetupStyle();
};
