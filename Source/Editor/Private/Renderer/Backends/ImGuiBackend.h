//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <GLFW/glfw3.h>
#include <string>

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
    std::string m_IniPath;
    bool m_HasLoadedSettings = false;
    bool m_DefaultLayoutBuilt = false;

    void SetupFonts();
    void SetupStyle();
    void SaveSettings();
    bool LoadSettings();
};
