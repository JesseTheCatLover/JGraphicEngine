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

// Settings
Settings DefaultSetting;
Settings *Setting = &DefaultSetting;
bool bCanChangeWireframe = true;

// Camera
JCamera DefaultCamera(glm::vec3(0.f, 0.f, 3.f));
JCamera *Camera = &DefaultCamera;
float LastX = Setting->GetScreenWidth() / 2.f;
float LastY = Setting->GetScreenHeight() / 2.f;

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
  JShader OutlinerShader("OutlineShader", "BlackColor");

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    Setting->GetbWireFrame() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
                             : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- Common transforms ---
    glm::mat4 projection = glm::perspective(glm::radians(Camera->Zoom),
                                            (float)Setting->GetScreenWidth() / (float)Setting->GetScreenHeight(),
                                            0.1f, 100.0f);
    glm::mat4 view = Camera->GetViewMatrix();
    glm::mat4 model{1.f};

    // --- 1. Draw model normally ---
    glEnable(GL_DEPTH_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // set stencil to 1 where fragments are drawn
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    ShaderProgram.Use();
    ShaderProgram.SetMat4("projection", projection);
    ShaderProgram.SetMat4("view", view);
    ShaderProgram.SetMat4("model", model);
    DioBrando.Draw(ShaderProgram);

    // 1. Draw outline (backfaces only)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // render only backfaces
    glEnable(GL_DEPTH_TEST);

    OutlinerShader.Use();
    OutlinerShader.SetMat4("projection", projection);
    OutlinerShader.SetMat4("view", view);
    OutlinerShader.SetMat4("model", model);
    OutlinerShader.SetFloat("outlineThickness", 0.03f); // adjust thickness
    DioBrando.Draw(OutlinerShader);

    // 2. Draw normal model (frontfaces)
    glCullFace(GL_BACK); // render frontfaces
    ShaderProgram.Use();
    ShaderProgram.SetMat4("projection", projection);
    ShaderProgram.SetMat4("view", view);
    ShaderProgram.SetMat4("model", model);
    DioBrando.Draw(ShaderProgram);

    glDisable(GL_CULL_FACE);

    glm::vec3 position = glm::vec3(-25.f, 0.0f, 0.f);
    model = glm::translate(model, position);

    // --- 1. Draw model normally ---
    glEnable(GL_DEPTH_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // set stencil to 1 where fragments are drawn
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    ShaderProgram.Use();
    ShaderProgram.SetMat4("projection", projection);
    ShaderProgram.SetMat4("view", view);
    ShaderProgram.SetMat4("model", model);
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