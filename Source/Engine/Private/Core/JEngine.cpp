//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/JEngine.h"

#include "Framework/SceneManager.h"
#include "Core/EngineGlobals.h"
#include "Core/Contexts/FViewportContext.h"
#include <iostream>
#include "Core/TServiceContainer.h"
#include "Framework/InputManager.h"
#include "Framework/PostProcessManager.h"
#include "InputSystem/InputBackendFactory.h"
#include "InputSystem/JInputSystem.h"

#include "Rendering/BackendFactory.h"
#include "Rendering/EGraphicsAPI.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/JRenderer.h"
#include "Resources/JResourceSystem.h"
#include "Resources/GpuResources/JModelResource.h"
#include "Scene/SceneComponents/JCameraComponent.h"
#include "Scene/SceneComponents/JModelComponent.h"

#include "GLFW/glfw3.h"

#include "InputSystem/MappingStyles/ActionAxis/ActionAxisConfig.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStyle.h"

namespace
{
    bool  gLookInitialized = false;
    float gYaw   = 0.0f; // radians
    float gPitch = 0.0f; // radians
}

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
    if (!InitializeBackends())
    {
        std::cerr << "[JEngine]: Failed to initialize backends" << std::endl;
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

    return true;
}

bool JEngine::InitializeBackends()
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

    m_InputBackend = InputBackendFactory::MakeInputBackend(m_PlatformSurface.get());
    if (!m_InputBackend)
    {
        std::cerr << "[JEngine]: No input backend available for the os/platform" << std::endl;
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

    m_ResourceSystem = TUniquePtr<JResourceSystem>(new JResourceSystem());
    if (!m_ResourceSystem)
    {
        std::cerr << "[JEngine]: Failed to initialize resource subsystem" << std::endl;
        return false;
    }
    m_ResourceSystem->SetRenderDevice(m_Renderer.get());

    m_InputSystem = TUniquePtr<JInputSystem>(new JInputSystem());
    if (!m_InputSystem)
    {
        std::cerr << "[JEngine]: Failed to initialize input subsystem" << std::endl;
        return false;
    }
    m_InputSystem->Initialize(m_InputBackend.get());

    // --------- Default action/axis mapping ---------
    FActionAxisMap map;

    auto addButton = [&](const char* name, EPhysicalInput input)
    {
        FActionAxisSlot slot;
        slot.name = name;
        slot.type = EInputChannelType::Bool;

        FInputBinding bind{};
        bind.deviceType  = EInputDeviceType::Keyboard;
        bind.deviceIndex = 0;
        bind.input       = input;
        bind.scale       = 1.0f;
        bind.deadZone    = 0.0f;
        bind.invert      = false;

        slot.bindings.push_back(bind);
        map.actions.push_back(std::move(slot));
    };

    // ESC -> Quit
    addButton("Quit", EPhysicalInput::Key_Escape);
    // F   -> Toggle wireframe
    addButton("ToggleWireframe", EPhysicalInput::Key_F);
    // J   -> Toggle view mode (Scene/UI)
    addButton("ToggleViewMode", EPhysicalInput::Key_J);

    // Mouse look: Axis2D
    {
        FActionAxisSlot look{};
        look.name = "Look";
        look.type = EInputChannelType::Axis2D;

        // X = mouse delta X
        FInputBinding bx{};
        bx.deviceType  = EInputDeviceType::Mouse;
        bx.deviceIndex = 0;
        bx.input       = EPhysicalInput::Mouse_DeltaX;
        bx.scale       = 0.0038f;    // sensitivity
        bx.deadZone    = 0.0f;
        bx.invert      = false;

        // Y = mouse delta Y
        FInputBinding by{};
        by.deviceType  = EInputDeviceType::Mouse;
        by.deviceIndex = 0;
        by.input       = EPhysicalInput::Mouse_DeltaY;
        by.scale       = 0.0038f;
        by.deadZone    = 0.0f;
        by.invert      = false;       // invert so moving mouse up is positive pitch

        look.bindings.push_back(bx);
        look.bindings.push_back(by);
        map.actions.push_back(std::move(look));
    }

    // Install mapping style
    m_InputSystem->SetMappingStyle(MakeUnique<ActionAxisStyle>(map));
    // ----------------------------------------------------


    return true;
}

bool JEngine::InitializeManagers()
{
    if (!m_Services->GetService<InputManager>()->Initialize(m_InputSystem.get()))
    {
        std::cerr << "[JEngine]: Failed to initialize InputManager" << std::endl;
        return false;
    }

    // ---- Bind a few test callbacks ----
    InputManager* input = GetInputManager();
    if (!input)
        return false;

    // ESC -> stop main loop
    input->BindAction("Quit", EInputEventPhase::Pressed,
        [this](InputChannelHandle, const FActionStateBool&)
        {
            std::cout << "[Input] Quit pressed -> stopping engine\n";
            m_State.SetRunning(false);
        });

    // F -> toggle wireframe
    input->BindAction("ToggleWireframe", EInputEventPhase::Pressed,
        [this](InputChannelHandle, const FActionStateBool&)
        {
            bool wf = !m_State.GetWireframeMode();
            m_State.SetWireframeMode(wf);
            std::cout << "[Input] ToggleWireframe -> " << (wf ? "ON" : "OFF") << "\n";
        });

    // J -> toggle view mode + cursor
    input->BindAction("ToggleViewMode", EInputEventPhase::Pressed,
        [this](InputChannelHandle, const FActionStateBool&)
        {
            if (!m_PlatformSurface)
                return;

            if (m_State.GetViewMode() == EViewMode::Scene)
            {
                m_State.SetViewMode(EViewMode::UI);
                m_PlatformSurface->SetCursorMode(ECursorMode::Visible);
                std::cout << "[Input] ViewMode -> UI\n";
            }
            else
            {
                m_State.SetViewMode(EViewMode::Scene);
                m_PlatformSurface->SetCursorMode(ECursorMode::Disabled);
                std::cout << "[Input] ViewMode -> Scene\n";
            }
        });


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
            scene->GatherRenderables(m_Renderer->GetSubmission(), ctx);
        }
        if (auto* scene = JEngine::Get().GetSceneManager()->GetActiveScene())
        {
            auto camera = scene->GetCameraComponent();
            if (camera)
            {
                m_State.SetCamera(camera);
            }
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
    m_Renderer->Shutdown();
    m_ResourceSystem->Shutdown();
    m_InputSystem->Shutdown();
    m_PlatformSurface->Shutdown();
}

void JEngine::Tick()
{
    auto& deltaTime = m_State.GetDeltaTime();

    if (GetSceneManager())
        GetSceneManager()->Tick(deltaTime);

    if (m_InputSystem)
        m_InputSystem->Tick(deltaTime);

    // --- Apply look axis to camera ---
    auto* sceneMgr = GetSceneManager();
    if (sceneMgr && m_State.GetViewMode() == EViewMode::Scene)
    {
        auto* scene = sceneMgr->GetActiveScene();
        if (scene)
        {
            auto* camera = scene->GetCameraComponent();
            if (camera)
            {
                JActor* camActor = camera->GetOwnerActor();
                if (camActor)
                {
                    // Initialize yaw/pitch from current camera rotation once
                    if (!gLookInitialized)
                    {
                        FTransform world = camera->GetWorldTransform();
                        FEuler rotation = world.GetRotation().ToEuler();

                        gPitch = rotation.Pitch;
                        gYaw   = rotation.Yaw;
                        gLookInitialized = true;
                    }

                    FVector2 lookDelta = GetInputManager()->GetAxis2D("Look");
                    float dx = lookDelta.x;
                    float dy = lookDelta.y;

                    if (dx != 0.0f || dy != 0.0f)
                    {
                        gYaw   += dx;
                        gPitch -= dy;

                        gPitch = FMath::Radians(
                            FMath::Clamp(FMath::Degrees(gPitch), -90.f, 90.f));

                        FEuler euler(-gPitch, gYaw, 0.f);
                        FQuat newRot = euler.ToQuat();
                        camera->SetWorldRotation(newRot);
                    }
                }
            }
        }
    }
    // --------------------------------------

    if (m_EditorBridge)
        m_EditorBridge->OnTick(deltaTime);
}

void JEngine::ProcessInputs(GLFWwindow* window, float deltaTime)
{
    // Close engine if ESC is pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        m_State.SetRunning(false);
    }

    //  TODO: TEMP: drive active camera with WASD/Space/Shift

    auto* sceneMgr = GetSceneManager();
    if (!sceneMgr) return;

    auto* scene = sceneMgr->GetActiveScene();
    if (!scene) return;

    auto* camera = scene->GetCameraComponent();
    if (!camera) return;

    JActor* camActor = camera->GetOwnerActor();
    if (!camActor) return;

    const float moveSpeed = 15.0f;
    FVector3 movement(0.f, 0.f, 0.f);

    // Move along camera local axes
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += camera->GetForwardVector();
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        GetSceneManager()->SaveSceneFile(GetSceneManager()->GetActiveScene(),"StartupScene");
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= camera->GetForwardVector();
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += camera->GetRightVector();
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= camera->GetRightVector();
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        movement += FVector3::Up();
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        movement -= FVector3::Up();

    if (movement.Length() > 0.0f)
    {
        movement = movement.Normalized() * moveSpeed * deltaTime;

        FVector3 pos = camActor->GetActorLocation();
        camActor->SetActorLocation(pos + movement);

        camera->RecalculateViewMatrix();
        camera->RecalculateProjectionMatrix();
    }
}

// void JEngine::OnScroll(double xOffset, double yOffset)
// {
//     // m_State.GetCamera()->ProcessMouseScroll(static_cast<float>(yOffset),
//         // m_State.GetCameraSettings()->GetMaxFOV());
// }

IPlatformSurface * JEngine::GetPlatformSurface()
{
    return m_PlatformSurface.get();
}

JResourceSystem* JEngine::GetResourceSystem()
{
    return m_ResourceSystem.get();
}

SceneManager* JEngine::GetSceneManager()
{
    return m_Services->GetService<SceneManager>().get();
}

PostProcessManager* JEngine::GetPostProcessManager()
{
    return m_Services->GetService<PostProcessManager>().get();
}

InputManager* JEngine::GetInputManager()
{
    return m_Services->GetService<InputManager>().get();
}

void JEngine::RegisterServices()
{
    m_Services->RegisterFactory<SceneManager>([]() -> TSharedPtr<SceneManager>
    {
        return TSharedPtr<SceneManager>(new SceneManager());
    });
    m_Services->GetService<SceneManager>();

    m_Services->RegisterFactory<PostProcessManager>([this]() -> TSharedPtr<PostProcessManager>
    {
        return TSharedPtr<PostProcessManager>(new PostProcessManager());
    });
    m_Services->GetService<PostProcessManager>();

    m_Services->RegisterFactory<InputManager>([this]() -> TSharedPtr<InputManager>
    {
        return TSharedPtr<InputManager>(new InputManager());
    });
    m_Services->GetService<InputManager>();
}

bool JEngine::BootstrapScene()
{
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
    m_ResourceSystem->Load<JModelResource>("Dio Brando/DioMansion.obj",      "Dio Brando/DioMansion.obj");
    m_ResourceSystem->Load<JModelResource>("MedievalWindow/MedievalWindow.obj","MedievalWindow/MedievalWindow.obj");

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
    JActor* actor1 = GetSceneManager()->SpawnActor<JActor>();

    actor1->SetName("DioMansion");

    // Attach a model component at runtime to the actor’s root (uses route rendering)
    auto* modelCompDio = actor1->AddRuntimeComponent<JModelComponent>();
    modelCompDio->SetModel("Dio Brando/DioMansion.obj");

    auto actor2 = spawnModelActor("MedievalWindow", "MedievalWindow/MedievalWindow.obj");
    actor1->SetActorLocation(-5.f, 10.f, 0.f);
    actor2->SetActorLocation(0.f, -10.f, 0.f);

    auto* modelComp = actor1->AddRuntimeComponent<JModelComponent>();
    modelComp->SetModel("MedievalWindow/MedievalWindow.obj");

    modelComp->AttachToComponent(modelCompDio);
    modelComp->SetWorldPosition(10.f, 0.f, 2.f);

    JActor* cameraActor = GetSceneManager()->SpawnActor<JActor>();
    cameraActor->SetName("CameraActor");
    cameraActor->AddRuntimeComponent<JCameraComponent>();
    cameraActor->SetActorLocation(-20.f, 0.f, 15.f);

    auto* sceneMgr = JEngine::Get().GetSceneManager();
    JActor* parent = sceneMgr->SpawnActor<JActor>();
    JActor* child  = sceneMgr->SpawnActor<JActor>();

    parent->SetName("ParentActor");
    child->SetName("ChildActor");

    parent->SetActorLocation(10.f, 0.f, 20.f);
    child->AttachToActor(parent);
    child->SetActorRelativeLocation(0.f, 5.f, 0.f); // relative to parent
    std::cout << child->GetActorLocation().ToString() << std::endl;


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