//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "BackendFactory.h""
#include <iostream>
#include "EGraphicsAPI.h"

// Build flags set by CMake based on availability
// add_compile_definitions(HAVE_GLFW) etc.
#if defined(HAVE_GLFW)
  #include "Rendering/Surfaces/GLFWSurface.h"
#endif

TUniquePtr<IPlatformSurface> BackendFactory::MakeSurfaceBackend(ESurfaceAPI api)
{
    switch (api) {

#if defined(HAVE_GLFW)
        case ESurfaceAPI::GLFW:
            return MakeUnique<GLFWSurface>();
#else
        case ESurfaceAPI::GLFW:
            std::cerr << "[BackendFactory] GLFW requested but not compiled; falling back" << std::endl;
            break;
#endif

        default:
#if defined(HAVE_GLFW)
            std::cerr << "[BackendFactory] Using GLFW as default fallback" << std::endl;
            return MakeUnique<GLFWSurface>();
#endif
            std::cerr << "[BackendFactory] No available backend to return" << std::endl;
            return nullptr;
    }
}

// Build flags set by CMake based on availability
// add_compile_definitions(HAVE_OPENGL) etc.
#if defined(HAVE_OPENGL)
  #include "Rendering/Backends/GLBackend.h"
#endif
#if defined(HAVE_VULKAN)
  #include "Rendering/Backends/VulkanBackend.h"
#endif
#if defined(HAVE_METAL)
  #include "Rendering/Backends/MetalBackend.h"
#endif
#if defined(HAVE_DIRECTX)
  #include "Rendering/Backends/DirectXBackend.h"
#endif
//#include "Rendering/Backends/NullBackend.h" // trivial no-op backend

TUniquePtr<IRenderBackend> BackendFactory::MakeRenderBackend(EGraphicsAPI api)
{
    switch (api) {

#if defined(HAVE_OPENGL)
        case EGraphicsAPI::OpenGL:
            return MakeUnique<GLBackend>();
#else
        case EGraphicsAPI::OpenGL:
            std::cerr << "[BackendFactory] OpenGL requested but not compiled; falling back" << std::endl;
            break;
#endif

#if defined(HAVE_VULKAN)
        case EGraphicsAPI::Vulkan:
            return MakeUnique<VulkanBackend>();
#else
        case EGraphicsAPI::Vulkan:
            std::cerr << "[BackendFactory] Vulkan requested but not compiled; falling back" << std::endl;
            break;
#endif

#if defined(HAVE_METAL)
        case EGraphicsAPI::Metal:
            return MakeUnique<MetalBackend>();
#else
        case EGraphicsAPI::Metal:
            std::cerr << "[BackendFactory] Metal requested but not compiled; falling back" << std::endl;
            break;
#endif

#if defined(HAVE_DIRECTX)
        case EGraphicsAPI::DirectX:
            return MakeUnique<DirectXBackend>();
#else
        case EGraphicsAPI::DirectX:
            std::cerr << "[BackendFactory] DirectX requested but not compiled; falling back" << std::endl;
            break;
#endif
        default:
#if defined(HAVE_OPENGL)
            std::cerr << "[BackendFactory] Using OpenGL as default fallback" << std::endl;
            return MakeUnique<GLBackend>();
#endif
            std::cerr << "[BackendFactory] No available backend to return" << std::endl;
            return nullptr;
    }
}