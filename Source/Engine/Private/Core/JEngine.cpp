//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/JEngine.h"

#include "Framework/SceneManager.h"
#include "Core/EngineGlobals.h"
#include <iostream>

#include "EngineContext.h"
#include "Assets/AssetImportSubsystem.h"
#include "Assets/AssetRegistrySubsystem.h"
#if JENGINE_WITH_SOURCE_ASSETS
#include "Assets/EngineSourceAssetBootstrapper.h"
#endif
#include "Assets/Importers/IAssetImporter.h"
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
#include "Core/Project/ProjectContext.h"
#include "Core/Serialization/SerializationSubsystem.h"
#include "Framework/DebugDrawFramework.h"

#include "InputSystem/MappingStyles/ActionAxis/ActionAxisConfig.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStyle.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Framework/AssetManager.h"
#include "Rendering/FRenderView.h"
#include "Rendering/IPlatformWindow.h"
#include "Scene/SceneComponents/JCameraComponent.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UProcess.h"

JEngine::JEngine()
    : m_Services(MakeUnique<TServiceContainer>()),
    m_ProjectContext(MakeUnique<ProjectContext>()),
    m_VirtualPathMounter(MakeUnique<VirtualPathMounter>())
{
}

JEngine::~JEngine()
{
}

bool JEngine::InitializeRuntime()
{
    if (bRuntimeInitialized)
    {
        std::cerr << "[JEngine]: InitializeRuntime() called more than once, "
                     "Ignoring subsequent call\n";
        return true;
    }

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

    bRuntimeInitialized = true;

    return true;
}

bool JEngine::OpenProject(const FProjectOpenRequest& request)
{
    if (!bRuntimeInitialized)
    {
        std::cerr << "[JEngine]: Runtime is not initialized. Cannot open project.\n";
        return false;
    }

    // Reset mount state first so we never keep stale mappings.
    m_ProjectContext->Reset();
    m_VirtualPathMounter->Clear();

    if (!m_ProjectContext->OpenProject(request))
    {
        std::cerr << "[JEngine]: Failed to open project context.\n";
        return false;
    }

    // Mount only the asset/content roots into the virtual namespace.
    if (!m_VirtualPathMounter->Mount("/Engine", m_ProjectContext->GetEngineAssetsRoot()))
    {
        std::cerr << "[JEngine]: Failed to mount /Engine to engine assets root: "
                  << m_ProjectContext->GetEngineAssetsRoot() << "\n";

        m_ProjectContext->Reset();
        m_VirtualPathMounter->Clear();
        return false;
    }

    if (!m_VirtualPathMounter->Mount("/Project", m_ProjectContext->GetProjectAssetsRoot()))
    {
        std::cerr << "[JEngine]: Failed to mount /Project to project assets root: "
                  << m_ProjectContext->GetProjectAssetsRoot() << "\n";

        m_ProjectContext->Reset();
        m_VirtualPathMounter->Clear();
        return false;
    }

    // Important:
    // Update surface's current context window to the primary first, before setting up the editor bridge or project
    m_PlatformSurface->MakeContextCurrent(m_PrimaryWindow);

    if (!InitializeProject())
    {
        std::cerr << "[JEngine]: Initialization of the project has failed" << std::endl;
        return false;
    }

    m_PrimaryWindow->SetTitle(m_PrimaryWindow->GetTitle() + " - " + m_ProjectContext->GetProjectName());
    m_PrimaryWindow->Show();

    return true;
}

bool JEngine::InitializeProject()
{
    if (!InitializeManagers())
    {
        std::cerr << "[JEngine]: Failed to initialize managers" << std::endl;
        return false;
    }

    if (!InitialBuildAssetPipeline())
    {
        std::cerr << "[JEngine]: Failed to initial build pipeline" << std::endl;
        return false;
    }

    // Bootstrap default scene
    if (!BootstrapScene())
    {
        std::cerr << "[JEngine]: Bootstrapping the default scene has failed" << std::endl;
        return false;
    }

    if (m_EditorBridge)
    {
        m_EditorBridge->OnProjectInitialized(m_PrimaryWindow.get(), *m_ProjectContext.get());
    }

    return true;
}

void JEngine::RestartEditor(const std::string& targetProjectPath, const std::string& extraArgs)
{
    std::filesystem::path exePath = UFileSystem::GetExecutablePath();

    if (std::filesystem::is_directory(exePath))
    {
        // Change "RedleafEngine" to match the exact executable file output name if it's named differently
#ifdef JENGINE_PLATFORM_WINDOWS
        exePath /= "RedleafEngine.exe";
#else
        exePath /= "RedleafEngine";
#endif
    }

    std::vector<std::string> arguments;

    arguments.emplace_back("--editor");

    if (targetProjectPath.empty())
    {
        arguments.emplace_back("--force-launcher");
    }
    else
    {
        arguments.emplace_back("--project=" + targetProjectPath);
    }

    if (!extraArgs.empty())
    {
        arguments.emplace_back(extraArgs);
    }

    UProcess::FLaunchOptions launchOptions;
    launchOptions.bDetached = true;
    launchOptions.bHidden = true;

    if (!UProcess::Launch(exePath, arguments, launchOptions))
    {
        std::cerr << "[JEngine]: Failed to restart editor.\n";
        return;
    }

    // Safely break the current engine's main loop
    if (m_Context)
    {
        m_Context->SetRunning(false);
    }
}

bool JEngine::Run()
{
    if (!bRuntimeInitialized)
    {
        std::cerr << "[JEngine]: Runtime is not initialized. Cannot run." << std::endl;
        return false;
    }

    if (!m_ProjectContext->IsOpen())
    {
        std::cerr << "[JEngine]: Cannot run without an open project.\n";
        return false;
    }

    RunMainLoop();
    Shutdown();

    return true;
}

bool JEngine::SurfaceInitialize()
{
    FWindowDesc surfaceState;
    surfaceState.width = m_Context->GetFramebufferWidth();
    surfaceState.height = m_Context->GetFramebufferHeight();
    surfaceState.windowState = EWindowState::Maximized;
    surfaceState.title = "RedleafEngine Alpha1 - JEditor (Prototype)";

    ESurfaceAPI surfaceAPI = ESurfaceAPI::GLFW;
    m_PlatformSurface = BackendFactory::MakeSurfaceBackend(surfaceAPI);
    if (!m_PlatformSurface)
    {
        std::cerr << "[JEngine]: No surface backend available for requested API" << std::endl;
        return false;
    }
    if (!m_PlatformSurface->Initialize())
    {
        std::cerr << "[JEngine]: Failed to initialize the platform surface" << std::endl;
        return false;
    }

    m_PrimaryWindow = m_PlatformSurface->CreateWindow(surfaceState, true);
    m_PrimaryWindow->Hide();

    // Ensure initial framebuffer size is correct
    {
        int fbW = 0, fbH = 0;
        m_PrimaryWindow->GetFramebufferSize(fbW, fbH);
        m_Context->SetFramebufferSize(fbW, fbH);
    }

    // --- Register callbacks ---

    // Framebuffer callback
    m_PrimaryWindow->SetFramebufferResizeCallback(
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

    auto inputWindow = m_PlatformSurface->GetEffectiveInputWindow();
    if (!inputWindow)
    {
        std::cerr << "[JEngine]: No effective input window available.\n";
        return false;
    }

    m_InputBackend = InputBackendFactory::MakeInputBackend(inputWindow.get());
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

    m_AssetRegistrySubsystem = MakeUnique<AssetRegistrySubsystem>();
    if (!m_AssetRegistrySubsystem)
    {
        std::cerr << "[JEngine]: Failed to initialize asset registry subsystem" << std::endl;
        return false;
    }

    m_AssetImportSubsystem = MakeUnique<AssetImportSubsystem>();
    if (!m_AssetImportSubsystem)
    {
        std::cerr << "[JEngine]: Failed to initialize asset import subsystem" << std::endl;
        return false;
    }
    m_AssetImportSubsystem->RegisterEssentialImporters();

    m_ResourceSubSystem = TUniquePtr<ResourceSubsystem>(new ResourceSubsystem(m_AssetRegistrySubsystem.get()));
    if (!m_ResourceSubSystem)
    {
        std::cerr << "[JEngine]: Failed to initialize resource subsystem" << std::endl;
        return false;
    }
    m_ResourceSubSystem->SetRenderDevice(m_Renderer.get());

    SerializationSubsystem::Get().Initialize();

    m_InputSubSystem = TUniquePtr<InputSubsystem>(new InputSubsystem());
    if (!m_InputSubSystem)
    {
        std::cerr << "[JEngine]: Failed to initialize input subsystem" << std::endl;
        return false;
    }
    m_InputSubSystem->Initialize(m_InputBackend.get());

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

    {
        FActionAxisSlot look{};
        look.name = "Camera.Look";
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

    {
        FActionAxisSlot move{};
        move.name = "Camera.Move";
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

    {
        FActionAxisSlot moveY{};
        moveY.name = "Camera.Up";
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
    m_InputSubSystem->SetMappingStyle(MakeUnique<ActionAxisStyle>(map));
    // ----------------------------------------------------

    return true;
}

bool JEngine::InitializeManagers()
{
    if (!m_Services->GetService<AssetManager>()->Initialize(GetAssetRegistrySubsystem(),
      GetAssetImportSubsystem(), m_VirtualPathMounter.get()))
    {
        std::cerr << "[JEngine]: Failed to initialize AssetManager" << std::endl;
        return false;
    }

    if (!m_Services->GetService<InputManager>()->Initialize(m_InputSubSystem.get()))
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

bool JEngine::InitialBuildAssetPipeline()
{
#if JENGINE_WITH_SOURCE_ASSETS
    { // Bootstrap and compile source assets if engine is being built from source
        if (!EngineSourceAssetBootstrapper::Bootstrap(
            *GetAssetImportSubsystem(), *GetProjectContext(), GetVirtualPathMounter()))
        {
            std::cerr << "[JEngine]: Failed to boot source assets\n";
            return false;
        }
    }
#endif

    // Initial AssetRegistry population:

    if (m_ProjectContext->IsOpen())
    {
        if (!GetAssetManager())
            return false;

        const FAssetOpResult sync = GetAssetManager()->InitialSyncRegistryFromDisk();
        if (!sync.bSuccess)
        {
            std::cerr << "[JEngine]: InitialSyncRegistryFromDisk failed ("
                  << sync.errors.size() << " errors, "
                  << sync.warnings.size() << " warnings)\n";

            for (const auto& e : sync.errors)
                std::cerr << "  - " << e << "\n";
            return false;
        }

        for (const auto& w : sync.warnings)
            std::cerr << "[JEngine]: InitialSyncRegistryFromDisk warning: " << w << "\n";
    }
    return true;
}

void JEngine::RunMainLoop()
{
    while (!m_PrimaryWindow->ShouldClose() && m_Context->GetIsRunning())
    {
        CalculateDeltaTime();

        if (GetDebugDraw())
            GetDebugDraw()->BeginFrame();

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
            m_EditorBridge->OnRenderOverlay(m_Context->GetDeltaTime());

        // Swap front/back buffers (show rendered frame)
        m_PlatformSurface->Present(m_PrimaryWindow);

        // Poll window + input events
        m_PlatformSurface->PollSurfaceEvents();
    }
}

void JEngine::Shutdown()
{
    GEngine = nullptr;
    m_Renderer->Shutdown();
    m_AssetImportSubsystem->Shutdown();
    m_AssetRegistrySubsystem->Shutdown();
    m_ResourceSubSystem->Shutdown();
    m_InputSubSystem->Shutdown();
    m_PlatformSurface->Shutdown();
    bRuntimeInitialized = false;
}

void JEngine::Tick()
{
    auto& deltaTime = m_Context->GetDeltaTime();

    if (GetSceneManager())
        GetSceneManager()->Tick(deltaTime);

    if (GetDebugDraw())
        GetDebugDraw()->Tick(deltaTime);

    if (m_InputSubSystem)
        m_InputSubSystem->Tick(deltaTime);

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

    m_Context->SubmitViewSource(view);
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
    FVector2 lookDelta = input->GetAxis2D("Camera.Look");
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
    FVector2 moveInput = input->GetAxis2D("Camera.Move");      // X = W/S, Y = A/D
    float moveZ = input->GetAxis1D("Camera.Up");// Space / Shift

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

EngineContext& JEngine::GetEngineContext()
{
    return *m_Context;
}

IPlatformSurface * JEngine::GetPlatformSurface()
{
    return m_PlatformSurface.get();
}

IPlatformWindow* JEngine::GetPrimaryWindow()
{
    return m_PrimaryWindow.get();
}

RendererSubsystem * JEngine::GetRenderer()
{
    return m_Renderer.get();
}

ResourceSubsystem* JEngine::GetResourceSubsystem()
{
    return m_ResourceSubSystem.get();
}

AssetImportSubsystem * JEngine::GetAssetImportSubsystem()
{
    return m_AssetImportSubsystem.get();
}

AssetRegistrySubsystem * JEngine::GetAssetRegistrySubsystem()
{
    return m_AssetRegistrySubsystem.get();
}

InputSubsystem * JEngine::GetInputSubsystem()
{
    return m_InputSubSystem.get();
}

AssetManager * JEngine::GetAssetManager()
{
    return m_Services->GetService<AssetManager>().get();
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

DebugDraw* JEngine::GetDebugDraw()
{
    return m_Services->GetService<DebugDraw>().get();
}

void JEngine::RegisterServices()
{
    m_Services->RegisterFactory<AssetManager>([]() -> TSharedPtr<AssetManager>
    {
        return TSharedPtr<AssetManager>(new AssetManager());
    });
    m_Services->GetService<AssetManager>();

    m_Services->RegisterFactory<SceneManager>([]() -> TSharedPtr<SceneManager>
    {
        return TSharedPtr<SceneManager>(new SceneManager());
    });
    m_Services->GetService<SceneManager>();

    m_Services->RegisterFactory<PostProcessManager>([]() -> TSharedPtr<PostProcessManager>
    {
        return TSharedPtr<PostProcessManager>(new PostProcessManager());
    });
    m_Services->GetService<PostProcessManager>();

    m_Services->RegisterFactory<InputManager>([]() -> TSharedPtr<InputManager>
    {
        return TSharedPtr<InputManager>(new InputManager());
    });
    m_Services->GetService<InputManager>();

    m_Services->RegisterFactory<DebugDraw>([]() -> TSharedPtr<DebugDraw>
    {
        return TSharedPtr<DebugDraw>(new DebugDraw());
    });
}

bool JEngine::BootstrapScene() // TODO: Implement a proper scene bootstrapper logic
{
    constexpr const char* kSceneVirtualPath = "/Project/Scenes/ScannedSceneVer2.jscene";

    auto* startupScene = GetSceneManager()->LoadSceneFile(kSceneVirtualPath);
    if (!startupScene)
    {
        CreateDefaultScene();
    }
    return true;
}

void JEngine::CreateDefaultScene() // TODO: Very DEMO DEVELOP Stage Function. Will be replaced by a proper default scene creator
{
}

void JEngine::CalculateDeltaTime()
{
    auto currentFrame = m_PlatformSurface->GetTimeSeconds();
    m_Context->SetDeltaTime(currentFrame - m_Context->GetLastFrameTime());
    m_Context->SetLastFrameTime(currentFrame);
}

IRenderBackend* JEngine::GetRenderBackend()
{
    return m_RenderBackend.get();
}
