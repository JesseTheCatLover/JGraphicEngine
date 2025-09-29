//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/JEngine.h"

#include "Framework/SceneManager.h"
#include "Core/EngineGlobals.h"
#include "Core/Contexts/FViewportContext.h"
#include "Rendering/JRenderer.h"
#include "Framework/PostProcessManager.h"
#include "Scene/JCamera.h"
#include <iostream>

#include "glad/gl.h"

bool JEngine::Initialize()
{
    if (!GLFWInitialize()) return false;

    GEngine = this;

    RegisterServices();

    // TODO: Make all scene object JActor driven in the future.
    // Try loading default
    auto* scene = GetSceneManager()->LoadSceneFile("DefaultScene");
    if (!scene)
    {
        // Create new scene object (not actor-driven yet)
        auto newScene = std::make_unique<JScene>("DefaultScene");

        // TEMPORARY hardcoded setup (direct system-level stuff)
        {
            // Add skybox
            auto skybox = std::make_unique<JSkybox>("Sea", "Skybox", "Skybox");
            newScene->AttachSkybox(std::move(skybox));

            // Add a model
            auto dio = std::make_unique<JModel>("Dio Brando/DioMansion.obj");
            newScene->AddModel(std::move(dio));

            auto window = std::make_unique<JModel>("MedievalWindow/MedievalWindow.obj");
            newScene->AddModel(std::move(window));

            // etc.
        }

        // Save it
        GetSceneManager()->SaveSceneFile(newScene.get(), "DefaultScene");

        // Activate it
        scene = GetSceneManager()->LoadSceneFile("DefaultScene");
    }

    return true;
}

void JEngine::Tick()
{
    auto* sceneMgr = GetSceneManager();
    if (sceneMgr)
        sceneMgr->Update(m_State.GetDeltaTime());
}

bool JEngine::Run()
{
    if (!Initialize()) return false;
    while (m_State.GetIsRunning())
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        m_State.SetDeltaTime(currentFrame - m_State.GetLastFrameTime());
        m_State.SetLastFrameTime(currentFrame);

        Tick();
    }
    Shutdown();
    return true;
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
    if (auto Renderer = GetService<JRenderer>())
        Renderer->Resize(width, height);
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

SceneManager* JEngine::GetSceneManager()
{
    return m_Services.GetService<SceneManager>();
}

PostProcessManager * JEngine::GetPostProcessManager()
{
    return m_Services.GetService<PostProcessManager>();
}

void JEngine::Shutdown()
{
    GEngine = nullptr;
}

void JEngine::RegisterServices()
{
    m_Services.RegisterService<JRenderer>(m_State.GetWindowWidth(), m_State.GetWindowHeight(), 4);
    m_Services.RegisterService<PostProcessManager>(m_State.GetWindowWidth(), m_State.GetWindowHeight());
    m_Services.RegisterService<SceneManager>();
}

bool JEngine::GLFWInitialize()
{
    // ----------------- GLFW Init -----------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* Window = glfwCreateWindow(m_State.GetWindowWidth(), m_State.GetWindowHeight(),
                         "Jesse's Magical Workshop", NULL, NULL);
    if (!Window) {
        std::cout << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return false;
    }

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