//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <vector>

#include "Memory/SmartPointers.h"
#include "IEditorBridge.h"

class DebugDraw;
struct FRenderView;
class EngineContext;
class IInputBackend;
class InputManager;
class PostProcessManager;
class SceneManager;
class InputSubsystem;
class ResourceSubsystem;
class RendererSubsystem;
class IRenderBackend;
class IPlatformSurface;
class JRendererLegacy;
class TEditorServiceContainer;

class JEngine
{
    friend class JApplication;
public:
    static JEngine& Get()
    {
        static JEngine instance;
        return instance;
    }

    // Non-copyable, non-movable
    JEngine(const JEngine&) = delete;
    JEngine& operator=(const JEngine&) = delete;

    template<typename T>
    std::shared_ptr<T> GetService();

    template<typename T>
    void RegisterFactory(std::function<std::shared_ptr<T>()> factory);

    [[nodiscard]] EngineContext& GetEngineContext();

    IPlatformSurface* GetPlatformSurface();
    RendererSubsystem* GetRenderer();
    ResourceSubsystem* GetResourceSystem();

    SceneManager* GetSceneManager();
    PostProcessManager* GetPostProcessManager();
    InputManager* GetInputManager();
    DebugDraw* GetDebugDraw();

private:
    JEngine();
    ~JEngine();

    bool Run();

    void SetEditorBridge(IEditorBridge* bridge) { m_EditorBridge = bridge; }

    TUniquePtr<EngineContext> m_Context;
    IEditorBridge* m_EditorBridge = nullptr;

    TUniquePtr<IPlatformSurface> m_PlatformSurface;
    TUniquePtr<IRenderBackend> m_RenderBackend;
    TUniquePtr<RendererSubsystem> m_Renderer;
    TUniquePtr<ResourceSubsystem> m_ResourceSystem;
    TUniquePtr<IInputBackend> m_InputBackend;
    TUniquePtr<InputSubsystem> m_InputSystem;

    TUniquePtr<TEditorServiceContainer> m_Services;

    bool Initialize();
    bool SurfaceInitialize();
    bool InitializeBackends();
    bool InitializeSubsystems();
    bool InitializeManagers();
    void RunMainLoop();
    void Shutdown();

    void Tick();
    void BuildGameViews();
    void TickGameFreeCamera(float deltaTime);

    void RegisterServices();

    bool BootstrapScene();
    void CreateDefaultScene();

    void CalculateDeltaTime();
};

#include "JEngine.inl"
