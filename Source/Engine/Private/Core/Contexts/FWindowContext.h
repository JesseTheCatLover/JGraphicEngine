//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "GLFW/glfw3.h"

struct FWindowContext
{
    GLFWwindow* window = nullptr;
    int width = 1080;
    int height = 1920;
    bool bFullscreen = true;

    // Store the monitor pointer for fullscreen queries
    GLFWmonitor* monitor = nullptr;

    void SetFullscreenSize()
    {
        if (monitor)
        {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (mode)
            {
                width = mode->width;
                height = mode->height;
            }
        }
    }
};
