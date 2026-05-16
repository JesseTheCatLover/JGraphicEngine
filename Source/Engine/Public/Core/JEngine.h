//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <vector>

#include "Memory/SmartPointers.h"
#include "IEditorBridge.h"

class IPlatformWindow;
class AssetManager;
class AssetImportSubsystem;
class AssetRegistrySubsystem;
class VirtualPathMounter;
struct FProjectOpenRequest;
class ProjectContext;
class TServiceContainer;
class DebugDraw;
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
    TSharedPtr<T> GetService();

    template<typename T>
    void RegisterFactory(std::function<std::shared_ptr<T>()> factory);

    [[nodiscard]] EngineContext& GetEngineContext();

    IPlatformSurface* GetPlatformSurface();
    IPlatformWindow* GetPrimaryWindow();
    RendererSubsystem* GetRenderer();
    AssetImportSubsystem* GetAssetImportSubsystem();
    AssetRegistrySubsystem* GetAssetRegistrySubsystem();
    ResourceSubsystem* GetResourceSubsystem();
    InputSubsystem* GetInputSubsystem();

    AssetManager* GetAssetManager();
    SceneManager* GetSceneManager();
    PostProcessManager* GetPostProcessManager();
    InputManager* GetInputManager();

    [[nodiscard]] const ProjectContext* GetProjectContext() const { return m_ProjectContext.get(); }
    [[nodiscard]] VirtualPathMounter& GetVirtualPathMounter() const { return *m_VirtualPathMounter.get(); }

    DebugDraw* GetDebugDraw();

private:
    JEngine();
    ~JEngine();

    void SetEditorBridge(IEditorBridge* bridge) { m_EditorBridge = bridge; }
    IRenderBackend* GetRenderBackend();

    bool bRuntimeInitialized = false;

    TUniquePtr<EngineContext> m_Context;
    IEditorBridge* m_EditorBridge = nullptr;
    TUniquePtr<ProjectContext> m_ProjectContext;
    TUniquePtr<VirtualPathMounter> m_VirtualPathMounter;

    TUniquePtr<IPlatformSurface> m_PlatformSurface;
    TSharedPtr<IPlatformWindow> m_PrimaryWindow;
    TUniquePtr<IRenderBackend> m_RenderBackend;
    TUniquePtr<RendererSubsystem> m_Renderer;
    TUniquePtr<AssetImportSubsystem> m_AssetImportSubsystem;
    TUniquePtr<AssetRegistrySubsystem> m_AssetRegistrySubsystem;
    TUniquePtr<ResourceSubsystem> m_ResourceSubSystem;
    TUniquePtr<IInputBackend> m_InputBackend;
    TUniquePtr<InputSubsystem> m_InputSubSystem;

    TUniquePtr<TServiceContainer> m_Services;

    bool InitializeRuntime();
    bool OpenProject(const FProjectOpenRequest& request);
    bool InitializeProject();

    bool Run();

    bool SurfaceInitialize();
    bool InitializeBackends();
    bool InitializeSubsystems();
    bool InitializeManagers();
    bool InitialBuildAssetPipeline();
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
