//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "glad/gl.h"

#include "Core/JEngine.h"

#include "Framework/SceneManager.h"
#include "Core/EngineGlobals.h"
#include "Core/Contexts/FViewportContext.h"
#include "../Rendering/Legacy/JRendererLegacy.h"
#include "Framework/PostProcessManager.h"
#include "Scene/JCamera.h"
#include <iostream>
#include "Core/TServiceContainer.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/JRenderer.h"
#include "Rendering/Backends/GLBackend.h"
#include "Rendering/Surfaces/GLFWSurface.h"
#include "Resources/JResourceManager.h"
#include "Resources/JModelResource.h"
#include "Scene/Components/Scene/JModelComponent.h"

class JModelResource;

JEngine::JEngine()
    : m_Services(MakeUnique<TServiceContainer>())
{
}

JEngine::~JEngine()
{
}

bool JEngine::Run()
{
    if (!Initialize())
    {
        std::cerr << "[JEngine]: Initialization of the engine has failed" << std::endl;
        return false;
    }

    // Bootstrap default scene
    if (!BootstrapScene())
    {
        std::cerr << "[JEngine]: Bootstrapping the default scene has failed" << std::endl;
        return false;
    }

    RunMainLoop();
    Shutdown();

    return true;
}

bool JEngine::Initialize()
{
    if (!SurfaceInitialize()) // TODO: Should be replaced by the SurfaceInitialize() and the new JRenderer should own a IRenderBackend and feed the surface to it.
    {
        std::cerr << "[JEngine]: Failed to initialize platform surface" << std::endl;
        return false;
    }

    GEngine = this;

    if (!InitializeSubsystems())
    {
        std::cerr << "[JEngine]: Failed to initialize subsystems" << std::endl;
        return false;
    }

    if (!InitializeManagers()) return false;

    if (m_EditorBridge)
    {
        // Get the SAME window created by GLFWSurface TODO: Temp test here
        auto* win = static_cast<GLFWwindow*>(m_PlatformSurface->GetNativeHandle());
        if (!win) {
            std::cerr << "[JEngine]: Native GLFWwindow* is null!\n";
            return false;
        }
        m_EditorBridge->OnEngineInitialized(win);
    }

    return true;
}

bool JEngine::GLFWInitialize()
{
    // ----------------- GLFW Init -----------------
    if (!glfwInit()) {
        std::cerr << "[JEngine]: Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Get fullscreen settings from state/context
    bool bFullscreen = m_State.GetIsWindowFullscreen();
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    GLFWwindow* Window = nullptr;

    if (bFullscreen && primaryMonitor && mode)
    {
        // --- Set hints for a borderless fullscreen window ---
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);  // title bar
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);   // stays behind other floating windows

        m_State.SetWindowWidth(mode->width);
        m_State.SetWindowHeight(mode->height);

        Window = glfwCreateWindow(
            mode->width,
            mode->height,
            "Jesse's Magical Workshop",
            nullptr,  // Attach to monitor for fullscreen
            nullptr
        );
    }
    else
    {
        // --- Windowed fallback ---
        Window = glfwCreateWindow(
            m_State.GetWindowWidth(),
            m_State.GetWindowHeight(),
            "Jesse's Magical Workshop",
            nullptr,
            nullptr
        );
    }

    if (!Window) {
        std::cout << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return false;
    }

    // Make context current *before* loading GLAD
    glfwMakeContextCurrent(Window);

    m_State.SetGLFWWindow(Window);

    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(Window, MouseCallback);
    glfwSetScrollCallback(Window, ScrollCallback);
    glfwSetKeyCallback(Window, KeyCallback);

    // Capture and hide the cursor
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(Window, m_State.GetLastMouseX(), m_State.GetLastMouseY());

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    return true;
}

bool JEngine::SurfaceInitialize()
{
    FSurfaceState surfaceState;
    surfaceState.width = m_State.GetWindowWidth();
    surfaceState.height = m_State.GetWindowHeight();
    surfaceState.windowState = EWindowState::Maximized;
    surfaceState.title = "JGraphicEngine";

    // TODO: Hardcoded backend selector for now, should select the backend for future.
    m_PlatformSurface = MakeUnique<GLFWSurface>();

    if (!m_PlatformSurface->Initialize(surfaceState))
        return false;

    return true;
}

bool JEngine::InitializeSubsystems()
{
    m_RenderBackend = MakeUnique<GLBackend>(); // TODO : TEMPORARY HERE
    if (!m_RenderBackend)
    {
        std::cerr << "[JEngine]: Failed to initialize backend renderer" << std::endl;
        return false;
    }
    m_RenderBackend->Initialize(m_PlatformSurface.get());
    m_Renderer = TUniquePtr<JRenderer>(new JRenderer(m_RenderBackend.get()));
    if (!m_Renderer)
    {
        std::cerr << "[JEngine]: Failed to initialize renderer" << std::endl;
        return false;
    }

    return true;

}

bool JEngine::InitializeManagers()
{
    RegisterServices();

    return true;
}

void JEngine::RunMainLoop()
{
    auto* win = static_cast<GLFWwindow*>(m_PlatformSurface
                                             ? m_PlatformSurface->GetNativeHandle()
                                             : nullptr);
    if (!win) {
        std::cerr << "[JEngine]: No glfw window was found to run the main loop" << std::endl;
        return;
    }

    while (!glfwWindowShouldClose(win) && m_State.GetIsRunning())
    {
        auto currentFrame = static_cast<float>(glfwGetTime());
        m_State.SetDeltaTime(currentFrame - m_State.GetLastFrameTime());
        m_State.SetLastFrameTime(currentFrame);

        ProcessInputs(win, m_State.GetDeltaTime());

        Tick();
        GetRenderer()->BeginScene();
        GetRenderer()->EndScene();

        // int fbW, fbH;
        // glfwGetFramebufferSize(m_State.GetGLFWWindow(), &fbW, &fbH);
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glViewport(0, 0, fbW, fbH);
        //
        // if (auto* ppm = GetPostProcessManager())
        //     ppm->ApplyChain(GetRenderer()->GetSceneTargetTexture(), fbW, fbH);
        m_State.SetViewMode(EViewMode::UI);
        if (m_EditorBridge)
            m_EditorBridge->OnRenderOverlay();

        // Swap front/back buffers (show rendered frame)
        m_PlatformSurface->SwapBuffers();

        // Poll window + input events
        m_PlatformSurface->PollSurfaceEvents();
    }
}

void JEngine::Shutdown()
{
    GEngine = nullptr;
    m_Renderer->Shutdown();
}

void JEngine::Tick()
{
    auto& deltaTime = m_State.GetDeltaTime();

    auto* sceneMgr = GetSceneManager();
    if (sceneMgr)
        sceneMgr->Tick(deltaTime);

    if (m_EditorBridge)
        m_EditorBridge->OnTick(deltaTime);
}

void JEngine::ProcessInputs(GLFWwindow* window, float deltaTime)
{
    // close engine if ESC is pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        m_State.SetRunning(false);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_State.GetCamera()->ProcessKeyboard(ECM_Forward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_State.GetCamera()->ProcessKeyboard(ECM_Backward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_State.GetCamera()->ProcessKeyboard(ECM_Left, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_State.GetCamera()->ProcessKeyboard(ECM_Right, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) m_State.GetCamera()->ProcessKeyboard(ECM_Up, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) m_State.GetCamera()->ProcessKeyboard((ECM_Down), deltaTime);
}

void JEngine::OnFramebufferResize(int width, int height)
{
    glViewport(0, 0, width, height);
    // if (auto Renderer = GetRenderer())
    //     Renderer->Resize(width, height);
}

void JEngine::OnMouseMove(double xPosIn, double yPosIn)
{
    if (m_State.GetViewMode() != EViewMode::Scene) return;

    const float xPos = static_cast<float>(xPosIn);
    const float yPos = static_cast<float>(yPosIn);

    float xOffset = xPos - m_State.GetLastMouseX();
    float yOffset = m_State.GetLastMouseY() - yPos; // reversed since y-coordinates go from bottom to top

    m_State.SetLastMouseX(xPos);
    m_State.SetLastMouseY(yPos);

    m_State.GetCamera()->ProcessMouseMovement(xOffset, yOffset);
}

void JEngine::OnScroll(double xOffset, double yOffset)
{
    m_State.GetCamera()->ProcessMouseScroll(static_cast<float>(yOffset),
        m_State.GetCameraSettings()->GetMaxFOV());
}

void JEngine::OnKeyboardAction(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_J && action == GLFW_PRESS)
    {
        if (m_State.GetViewMode() == EViewMode::Scene)
        {
            glfwSetCursorPos(window, m_State.GetLastMouseX(), m_State.GetLastMouseY());
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_State.SetViewMode(EViewMode::UI);
        } else if (m_State.GetViewMode() == EViewMode::UI) {
            glfwSetCursorPos(window, m_State.GetLastMouseX(), m_State.GetLastMouseY());
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_State.SetViewMode(EViewMode::Scene);
        }
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        m_State.SetWireframeMode(!m_State.GetWireframeMode()); // Toggling the wireframe mode
}

JRenderer* JEngine::GetRenderer()
{
    return m_Renderer.get();
}

SceneManager* JEngine::GetSceneManager()
{
    return m_Services->GetService<SceneManager>().get();
}

PostProcessManager * JEngine::GetPostProcessManager()
{
    return m_Services->GetService<PostProcessManager>().get();
}

void JEngine::RegisterServices()
{
    m_Services->RegisterFactory<PostProcessManager>([this]() -> std::shared_ptr<PostProcessManager>
    {
        return std::make_shared<PostProcessManager>(m_State.GetWindowWidth(), m_State.GetWindowHeight());
    });

    m_Services->RegisterFactory<SceneManager>([]() -> std::shared_ptr<SceneManager>
    {
        return std::make_shared<SceneManager>();
    });

    // Optionally: eager creation for deterministic startup
    m_Services->GetService<PostProcessManager>();
    m_Services->GetService<SceneManager>();
}

bool JEngine::BootstrapScene()
{
    auto* defaultScene = GetSceneManager()->LoadSceneFile("DefaultScene");
    if (!defaultScene)
    {
        CreateDefaultScene();
    }
    return true;
}

void JEngine::CreateDefaultScene()
{
    auto& rm = JResourceManager::Get();
    auto* sceneManager = GetSceneManager();

    // --- 1. Preload commonly used models (optional, speeds up first draw) ---
    rm.Load<JModelResource>("DioMansion", "Dio Brando/DioMansion.obj");
    rm.Load<JModelResource>("MedievalWindow", "MedievalWindow/MedievalWindow.obj");

    // --- 2. Load startup scene ---
    auto* startupScene = sceneManager->LoadSceneFile("StartupScene");
    if (!startupScene)
    {
        // Scene doesn't exist, create a default one
        bool bCreated = sceneManager->CreateSceneFile("StartupScene", "StartupScene", true);
        if (!bCreated) return; // could not create
        startupScene = sceneManager->LoadSceneFile("StartupScene");
    }

    if (!startupScene) return; // failed to load or create

    // --- 3. Populate scene actors (temporary hardcoded setup) ---

    // Example: create a Dio Mansion actor
    auto* dioActor = SceneManager().SpawnActor<JActor>();
    auto* dioComp = dioActor->CreateDefaultComponent<JModelComponent>("DioMansion");
    dioComp->SetModel("Dio Brando/DioMansion.obj");

    // Example: create a Medieval Window actor
    auto* windowActor = SceneManager().SpawnActor<JActor>();
    auto* windowComp = windowActor->CreateDefaultComponent<JModelComponent>("MedievalWindow");
    windowComp->SetModel("MedievalWindow/MedievalWindow.obj");

    // Save the scene for next launch
    sceneManager->SaveSceneFile(startupScene, "StartupScene");

    // At this point, every actor's JModelComponent already knows its model via the path
    // No need to manually fetch from ResourceManager or assign
}


// --- Static Callbacks ---
void JEngine::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Get().OnFramebufferResize(width, height);
}

void JEngine::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Get().OnMouseMove(xpos, ypos);
}

void JEngine::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Get().OnScroll(xoffset, yoffset);
}

void JEngine::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Get().OnKeyboardAction(window, key, scancode, action, mods);
}