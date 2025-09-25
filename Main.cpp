// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#define GLFW_INCLUDE_NONE

#include <Rendering/Rendering.h>
#include <Scene/Scene.h>
#include <EditorApp.h>
#include <Core/JEngine.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "glm/gtc/type_ptr.hpp"
#include "Rendering/JRenderer.h"
#include "Framework/PostProcessManager.h"


void ProcessInput(GLFWwindow *Window);
inline void FramebufferSizeCallback(GLFWwindow *Window, int Width, int Height);
void MouseCallback(GLFWwindow *Window, double xPosIn, double yPosIn);
void ScrollCallback(GLFWwindow *Window, double xOffset, double yOffset);
void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


// Global renderer & post-process manager
JRenderer* GRenderer = nullptr;
PostProcessManager* GPostProcessManager = nullptr;

// Settings
Settings DefaultSetting;
Settings *Setting = &DefaultSetting;
bool bCanChangeWireframe = true;
enum ViewMode { Scene, UI, };

// Camera
JCamera DefaultCamera(glm::vec3(0.f, 0.f, 3.f));
JCamera *Camera = &DefaultCamera;
float LastX = Setting->GetScreenWidth() / 2.f;
float LastY = Setting->GetScreenHeight() / 2.f;
ViewMode viewMode = ViewMode::Scene;

// Timing
float DeltaTime = 0.f;
float LastFrame = 0.f;

int main() {
  // ----------------- GLFW Init -----------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  GLFWwindow *Window =
      glfwCreateWindow(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
                       "Jesse's Magical Workshop", NULL, NULL);
  if (!Window) {
    std::cout << "Failed to create GLFW window." << std::endl;
    glfwTerminate();
    return -1;
  }

  // Assigning callbacks
  glfwMakeContextCurrent(Window);
  glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
  glfwSetCursorPosCallback(Window, MouseCallback);
  glfwSetScrollCallback(Window, ScrollCallback);
  glfwSetKeyCallback(Window, KeyboardCallback);

  // Capture and hide the cursor
  glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(Window, LastX, LastY);

  if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Get EngineState
  EngineState& State = JEngine::Get().GetState();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // ----------------- Rendering & PostProcessor -----------------
  // Initialize scene renderer
  GRenderer = new JRenderer(Setting->GetScreenWidth(), Setting->GetScreenHeight(), 4);
  // Initialize post-processing manager
  GPostProcessManager = new PostProcessManager(Setting->GetScreenWidth(), Setting->GetScreenHeight());

  // Add effects
  GPostProcessManager->AddProcessor(std::make_unique<JPostProcessor>("PostProcess", "PostProcessNoEffect"));
  GPostProcessManager->AddProcessor(std::make_unique<JPostProcessor>("PostProcess", "PostProcess/Chroma"));
  GPostProcessManager->AddProcessor(std::make_unique<JPostProcessor>("PostProcess", "PostProcess/CRTScanline"));

  // ----------------- Shaders -----------------
  JShader ShaderProgram("ModelLoading", "ModelLoading");
  JShader BlackColorShader("OutlineShader", "BlackColor");

  // --- UBO ---
  GLuint uboCamera;
  glGenBuffers(1, &uboCamera);
  glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
  glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboCamera); // binding point 0

  // link shaders to this block
  ShaderProgram.LinkUniformBlock("CameraData", 0);
  BlackColorShader.LinkUniformBlock("CameraData", 0);

  // ----------------- Load Models -----------------
  JModel DioBrando("Dio Brando/DioMansion.obj");
  JModel MedievalWindow("MedievalWindow/MedievalWindow.obj");

  // ----------------- Skybox -----------------
  JSkybox SeaSkybox("Sea", "Skybox", "Skybox");

  // ----------------- Scene -----------------
  State.GetSceneActors().emplace_back(&DioBrando, "DioBrando Outline");
  State.GetSceneActors().back().Position = glm::vec3(0,0,0);
  State.GetSceneActors().back().Scale = glm::vec3(1.0f);
  State.GetSceneActors().back().Config.bDrawOutline = true;
  State.GetSceneActors().back().Config.bBackCulling = false;

  // Example of a second object
  State.GetSceneActors().emplace_back(&DioBrando, "DioBrando");
  State.GetSceneActors().back().Position = glm::vec3(-25.f, 0.0f, 0.f);
  State.GetSceneActors().back().Config.bBackCulling = false;

  // Transparent windows
  State.GetSceneActors().emplace_back(&MedievalWindow, "Window 1");
  State.GetSceneActors().back().Config.bIsTransparent = true;
  State.GetSceneActors().back().Position = glm::vec3(-25.f, 0.0f, 20.f);

  State.GetSceneActors().emplace_back(&MedievalWindow, "Window 2");
  State.GetSceneActors().back().Config.bIsTransparent = true;
  State.GetSceneActors().back().Position = glm::vec3(-15.f, 0.0f, -10.f);

  // ----------------- GUI Init -----------------
  EditorApp Editor(Window);

  // ----------------- Render Loop -----------------
  while (!glfwWindowShouldClose(Window))
  {
    // DeltaTime
    float currentFrame = static_cast<float>(glfwGetTime());
    State.SetDeltaTime(currentFrame - LastFrame);
    LastFrame = currentFrame;

    // Inputs
    DeltaTime = State.GetDeltaTime(); // TODO: Temporarily here
    ProcessInput(Window);

    Setting->GetbWireFrame() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
                             : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- Begin Editor Frame ---
    Editor.BeginFrame();

    // ----------------- Camera & Projection -----------------
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(Window, &fbWidth, &fbHeight);
    glm::mat4 projection = glm::perspective(
        glm::radians(Camera->Zoom),
        (float)fbWidth / fbHeight,
        0.1f, 100.0f
    );
    glm::mat4 view = Camera->GetViewMatrix();

    glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));

    // --- Draw Editor ---
    if (viewMode == ViewMode::UI)
    {
      ImGui::Begin("Debug Window");
      static float fps = 0.0f;
      static float smoothing = 0.99f; // closer to 1 = smoother
      float currentFPS = 1.0f / DeltaTime;
      fps = fps * smoothing + currentFPS * (1.0f - smoothing);
      ImGui::Text("FPS: %.0f ms", fps);
      ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)",
                  Camera->Position.x, Camera->Position.y, Camera->Position.z);

      ImGui::End();

      Editor.RenderPanels();
    }

    // ----------------- Draw Scene -----------------
    // Begin scene rendering
    GRenderer->BeginScene();

    std::vector<std::pair<float, JActor>> sortedTransparent;
    for (auto& act : State.GetSceneActors())
    {
      if (act.Config.bIsTransparent)
      {
        float distance = glm::length(Camera->Position - act.Position);
        sortedTransparent.emplace_back(distance, act);
      }
      else
      {
        // Draw opaque immediately
        act.DrawConfig(ShaderProgram, BlackColorShader);
      }
    }

    // Sort transparent ones farthest to nearest
    std::sort(sortedTransparent.begin(), sortedTransparent.end(),
    [](const auto& a, const auto& b) { return a.first > b.first;});

    // Draw transparent ones in back-to-front order
    for (auto& [distance, act] : sortedTransparent)
    {
      act.DrawConfig(ShaderProgram, BlackColorShader);
    }

    // Reset wireframe so post-process quad is normal
    if (Setting -> GetbWireFrame()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- Draw skybox ---
    SeaSkybox.Draw(view, projection);

    // Finish scene rendering
    GRenderer->EndScene();

    // Apply post-processing
    GPostProcessManager->ApplyChain(
      GRenderer->GetSceneTargetTexture(), fbWidth, fbHeight);

    // --- Render Editor ---
    Editor.EndFrame();

    // Check and call events (Swap and buffers)
    glfwSwapBuffers(Window);
    glfwPollEvents();
  }

  // De-allocated all resources
  Editor.Shutdown();
  glfwDestroyWindow(Window);
  glfwTerminate();

  return 0;
}

// --------------------- Input & Callbacks ---------------------
void ProcessInput(GLFWwindow *Window)
{
  if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
    Camera->ProcessKeyboard(ECM_Forward, DeltaTime);
  if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
    Camera->ProcessKeyboard(ECM_Backward, DeltaTime);
  if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
    Camera->ProcessKeyboard(ECM_Left, DeltaTime);
  if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
    Camera->ProcessKeyboard(ECM_Right, DeltaTime);
  if (glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS)
    Camera->ProcessKeyboard(ECM_Up, DeltaTime);
  if (glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    Camera->ProcessKeyboard((ECM_Down), DeltaTime);
}

// Callbacks
inline void FramebufferSizeCallback(GLFWwindow *Window, int Width, int Height)
{
  glViewport(0, 0, Width, Height);
  if (GRenderer)
    GRenderer->Resize(Width, Height);
}

void MouseCallback(GLFWwindow *Window, double xPosIn, double yPosIn)
{
  if (viewMode != ViewMode::Scene) return;

  const float xPos = static_cast<float>(xPosIn);
  const float yPos = static_cast<float>(yPosIn);

  float xOffset = xPos - LastX;
  float yOffset =
      LastY - yPos; // reversed since y-coordinates go from bottom to top

  LastX = xPos;
  LastY = yPos;

  Camera->ProcessMouseMovement(xOffset, yOffset);
}

void ScrollCallback(GLFWwindow *Window, double xOffset, double yOffset)
{
  Camera->ProcessMouseScroll(static_cast<float>(yOffset),
                             Setting->GetCameraMaxFOV());
}

void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (key == GLFW_KEY_J && action == GLFW_PRESS)
    {
    if (viewMode == ViewMode::Scene)
      {
      glfwSetCursorPos(window, LastX, LastY);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      viewMode = ViewMode::UI;
    } else if (viewMode == ViewMode::UI) {
      glfwSetCursorPos(window, LastX, LastY);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      viewMode = ViewMode::Scene;
    }
  }

  if (key == GLFW_KEY_F && action == GLFW_PRESS)
    Setting->SetbWireFrame(
        !Setting->GetbWireFrame()); // Toggling the wireframe mode
}