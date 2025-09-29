//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "EngineState.h"
#include "Core/TServiceContainer.h"

class SceneManager;
class PostProcessManager;
class EditorContext;

class JEngine
{
public:
    static JEngine& Get()
    {
        static JEngine instance;
        return instance;
    }

    // Non-copyable, non-movable
    JEngine(const JEngine&) = delete;
    JEngine& operator=(const JEngine&) = delete;

    bool Run();

    EngineState& GetState() { return m_State; }

    // Convenience Core Manager accessors
    SceneManager* GetSceneManager();
    PostProcessManager* GetPostProcessManager();

    // Service access (modular)
    template<typename T>
    T* GetService() { return m_Services.GetService<T>(); }

    template<typename T, typename... Args>
    T* GetOrCreateService(Args&&... args) { return m_Services.GetOrCreateService<T>(std::forward<Args>(args)...); }

    // GLFW input forwarders
    void ProcessInputs(GLFWwindow* window, float deltaTime);
    void OnFramebufferResize(int width, int height);
    void OnMouseMove(double xPosIn, double yPosIn);
    void OnScroll(double xOffset, double yOffset);
    void OnKeyboardAction(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    JEngine() {}
    ~JEngine() = default;

    EngineState m_State;
    TServiceContainer m_Services;

    bool Initialize();
    void Tick();
    void Shutdown();

    void RegisterServices();
    bool GLFWInitialize();

    // --- Static callbacks for GLFW ---
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
