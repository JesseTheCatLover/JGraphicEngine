//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/JEngine.h"

#include "Framework/SceneManager.h"
#include "Core/EngineGlobals.h"
#include <iostream>

#include "EngineContext.h"
#include "Core/TServiceContainer.h"
#include "Framework/InputManager.h"
#include "Framework/PostProcessManager.h"
#include "InputSystem/InputBackendFactory.h"
#include "InputSystem/InputSubsystem.h"

#include "Rendering/BackendFactory.h"
#include "Rendering/EGraphicsAPI.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/RendererSubsystem.h"
#include "Resources/ResourceSubsystem.h"
#include "Core/Math/FMath.h"
#include "Scene/SceneComponents/JCameraComponent.h"
#include "Scene/SceneComponents/JModelComponent.h"

#include "InputSystem/MappingStyles/ActionAxis/ActionAxisConfig.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStyle.h"
#include "Rendering/FRenderView.h"

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
    m_Context = TUniquePtr<EngineContext>(new EngineContext());

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
        m_EditorBridge->OnEngineInitialized(m_PlatformSurface.get());
    }

    return true;
}

bool JEngine::SurfaceInitialize()
{
    FSurfaceState surfaceState;
    surfaceState.width = m_Context->GetFramebufferWidth();
    surfaceState.height = m_Context->GetFramebufferHeight();
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

    // Ensure initial framebuffer size is correct
    {
        int fbW = 0, fbH = 0;
        m_PlatformSurface->GetFramebufferSize(fbW, fbH);
        m_Context->SetFramebufferSize(fbW, fbH);
    }

    // --- Register callbacks ---

    // Framebuffer callback
    m_PlatformSurface->SetFramebufferResizeCallback(
        [this](int fbWidth, int fbHeight)
        {
            m_Context->SetFramebufferSize(fbWidth, fbHeight);
        }
    );

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
    m_Renderer = TUniquePtr<RendererSubsystem>(new RendererSubsystem(m_RenderBackend.get(), *m_Context));
    if (!m_Renderer)
    {
        std::cerr << "[JEngine]: Failed to initialize renderer" << std::endl;
        return false;
    }
    m_Renderer->SetPostProcessManager(GetPostProcessManager());

    m_ResourceSystem = TUniquePtr<ResourceSubsystem>(new ResourceSubsystem());
    if (!m_ResourceSystem)
    {
        std::cerr << "[JEngine]: Failed to initialize resource subsystem" << std::endl;
        return false;
    }
    m_ResourceSystem->SetRenderDevice(m_Renderer.get());

    m_InputSystem = TUniquePtr<InputSubsystem>(new InputSubsystem());
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

    // ----------------- Editor camera mappings (Temp) ----------------- TODO: we need a special mapping separation for the editor.

    // 1) Editor_Look: mouse delta X/Y (separate from any gameplay look)
    {
        FActionAxisSlot look{};
        look.name = "Editor_Look";
        look.type = EInputChannelType::Axis2D;

        // X = mouse delta X
        FInputBinding mouseX{};
        mouseX.deviceType    = EInputDeviceType::Mouse;
        mouseX.deviceIndex   = 0;
        mouseX.input         = EPhysicalInput::Mouse_DeltaX;
        mouseX.scale         = 0.0038f;    // sensitivity
        mouseX.deadZone      = 0.0f;
        mouseX.invert        = false;
        mouseX.axisComponent = EAxisComponent::X;

        // Y = mouse delta Y
        FInputBinding mouseY{};
        mouseY.deviceType    = EInputDeviceType::Mouse;
        mouseY.deviceIndex   = 0;
        mouseY.input         = EPhysicalInput::Mouse_DeltaY;
        mouseY.scale         = 0.0038f;
        mouseY.deadZone      = 0.0f;
        mouseY.invert        = false;       // invert so moving mouse up is positive pitch
        mouseY.axisComponent = EAxisComponent::Y;

        look.bindings.push_back(mouseX);
        look.bindings.push_back(mouseY);
        map.actions.push_back(std::move(look));
    }

    // 2) Editor_Move (XY plane): WASD
    {
        FActionAxisSlot move{};
        move.name = "Editor_Move";
        move.type = EInputChannelType::Axis2D;

        // X component = W / S
        FInputBinding forwardX{};
        forwardX.deviceType    = EInputDeviceType::Keyboard;
        forwardX.deviceIndex   = 0;
        forwardX.input         = EPhysicalInput::Key_W;
        forwardX.scale         = +1.0f;
        forwardX.axisComponent = EAxisComponent::X;

        FInputBinding backwardX = forwardX;
        backwardX.input  = EPhysicalInput::Key_S;
        backwardX.scale  = -1.0f;

        // Y component = A / D
        FInputBinding rightY{};
        rightY.deviceType    = EInputDeviceType::Keyboard;
        rightY.deviceIndex   = 0;
        rightY.input         = EPhysicalInput::Key_D;
        rightY.scale         = +1.0f;
        rightY.axisComponent = EAxisComponent::Y;

        FInputBinding leftY = rightY;
        leftY.input = EPhysicalInput::Key_A;
        leftY.scale = -1.0f;

        move.bindings.push_back(forwardX);
        move.bindings.push_back(backwardX);
        move.bindings.push_back(rightY);
        move.bindings.push_back(leftY);

        map.actions.push_back(std::move(move));
    }

    // 3) Editor_MoveUpDown: Space / Shift
    {
        FActionAxisSlot moveY{};
        moveY.name = "Editor_MoveUpDown";
        moveY.type = EInputChannelType::Axis1D;

        FInputBinding up{};
        up.deviceType  = EInputDeviceType::Keyboard;
        up.deviceIndex = 0;
        up.input       = EPhysicalInput::Key_Space;
        up.scale       = +1.0f;

        FInputBinding down = up;
        down.input = EPhysicalInput::Key_LeftShift;
        down.scale = -1.0f;

        moveY.bindings.push_back(up);
        moveY.bindings.push_back(down);

        map.actions.push_back(std::move(moveY));
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
            m_Context->SetRunning(false);
        });

    return true;
}

void JEngine::RunMainLoop()
{
    while (!m_PlatformSurface->ShouldClose() && m_Context->GetIsRunning())
    {
        CalculateDeltaTime();

        // Per-frame views reset
        m_Context->ClearViewSources();

        // Game/editor logic tick
        Tick();

        if (!m_EditorBridge) // Build game views when we are in game mode
        {
            BuildGameViews();
        }

        // Render all assigned views in the context
        m_Renderer->RenderFrame(m_Context->GetViewSources());

        // Editor overlay after 3D views being rendered
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
    auto& deltaTime = m_Context->GetDeltaTime();

    if (GetSceneManager())
        GetSceneManager()->Tick(deltaTime);

    if (m_InputSystem)
        m_InputSystem->Tick(deltaTime);

    if (m_EditorBridge)
        m_EditorBridge->OnTick(deltaTime);
    else
    {
        // Game-mode free camera tick
        TickGameFreeCamera(deltaTime);
    }
}

void JEngine::BuildGameViews() // TODO: this responsibility will be placed in a dedicated framework (SplitScreenManager)
{
    auto* scene = GetSceneManager() ? GetSceneManager()->GetActiveScene() : nullptr;
    if (!scene)
        return;

    auto* cameraComp = scene->GetCameraComponent();
    if (!cameraComp)
        return;

    const int fbW = m_Context->GetFramebufferWidth();
    const int fbH = m_Context->GetFramebufferHeight();

    FRenderView view{};
    view.scene     = scene;
    view.camera    = cameraComp;
    view.viewType  = EViewType::GameView;
    view.viewIndex = 0;

    // GAME: back buffer -> invalid FBO handle
    view.targetFBO = RFramebufferHandle{};

    view.viewportX = 0;
    view.viewportY = 0;
    view.viewportW = fbW;
    view.viewportH = fbH;

    // MSAA / post process profile
    view.sampleCount       = 4;
    view.bClearColor       = true;
    view.bClearDepth       = true;
    view.clearColorValue   = {0.1f, 0.1f, 0.1f, 1.0f};
    view.renderMask        = 0xFFFFFFFFu;
    view.bEnablePostProcess = true;
    view.postProfileId      = 0; // default profile

    m_Context->AddViewSource(view);
}

void JEngine::TickGameFreeCamera(float deltaTime)
{
    // No editor: we assume main camera is the gameplay camera.
    auto* sceneMgr = GetSceneManager();
    if (!sceneMgr) return;

    auto* scene = sceneMgr->GetActiveScene();
    if (!scene) return;

    auto* camera = scene->GetCameraComponent();
    if (!camera) return;

    InputManager* input = GetInputManager();
    if (!input) return;

    // --- Mouse look (same scale as editor camera tool) ---
    FVector2 lookDelta = input->GetAxis2D("Editor_Look");
    if (lookDelta.x != 0.f || lookDelta.y != 0.f)
    {
        static bool sLookInitialized = false;
        static float sYaw   = 0.f;
        static float sPitch = 0.f;

        if (!sLookInitialized)
        {
            FTransform world = camera->GetWorldTransform();
            FEuler rotation  = world.GetRotation().ToEuler();
            sPitch = rotation.Pitch;
            sYaw  = rotation.Yaw;
            sLookInitialized = true;
        }

        sYaw += lookDelta.x;
        sPitch -= lookDelta.y;

        sPitch = FMath::Radians(
            FMath::Clamp(FMath::Degrees(sPitch), -90.f, 90.f)
        );

        FEuler euler(-sPitch, sYaw, 0.f);
        FQuat newRot = euler.ToQuat();
        camera->SetWorldRotation(newRot);
    }

    // --- WASD + Space/Shift movement (poll axes) ---
    FVector2 moveInput = input->GetAxis2D("Editor_Move");      // X = W/S, Y = A/D
    float moveZ = input->GetAxis1D("Editor_MoveUpDown");// Space / Shift

    if (moveInput.x == 0.f && moveInput.y == 0.f && moveZ == 0.f)
        return;

    // Build basis from camera rotation
    FQuat rot = camera->GetWorldTransform().GetRotation();
    FVector3 forward = rot.RotateVector(FVector3::Forward()).Normalized();
    FVector3 right = rot.RotateVector(FVector3::Right())  .Normalized();
    FVector3 up = FVector3::Up();

    FVector3 move(0.f, 0.f, 0.f);
    move += forward * moveInput.x;   // W/S
    move += right   * moveInput.y;   // A/D
    move += up      * moveZ;         // Space/Shift

    if (move.Length() <= 0.f)
        return;

    const float moveSpeed = 15.0f; // or expose/cfg
    move = move.Normalized() * moveSpeed * deltaTime;

    JActor* camActor = camera->GetOwnerActor();
    if (!camActor) return;

    FVector3 pos = camActor->GetActorLocation();
    camActor->SetActorLocation(pos + move);
}

// void JEngine::OnScroll(double xOffset, double yOffset)
// {
//     // m_State.GetCamera()->ProcessMouseScroll(static_cast<float>(yOffset),
//         // m_State.GetCameraSettings()->GetMaxFOV());
// }

EngineContext& JEngine::GetEngineContext()
{
    return *m_Context;
}

IPlatformSurface * JEngine::GetPlatformSurface()
{
    return m_PlatformSurface.get();
}

RendererSubsystem * JEngine::GetRenderer()
{
    return m_Renderer.get();
}

ResourceSubsystem* JEngine::GetResourceSystem()
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

void JEngine::CreateDefaultScene() // TODO: TEMP bootstrap; will be replaced by proper scene loading
{
    constexpr const char* kSceneKey  = "ScannedScene";
    constexpr const char* kSceneName = "ScannedScene";

    if (!GetSceneManager())
    {
        std::cerr << "[JEngine] CreateStartupScene: SceneManager is null.\n";
        return;
    }

    // ---------------------------------------------------------------------
    // 1) (Optional) Preload a few heavy assets to smooth first-frame stutter
    //    Keys should be the same strings you’ll use from components.
    // ---------------------------------------------------------------------
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

    actor1->SetName("TheArmoury");

    // Attach a model component at runtime to the actor’s root (uses route rendering)
    auto* modelArmoury = actor1->AddRuntimeComponent<JModelComponent>();
    modelArmoury->SetModel("TheArmoury/model.obj");

    auto actor2 = spawnModelActor("Tape", "Tape/Tape.obj");
    actor2->SetActorLocation(5.f, 0.f, 5.f);
    actor2->SetActorScale(FVector3(0.9f));

    JActor* cameraActor = GetSceneManager()->SpawnActor<JActor>();
    cameraActor->SetName("CameraActor");
    cameraActor->AddRuntimeComponent<JCameraComponent>();
    cameraActor->SetActorLocation(-20.f, 0.f, 15.f);

    // ---------------------------------------------------------------------
    // 5) Save the scene so next launch restores this layout
    // ---------------------------------------------------------------------
    GetSceneManager()->SaveSceneFile(scene, kSceneKey);
}

void JEngine::CalculateDeltaTime()
{
    auto currentFrame = m_PlatformSurface->GetTimeSeconds();
    m_Context->SetDeltaTime(currentFrame - m_Context->GetLastFrameTime());
    m_Context->SetLastFrameTime(currentFrame);
}