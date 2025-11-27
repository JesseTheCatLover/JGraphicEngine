//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>

#include "EngineState.h"
#include "Memory/SmartPointers.h"
#include "IEditorBridge.h"

class IInputBackend;
class InputManager;
class PostProcessManager;
class SceneManager;
class JInputSystem;
class JResourceSystem;
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
    IPlatformSurface* GetPlatformSurface();
    JResourceSystem* GetResourceSystem();

    SceneManager* GetSceneManager();
    PostProcessManager* GetPostProcessManager();
    InputManager* GetInputManager();

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
    TUniquePtr<JResourceSystem> m_ResourceSystem;
    TUniquePtr<IInputBackend> m_InputBackend;
    TUniquePtr<JInputSystem> m_InputSystem;

    TUniquePtr<TServiceContainer> m_Services;

    bool Initialize();
    bool SurfaceInitialize();
    bool InitializeBackends();
    bool InitializeSubsystems();
    bool InitializeManagers();
    void RunMainLoop();
    void Shutdown();

    void Tick();

    void RegisterServices();

    bool BootstrapScene();
    void CreateDefaultScene();

    void CalculateDeltaTime();
    void UpdateFramebufferSizeContext();

    // GLFW input forwarders | TODO: These all should be moved into a dedicated InputSystem for future
    void ProcessInputs(GLFWwindow* window, float deltaTime);
    void OnMouseMove(double xPosIn, double yPosIn);
    void OnScroll(double xOffset, double yOffset);
    void OnKeyboardAction(GLFWwindow* window, int key, int scancode, int action, int mods);

    // --- Static callbacks for GLFW ---
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#include "JEngine.inl"
