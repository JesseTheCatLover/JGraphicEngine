// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#define GLFW_INCLUDE_NONE

#include <Core/Core.h>
#include <Rendering/Rendering.h>
#include <Scene/Scene.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <GLFW/glfw3.h>

void ProcessInput(GLFWwindow *Window);
inline void FramebufferSizeCallback(GLFWwindow *Window, int Width, int Height);
void MouseCallback(GLFWwindow *Window, double xPosIn, double yPosIn);
void ScrollCallback(GLFWwindow *Window, double xOffset, double yOffset);
void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// PostProcessor
JPostProcessor* GPostProcessor = nullptr;

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

  // Select our window to draw and assigning callbacks
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

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Wireframes
  if (Setting->GetbWireFrame())
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // ----------------- Post Processors -----------------
  JPostProcessor PostProcessorNoEffect(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess","PostProcessNoEffect");
  JPostProcessor PostProcessorBlur(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/RadialBlur");
  JPostProcessor PostProcessorGrayscale(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/Grayscale");
  JPostProcessor PostProcessorSharpen(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/SharpenKernel");
  JPostProcessor PostProcessorChroma(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/Chroma");
  JPostProcessor PostProcessorInverter(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/ColorInverter");
  JPostProcessor PostProcessorCRT(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/CRTScanline");
  JPostProcessor PostProcessorEdge(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/EdgeDetection");
  JPostProcessor PostProcessorPixelate(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
    "PostProcess", "PostProcess/PixelatedMosaic");
  JPostProcessor PostProcessorWavy(Setting->GetScreenWidth(), Setting->GetScreenHeight(),
   "PostProcess", "PostProcess/WavyDistortion");

  GPostProcessor = &PostProcessorNoEffect;

  // ----------------- Shaders -----------------
  JShader ShaderProgram("ModelLoading", "ModelLoading");
  JShader ReflectShader("EnvironmentCapture", "EnvironmentReflect");
  JShader RefractionShader("EnvironmentCapture", "EnvironmentRefraction");
  JShader BlackColorShader("OutlineShader", "BlackColor");

  // ----------------- Load Models -----------------
  JModel DioBrando("Dio Brando/DioMansion.obj");
  JModel MedievalWindow("MedievalWindow/MedievalWindow.obj");
  JModel Cube("Cube/Cube.obj");

  // ----------------- Skybox -----------------
  JSkybox SeaSkybox("Sea", "Skybox", "Skybox");


  // ----------------- Scene -----------------
  std::vector<JActor> SceneActors;
  SceneActors.emplace_back(&DioBrando);
  SceneActors.back().Position = glm::vec3(0,0,0);
  SceneActors.back().Scale = glm::vec3(1.0f);
  SceneActors.back().Config.bDrawOutline = true;
  SceneActors.back().Config.bBackCulling = true;

  // Example of a second object
  SceneActors.emplace_back(&DioBrando);
  SceneActors.back().Position = glm::vec3(-25.f, 0.0f, 0.f);
  SceneActors.back().Config.bBackCulling = true;

  // Transparent windows
  SceneActors.emplace_back(&MedievalWindow);
  SceneActors.back().Config.bIsTransparent = true;
  SceneActors.back().Position = glm::vec3(-25.f, 0.0f, 20.f);

  SceneActors.emplace_back(&MedievalWindow);
  SceneActors.back().Config.bIsTransparent = true;
  SceneActors.back().Position = glm::vec3(-15.f, 0.0f, -10.f);

  SceneActors.emplace_back(&Cube);
  SceneActors.back().Position = glm::vec3(15.f, 0.0f, -10.f);
  SceneActors.back().Scale = glm::vec3(2.0f);


  // ----------------- Render Loop -----------------
  while (!glfwWindowShouldClose(Window))
  {
    // DeltaTime
    float CurrentFrame = static_cast<float>(glfwGetTime());
    DeltaTime = CurrentFrame - LastFrame;
    LastFrame = CurrentFrame;

    // Inputs
    ProcessInput(Window);

    // TODO: TEMP Input for demo
    if (glfwGetKey(Window, GLFW_KEY_0) == GLFW_PRESS)
      GPostProcessor = &PostProcessorNoEffect;
    if (glfwGetKey(Window, GLFW_KEY_1) == GLFW_PRESS)
      GPostProcessor = &PostProcessorBlur;
    if (glfwGetKey(Window, GLFW_KEY_2) == GLFW_PRESS)
      GPostProcessor = &PostProcessorChroma;
    if (glfwGetKey(Window, GLFW_KEY_3) == GLFW_PRESS)
      GPostProcessor = &PostProcessorSharpen;
    if (glfwGetKey(Window, GLFW_KEY_4) == GLFW_PRESS)
      GPostProcessor = &PostProcessorGrayscale;
    if (glfwGetKey(Window, GLFW_KEY_5) == GLFW_PRESS)
      GPostProcessor = &PostProcessorCRT;
    if (glfwGetKey(Window, GLFW_KEY_6) == GLFW_PRESS)
      GPostProcessor = &PostProcessorPixelate;
    if (glfwGetKey(Window, GLFW_KEY_7) == GLFW_PRESS)
      GPostProcessor = &PostProcessorWavy;
    if (glfwGetKey(Window, GLFW_KEY_8) == GLFW_PRESS)
      GPostProcessor = &PostProcessorInverter;
    if (glfwGetKey(Window, GLFW_KEY_9) == GLFW_PRESS)
      GPostProcessor = &PostProcessorEdge;

    // --- Begin PostProcessing ---
    GPostProcessor->Begin();

    // Clear buffers
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    Setting->GetbWireFrame() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
                             : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ----------------- Camera & Projection -----------------
    glm::mat4 projection = glm::perspective(
        glm::radians(Camera->Zoom),
        (float)Setting->GetScreenWidth() / (float)Setting->GetScreenHeight(),
        0.1f, 100.0f
    );
    glm::mat4 view = Camera->GetViewMatrix();

    ShaderProgram.Use();
    ShaderProgram.SetMat4("projection", projection);
    ShaderProgram.SetMat4("view", view);

    ReflectShader.Use();
    ReflectShader.SetMat4("projection", projection);
    ReflectShader.SetMat4("view", view);
    ReflectShader.SetVec3("cameraPos", Camera->Position);

    RefractionShader.Use();
    RefractionShader.SetMat4("projection", projection);
    RefractionShader.SetMat4("view", view);
    RefractionShader.SetVec3("cameraPos", Camera->Position);

    BlackColorShader.Use();
    BlackColorShader.SetMat4("projection", projection);
    BlackColorShader.SetMat4("view", view);

    // ----------------- Draw Scene -----------------
    std::vector<std::pair<float, JActor>> sortedTransparent;
    for (auto& act : SceneActors)
    {
      if (act.Config.bIsTransparent)
      {
        float distance = glm::length(Camera->Position - act.Position);
        sortedTransparent.emplace_back(distance, act);
      }
      else
      {
        // Draw opaque immediately
        act.DrawConfig(RefractionShader, BlackColorShader);
      }
    }

    // Sort transparent ones farthest to nearest
    std::sort(sortedTransparent.begin(), sortedTransparent.end(),
    [](const auto& a, const auto& b) { return a.first > b.first;});

    // Draw transparent ones in back-to-front order
    for (auto& [distance, act] : sortedTransparent)
    {
      act.DrawConfig(RefractionShader, BlackColorShader);
    }

    // Reset wireframe so post-process quad is normal
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- Draw skybox ---
    SeaSkybox.Draw(view, projection);

    // --- End PostProcessing ---
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(Window, &fbWidth, &fbHeight);
    GPostProcessor->End(fbWidth, fbHeight);

    // Check and call events (Swap and buffers)
    glfwSwapBuffers(Window);
    glfwPollEvents();
  }

  // De-allocated all resources
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
  GPostProcessor->Resize(Width, Height);
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