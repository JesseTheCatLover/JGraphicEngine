// Copyright 2024 JesseTheCatLover. All Rights Reserved.

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Source/JShader.h"
#include "Source/JTexture.h"
#include "Source/Settings.h"
#include "Source/ThirdParty/stb_image.h"
#include "Source/JCamera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void ProcessInput(GLFWwindow* Window);

// Callbacks
inline void FramebufferSizeCallback(GLFWwindow* Window, int Width, int Height) { glViewport(0, 0, Width, Height); }
void MouseCallback(GLFWwindow* Window, double xPosIn, double yPosIn);
void ScrollCallback(GLFWwindow* Window, double xOffset, double yOffset);

// Settings
Settings DefaultSetting;
Settings *Setting = &DefaultSetting;
bool bCanChangeWireframe = true;

// Camera
JCamera DefaultCamera(glm::vec3(0.f, 0.f, 3.f));
JCamera* Camera = &DefaultCamera;
float LastX = Setting -> GetScreenWidth() / 2.f;
float LastY = Setting -> GetScreenHeight() / 2.f;

// Light
glm::vec3 LightPos{1.2f, 1.f, 2.f};

// Timing
float DeltaTime = 0.f;
float LastFrame = 0.f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow *Window = glfwCreateWindow(Setting->GetScreenWidth(), Setting->GetScreenHeight(), "Jesse's Magical Workshop",
            NULL, NULL);
    if(!Window)
    {
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

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Wireframes
    if(Setting -> GetbWireFrame()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Build and compile our shader program
    JShader ShaderProgram("DefaultVertex.vert", "Color.frag");
    JShader LightCubeShader("LightVertex.vert", "LightCube.frag");
    stbi_set_flip_vertically_on_load(true);
    JTexture DiffuseMap("Container2.png");
    JTexture SpecularMap("Container2_Specular.png");
    JTexture EmissionMap("dio.png");

    ShaderProgram.Use();
    ShaderProgram.SetInt("Material.Diffuse", 0);
    ShaderProgram.SetInt("Material.Specular", 1);
    ShaderProgram.SetInt("Material.Emission", 2);

    float vertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
    // positions all containers
    glm::vec3 cubePositions[] = {
            glm::vec3( 0.0f,  0.0f,  0.0f),
            glm::vec3( 2.0f,  5.0f, -15.0f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -12.3f),
            glm::vec3( 2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f,  3.0f, -7.5f),
            glm::vec3( 1.3f, -2.0f, -2.5f),
            glm::vec3( 1.5f,  2.0f, -2.5f),
            glm::vec3( 1.5f,  0.2f, -1.5f),
            glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    glm::vec3 pointLightPositions[] = {
            glm::vec3( 0.7f,  0.2f,  2.0f),
            glm::vec3( 2.3f, -3.3f, -4.0f),
            glm::vec3(-4.0f,  2.0f, -12.0f),
            glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    unsigned int VBO, VAO, LightVAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(VAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    glGenVertexArrays(1, &LightVAO);
    glBindVertexArray(LightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

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
		Setting -> GetbWireFrame() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        /*
        LightPos.x = 2.0f + sin(glfwGetTime()) * 2.0f;
        LightPos.y = sin(glfwGetTime() / 2.0f) * 1.0f;
        LightPos.z = sin(glfwGetTime()) * 2.0f; */

        // Light uniforms
        ShaderProgram.Use();
        ShaderProgram.SetVec3("ViewPos", Camera -> Position);
        ShaderProgram.SetFloat("Material.Shininess", 64.0f);
        ShaderProgram.SetVec3("DirLight.Direction", -0.2f, -1.0f, -0.3f);
        ShaderProgram.SetVec3("DirLight.Ambient", 0.05f, 0.05f, 0.05f);
        ShaderProgram.SetVec3("DirLight.Diffuse", 0.4f, 0.4f, 0.4f);
        ShaderProgram.SetVec3("DirLight.Specular", 0.5f, 0.5f, 0.5f);
        // point light 1
        ShaderProgram.SetVec3("PointLights[0].Position", pointLightPositions[0]);
        ShaderProgram.SetVec3("PointLights[0].Ambient", 0.05f, 0.05f, 0.05f);
        ShaderProgram.SetVec3("PointLights[0].Diffuse", 0.8f, 0.8f, 0.8f);
        ShaderProgram.SetVec3("PointLights[0].Specular", 1.0f, 1.0f, 1.0f);
        ShaderProgram.SetFloat("PointLights[0].Constant", 1.0f);
        ShaderProgram.SetFloat("PointLights[0].Linear", 0.09f);
        ShaderProgram.SetFloat("PointLights[0].Quadratic", 0.032f);
        // point light 2
        ShaderProgram.SetVec3("PointLights[1].Position", pointLightPositions[1]);
        ShaderProgram.SetVec3("PointLights[1].Ambient", 0.05f, 0.05f, 0.05f);
        ShaderProgram.SetVec3("PointLights[1].Diffuse", 0.8f, 0.8f, 0.8f);
        ShaderProgram.SetVec3("PointLights[1].Specular", 1.0f, 1.0f, 1.0f);
        ShaderProgram.SetFloat("PointLights[1].Constant", 1.0f);
        ShaderProgram.SetFloat("PointLights[1].Linear", 0.09f);
        ShaderProgram.SetFloat("PointLights[1].Quadratic", 0.032f);
        // point light 3
        ShaderProgram.SetVec3("PointLights[2].Position", pointLightPositions[2]);
        ShaderProgram.SetVec3("PointLights[2].Ambient", 0.05f, 0.05f, 0.05f);
        ShaderProgram.SetVec3("PointLights[2].Diffuse", 0.8f, 0.8f, 0.8f);
        ShaderProgram.SetVec3("PointLights[2].Specular", 1.0f, 1.0f, 1.0f);
        ShaderProgram.SetFloat("PointLights[2].Constant", 1.0f);
        ShaderProgram.SetFloat("PointLights[2].Linear", 0.09f);
        ShaderProgram.SetFloat("PointLights[2].Quadratic", 0.032f);
        // point light 4
        ShaderProgram.SetVec3("PointLights[3].Position", pointLightPositions[3]);
        ShaderProgram.SetVec3("PointLights[3].Ambient", 0.05f, 0.05f, 0.05f);
        ShaderProgram.SetVec3("PointLights[3].Diffuse", 0.8f, 0.8f, 0.8f);
        ShaderProgram.SetVec3("PointLights[3].Specular", 1.0f, 1.0f, 1.0f);
        ShaderProgram.SetFloat("PointLights[3].Constant", 1.0f);
        ShaderProgram.SetFloat("PointLights[3].Linear", 0.09f);
        ShaderProgram.SetFloat("PointLights[3].Quadratic", 0.032f);
        // spotLight
        ShaderProgram.SetVec3("SpotLight.Position", Camera -> Position);
        ShaderProgram.SetVec3("SpotLight.Direction", Camera -> Front);
        ShaderProgram.SetVec3("SpotLight.Ambient", 0.0f, 0.0f, 0.0f);
        ShaderProgram.SetVec3("SpotLight.Diffuse", 1.0f, 1.0f, 1.0f);
        ShaderProgram.SetVec3("SpotLight.Specular", 1.0f, 1.0f, 1.0f);
        ShaderProgram.SetFloat("SpotLight.Constant", 1.0f);
        ShaderProgram.SetFloat("SpotLight.Linear", 0.09f);
        ShaderProgram.SetFloat("SpotLight.Quadratic", 0.032f);
        ShaderProgram.SetFloat("SpotLight.InnerCutOff", glm::cos(glm::radians(12.5f)));
        ShaderProgram.SetFloat("SpotLight.OuterCutOff", glm::cos(glm::radians(15.0f)));

        // Transform
        glm::mat4 projection = glm::perspective(glm::radians(Camera -> Zoom),
                (float)Setting -> GetScreenWidth() / (float)Setting -> GetScreenHeight(), 0.1f, 100.0f);
        ShaderProgram.SetMat4("projection", projection);

        glm::mat4 view = Camera -> GetViewMatrix();
        ShaderProgram.SetMat4("view", view);

        // World transformation
        glm::mat4 Model{1.f};
        ShaderProgram.SetMat4("model", Model);

        glActiveTexture(GL_TEXTURE0);
        DiffuseMap.Bind();
        glActiveTexture(GL_TEXTURE1);
        SpecularMap.Bind();
        glActiveTexture(GL_TEXTURE2);
        EmissionMap.Bind();

        // Render the cube
        glBindVertexArray(VAO);
        for (unsigned int i = 0; i < 10; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            ShaderProgram.SetMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        LightCubeShader.Use();
        LightCubeShader.SetMat4("projection", projection);
        LightCubeShader.SetMat4("view", view);

        glBindVertexArray(LightVAO);
        for (unsigned int i = 0; i < 4; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f));
            LightCubeShader.SetMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

		// Check and call events (Swap and buffers)
		glfwSwapBuffers(Window);
		glfwPollEvents();
	}

    // De-allocated all resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &LightVAO);
    glDeleteBuffers(1, &VBO);
	glfwTerminate();

	return 0;

}

void ProcessInput(GLFWwindow* Window)
{
    if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(Window, true);
    }

    if(glfwGetKey(Window, GLFW_KEY_J) == GLFW_PRESS)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursorPos(Window, LastX, LastY);
    }

    if(glfwGetKey(Window, GLFW_KEY_I) == GLFW_PRESS)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetCursorPos(Window, LastX, LastY);
    }

    if(glfwGetKey(Window, GLFW_KEY_F) == GLFW_PRESS && bCanChangeWireframe)
    {
        bCanChangeWireframe = false;
        Setting -> SetbWireFrame(!Setting -> GetbWireFrame()); // Toggling the wireframe mode
    }
    if(glfwGetKey(Window, GLFW_KEY_F) == GLFW_RELEASE)
    {
        bCanChangeWireframe = true;
    }
    if(glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
        Camera -> ProcessKeyboard(ECM_Forward, DeltaTime);
    if(glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
        Camera -> ProcessKeyboard(ECM_Backward, DeltaTime);
    if(glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
        Camera -> ProcessKeyboard(ECM_Left, DeltaTime);
    if(glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
        Camera -> ProcessKeyboard(ECM_Right, DeltaTime);
    if(glfwGetKey(Window, GLFW_KEY_SPACE) == GLFW_PRESS)
        Camera -> ProcessKeyboard(ECM_Up, DeltaTime);
    if(glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        Camera ->ProcessKeyboard((ECM_Down), DeltaTime);
}

void MouseCallback(GLFWwindow* Window, double xPosIn, double yPosIn)
{
    const float xPos = static_cast<float>(xPosIn);
    const float yPos = static_cast<float>(yPosIn);

    float xOffset = xPos - LastX;
    float yOffset = LastY - yPos; // reversed since y-coordinates go from bottom to top

    LastX = xPos;
    LastY = yPos;

    Camera -> ProcessMouseMovement(xOffset, yOffset);
}

void ScrollCallback(GLFWwindow* Window, double xOffset, double yOffset)
{
    Camera -> ProcessMouseScroll(static_cast<float>(yOffset), Setting -> GetCameraMaxFOV());
}