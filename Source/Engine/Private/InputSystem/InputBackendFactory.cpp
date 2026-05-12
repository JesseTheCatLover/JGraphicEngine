//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "InputBackendFactory.h"
#include <iostream>

#if defined(HAVE_GLFW)
    #include "../Rendering/Surfaces/GLFWSurface.h"
    #include "Backends/GLFWInputBackend.h"
#endif

TUniquePtr<IInputBackend> InputBackendFactory::MakeInputBackend(IPlatformWindow* window)
{
    if (!window)
    {
        std::cerr << "[InputBackendFactory] No window provided, cannot create input backend\n";
        return nullptr;
    }

#if JENGINE_PLATFORM_MACOS || JENGINE_PLATFORM_LINUX || JENGINE_PLATFORM_WINDOWS
    // Desktop builds – currently using GLFW everywhere
#if defined(HAVE_GLFW)
    void* nativeHandle = window->GetNativeHandle();
    auto* glfwWindow = reinterpret_cast<GLFWwindow*>(nativeHandle);
    if (!glfwWindow)
    {
        std::cerr << "[InputBackendFactory]: GLFW native handle is null\n";
        return nullptr;
    }

    return MakeUnique<GLFWInputBackend>(glfwWindow);
#else
    std::cerr << "[InputBackendFactory]: Desktop platform but GLFW not compiled in\n";
    return nullptr;
#endif

#else
    // Future: consoles / mobile, custom backends, etc.
    std::cerr << "[InputBackendFactory]: No input backend for this platform\n";
    return nullptr;
#endif
}