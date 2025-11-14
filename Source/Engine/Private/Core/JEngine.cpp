//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "glad/gl.h"

#include "Core/JEngine.h"

#include "Framework/SceneManager.h"
#include "Core/EngineGlobals.h"
#include "Core/Contexts/FViewportContext.h"
#include "Scene/JCamera.h"
#include <iostream>
#include "Core/TServiceContainer.h"

#include "Rendering/BackendFactory.h"
#include "Rendering/EGraphicsAPI.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/JRenderer.h"
#include "Resources/JResourceManager.h"
#include "../Resources/GpuResources/JModelResource.h"
#include "Scene/SceneComponents/JModelComponent.h"

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
    if (!SurfaceInitialize())
    {
        std::cerr << "[JEngine]: Failed to initialize platform surface" << std::endl;
        return false;
    }
    if (!RenderBackendInitialize())
    {
        std::cerr << "[JEngine]: Failed to initialize rendering backend" << std::endl;
        return false;
    }

    GEngine = this;
    RegisterServices();

    if (!InitializeSubsystems())
    {
        std::cerr << "[JEngine]: Failed to initialize subsystems" << std::endl;
        return false;
    }

    if (!InitializeManagers())
    {
        std::cerr << "[JEngine]: Failed to initialize managers" << std::endl;
        return false;
    }

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

bool JEngine::SurfaceInitialize()
{
    FSurfaceState surfaceState;
    surfaceState.width = m_State.GetFramebufferWidth();
    surfaceState.height = m_State.GetFramebufferHeight();
    surfaceState.windowState = EWindowState::Maximized;
    surfaceState.title = "JGraphicEngine";

    ESurfaceAPI surfaceAPI = ESurfaceAPI::GLFW;
    m_PlatformSurface = BackendFactory::MakeSurfaceBackend(surfaceAPI);
    if (!m_PlatformSurface)
    {
        std::cerr << "[JEngine]: No surface backend available for requested API" << std::endl;
        return false;
    }
    if (!m_PlatformSurface->Initialize(surfaceState))
    {
        std::cerr << "[JEngine]: Failed to initialize the platform surface" << std::endl;
        return false;
    }

    m_State.SetFramebufferWidth(m_PlatformSurface->GetWidth());
    m_State.SetFramebufferHeight(m_PlatformSurface->GetHeight());

    auto* win = static_cast<GLFWwindow*>(m_PlatformSurface
                                             ? m_PlatformSurface->GetNativeHandle()
                                             : nullptr);
    if (!win)
    {
        std::cerr << "Win is null. (temp)" << std::endl;
        return false;
    }
    glfwSetCursorPosCallback(win, MouseCallback); // TODO: Make an input system to handle these callbacks
    glfwSetScrollCallback(win, ScrollCallback);
    glfwSetKeyCallback(win, KeyCallback);

    return true;
}

bool JEngine::RenderBackendInitialize()
{
    EGraphicsAPI renderingAPI = EGraphicsAPI::OpenGL;
    m_RenderBackend = BackendFactory::MakeRenderBackend(renderingAPI);
    if (!m_RenderBackend)
    {
        std::cerr << "[JEngine]: No render backend available for requested API" << std::endl;
        return false;
    }
    if (!m_RenderBackend->Initialize(m_PlatformSurface.get()))
    {
        std::cerr << "[JEngine]: Failed to initialize rendering backend" << std::endl;
        return false;
    }

    return true;
}

bool JEngine::InitializeSubsystems()
{
    m_Renderer = TUniquePtr<JRenderer>(new JRenderer(m_RenderBackend.get()));
    if (!m_Renderer)
    {
        std::cerr << "[JEngine]: Failed to initialize renderer" << std::endl;
        return false;
    }
    m_Renderer->SetPostProcessManager(GetPostProcessManager());
    JResourceManager::Get().SetRenderDevice(m_Renderer.get());

    return true;
}

bool JEngine::InitializeManagers()
{

    return true;
}

void JEngine::RunMainLoop()
{
    auto* win = static_cast<GLFWwindow*>(m_PlatformSurface // TODO: Temporarily here until InputSystem is defined
                                             ? m_PlatformSurface->GetNativeHandle()
                                             : nullptr);
    if (!win) {
        std::cerr << "[JEngine]: No glfw window was found to run the main loop" << std::endl;
        return;
    }

    while (!m_PlatformSurface->ShouldClose() && m_State.GetIsRunning())
    {
        CalculateDeltaTime();
        UpdateFramebufferSizeContext();

        ProcessInputs(win, m_State.GetDeltaTime());

        Tick();
        m_Renderer->BeginScene();
        FRenderContext ctx{};
        if (auto* scene = JEngine::Get().GetSceneManager()->GetActiveScene())
        {
            scene->GatherRenderables(GetRenderer()->GetSubmission(), ctx);
        }
        m_Renderer->EndScene();
        
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
    JResourceManager::Get().Shutdown();
    m_Renderer->Shutdown();
    m_PlatformSurface->Shutdown();
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
        m_PlatformSurface->SetShouldClose(true);

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

IPlatformSurface * JEngine::GetPlatformSurface()
{
    return m_PlatformSurface.get();
}

JRenderer* JEngine::GetRenderer()
{
    return m_Renderer.get();
}

SceneManager* JEngine::GetSceneManager()
{
    return m_Services->GetService<SceneManager>().get();
}

PostProcessManager* JEngine::GetPostProcessManager()
{
    return m_Services->GetService<PostProcessManager>().get();
}

void JEngine::RegisterServices()
{
    m_Services->RegisterFactory<PostProcessManager>([this]() -> std::shared_ptr<PostProcessManager>
    {
        return std::make_shared<PostProcessManager>();
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
    CreateDefaultScene();
    return true;
    auto* startupScene = GetSceneManager()->LoadSceneFile("StartupScene");
    if (!startupScene)
    {
        CreateDefaultScene();
    }
    return true;
}

void JEngine::CreateDefaultScene() // TEMP bootstrap; will be replaced by proper scene loading
{
    constexpr const char* kSceneKey  = "StartupScene";
    constexpr const char* kSceneName = "StartupScene";

    if (!GetSceneManager())
    {
        std::cerr << "[JEngine] CreateStartupScene: SceneManager is null.\n";
        return;
    }

    // ---------------------------------------------------------------------
    // 1) (Optional) Preload a few heavy assets to smooth first-frame stutter
    //    Keys should be the same strings you’ll use from components.
    // ---------------------------------------------------------------------
    JResourceManager::Get().Load<JModelResource>("Dio Brando/DioMansion.obj",      "Dio Brando/DioMansion.obj");
    JResourceManager::Get().Load<JModelResource>("MedievalWindow/MedievalWindow.obj","MedievalWindow/MedievalWindow.obj");

    // ---------------------------------------------------------------------
    // 2) Load or create the startup scene asset
    // ---------------------------------------------------------------------
    auto* scene = GetSceneManager()->LoadSceneFile(kSceneKey);
    if (!scene)
    {
        const bool created = GetSceneManager()->CreateSceneFile(kSceneKey, kSceneName, /*setActive*/true);
        if (!created)
        {
            std::cerr << "[JEngine] CreateStartupScene: failed to create scene file.\n";
            return;
        }
        scene = GetSceneManager()->LoadSceneFile(kSceneKey);
        if (!scene)
        {
            std::cerr << "[JEngine] CreateStartupScene: failed to load freshly created scene.\n";
            return;
        }
    }

    // ---------------------------------------------------------------------
    // 3) Utility: spawn a simple “Model Actor” with one JModelComponent
    // ---------------------------------------------------------------------
    auto spawnModelActor = [&](const char* actorName, const char* modelKey) -> JActor*
    {
        JActor* actor = GetSceneManager()->SpawnActor<JActor>();
        if (!actor) return nullptr;

        actor->SetName(actorName ? actorName : "ModelActor");

        // Attach a model component at runtime to the actor’s root (uses route rendering)
        auto* modelComp = actor->AddRuntimeComponent<JModelComponent>();
        modelComp->SetModel(modelKey);

        return actor;
    };

    // ---------------------------------------------------------------------
    // 4) Populate scene (temporary, hardcoded)
    // ---------------------------------------------------------------------
    auto actor1 = spawnModelActor("Dio Mansion",    "Dio Brando/DioMansion.obj");
    auto actor2 = spawnModelActor("MedievalWindow", "MedievalWindow/MedievalWindow.obj");
    actor1->SetActorPosition({10.f, 0.f, 5.f});
    actor2->SetActorPosition({-10.f, 0.f, 0.f});


    // ---------------------------------------------------------------------
    // 5) Save the scene so next launch restores this layout
    // ---------------------------------------------------------------------
    GetSceneManager()->SaveSceneFile(scene, kSceneKey);
}

void JEngine::CalculateDeltaTime()
{
    auto currentFrame = static_cast<float>(glfwGetTime());
    m_State.SetDeltaTime(currentFrame - m_State.GetLastFrameTime());
    m_State.SetLastFrameTime(currentFrame);
}

void JEngine::UpdateFramebufferSizeContext()
{
    int fbW = 0, fbH = 0;
    m_PlatformSurface->GetFramebufferSize(fbW, fbH);
    m_State.SetFramebufferWidth(fbW);
    m_State.SetFramebufferHeight(fbH);
}

// --- Static Callbacks ---
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