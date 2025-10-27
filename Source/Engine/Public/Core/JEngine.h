//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "EngineState.h"
#include "Memory/SmartPointers.h"
#include "IEditorBridge.h"
#include "Framework/PostProcessManager.h"
#include "Framework/SceneManager.h"

class JRenderer;
class IRenderBackend;
class IPlatformSurface;
class JRendererLegacy;
class TServiceContainer;
class EditorContext;

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

    EngineState& GetState() { return m_State; }

    template<typename T>
    std::shared_ptr<T> GetService();

    template<typename T>
    void RegisterFactory(std::function<std::shared_ptr<T>()> factory);

    // Syntactic sugar manager accessors
    JRenderer* GetRenderer();
    SceneManager* GetSceneManager();
    PostProcessManager* GetPostProcessManager();

private:
    JEngine();
    ~JEngine();

    bool Run();

    void SetEditorBridge(IEditorBridge* bridge) { m_EditorBridge = bridge; }

    EngineState m_State;
    IEditorBridge* m_EditorBridge = nullptr;
    TUniquePtr<IPlatformSurface> m_PlatformSurface;
    TUniquePtr<IRenderBackend> m_RenderBackend;
    TUniquePtr<JRenderer> m_Renderer;
    TUniquePtr<TServiceContainer> m_Services;

    bool Initialize();
    bool GLFWInitialize(); // TODO: Deprecated
    bool SurfaceInitialize();
    bool RenderBackendInitialize();
    bool InitializeSubsystems();
    bool InitializeManagers();
    void RunMainLoop();
    void Shutdown();

    void Tick();

    void RegisterServices();

    bool BootstrapScene();
    void CreateDefaultScene();

    // GLFW input forwarders
    void ProcessInputs(GLFWwindow* window, float deltaTime);
    void OnFramebufferResize(int width, int height);
    void OnMouseMove(double xPosIn, double yPosIn);
    void OnScroll(double xOffset, double yOffset);
    void OnKeyboardAction(GLFWwindow* window, int key, int scancode, int action, int mods);

    // --- Static callbacks for GLFW ---
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#include "JEngine.inl"
