// Copyright 2024 JesseTheCatLover. All Rights Reserved.

#define GLFW_INCLUDE_NONE

#include "Source/JCamera.h"
#include "Source/JModel.h"
#include "Source/JShader.h"
#include "Source/Settings.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <GLFW/glfw3.h>

void ProcessInput(GLFWwindow *Window);

// Callbacks
inline void FramebufferSizeCallback(GLFWwindow *Window, int Width, int Height) {
  glViewport(0, 0, Width, Height);
}
void MouseCallback(GLFWwindow *Window, double xPosIn, double yPosIn);
void ScrollCallback(GLFWwindow *Window, double xOffset, double yOffset);
void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Settings
Settings DefaultSetting;
Settings *Setting = &DefaultSetting;
bool bCanChangeWireframe = true;
enum ViewMode { OBJECT, VIEW, };

// Camera
JCamera DefaultCamera(glm::vec3(0.f, 0.f, 3.f));
JCamera *Camera = &DefaultCamera;
float LastX = Setting->GetScreenWidth() / 2.f;
float LastY = Setting->GetScreenHeight() / 2.f;
ViewMode viewMode = ViewMode::VIEW;

// Timing
float DeltaTime = 0.f;
float LastFrame = 0.f;

int main() {
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

  // Wireframes
  if (Setting->GetbWireFrame())
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // Build and compile our shader program
  JShader ShaderProgram("ModelLoading", "ModelLoading");

  JModel DioBrando("Dio Brando/DioMansion.obj");

  while (!glfwWindowShouldClose(Window)) // RenderLoop
  {
    // DeltaTime
    float CurrentFrame = static_cast<float>(glfwGetTime());
    DeltaTime = CurrentFrame - LastFrame;
    LastFrame = CurrentFrame;

    // Inputs
    ProcessInput(Window);

    // Rendering commands
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Setting->GetbWireFrame() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
                             : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Enable the shader
    ShaderProgram.Use();

    // Transform
    glm::mat4 projection = glm::perspective(
        glm::radians(Camera->Zoom),
        (float)Setting->GetScreenWidth() / (float)Setting->GetScreenHeight(),
        0.1f, 100.0f);
    ShaderProgram.SetMat4("projection", projection);

    glm::mat4 view = Camera->GetViewMatrix();
    ShaderProgram.SetMat4("view", view);

    // World transformation
    glm::mat4 Model{1.f};
    ShaderProgram.SetMat4("model", Model);
    DioBrando.Draw(ShaderProgram);

    // Check and call events (Swap and buffers)
    glfwSwapBuffers(Window);
    glfwPollEvents();
  }

  // De-allocated all resources
  glfwTerminate();

  return 0;
}

void ProcessInput(GLFWwindow *Window) {
  if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(Window, true);
  }

  if (glfwGetKey(Window, GLFW_KEY_J) == GLFW_PRESS) {
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(Window, LastX, LastY);
  }

  if (glfwGetKey(Window, GLFW_KEY_I) == GLFW_PRESS) {
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPos(Window, LastX, LastY);
  }

  if (glfwGetKey(Window, GLFW_KEY_F) == GLFW_PRESS && bCanChangeWireframe) {
    bCanChangeWireframe = false;
    Setting->SetbWireFrame(
        !Setting->GetbWireFrame()); // Toggling the wireframe mode
  }
  if (glfwGetKey(Window, GLFW_KEY_F) == GLFW_RELEASE) {
    bCanChangeWireframe = true;
  }
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

void MouseCallback(GLFWwindow *Window, double xPosIn, double yPosIn) {
  const float xPos = static_cast<float>(xPosIn);
  const float yPos = static_cast<float>(yPosIn);

  float xOffset = xPos - LastX;
  float yOffset =
      LastY - yPos; // reversed since y-coordinates go from bottom to top

  LastX = xPos;
  LastY = yPos;

  Camera->ProcessMouseMovement(xOffset, yOffset);
}

void ScrollCallback(GLFWwindow *Window, double xOffset, double yOffset) {
  Camera->ProcessMouseScroll(static_cast<float>(yOffset),
                             Setting->GetCameraMaxFOV());
}

void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (key == GLFW_KEY_J && action == GLFW_PRESS) {
    if (viewMode == ViewMode::VIEW) {
      glfwSetCursorPos(window, LastX, LastY);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      viewMode = ViewMode::OBJECT;
    } else if (viewMode == ViewMode::OBJECT) {
      glfwSetCursorPos(window, LastX, LastY);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      viewMode = ViewMode::VIEW;
    }
  }

  if (key == GLFW_KEY_F && action == GLFW_PRESS)
    Setting->SetbWireFrame(
        !Setting->GetbWireFrame()); // Toggling the wireframe mode
}