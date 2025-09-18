// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JPostProcessor.h"

#include <iostream>

// ----------------- JFrameBuffer Implementation -----------------
JPostProcessor::JFrameBuffer::JFrameBuffer(int width, int height)
    : Width(width), Height(height)
{
    // Generate framebuffer
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Color attachment (texture)
    glGenTextures(1, &ColorTex);
    glBindTexture(GL_TEXTURE_2D, ColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, ColorTex, 0);

    // Depth + stencil (renderbuffer)
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, RBO);

    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

JPostProcessor::JFrameBuffer::~JFrameBuffer()
{
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &ColorTex);
    glDeleteRenderbuffers(1, &RBO);
}

void JPostProcessor::JFrameBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, Width, Height);
}

void JPostProcessor::JFrameBuffer::Unbind(int screenWidth, int screenHeight) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void JPostProcessor::JFrameBuffer::Resize(int newWidth, int newHeight)
{
    Width = newWidth;
    Height = newHeight;

    // Resize color texture
    glBindTexture(GL_TEXTURE_2D, ColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Resize depth + stencil renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Width, Height);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

// ----------------- JScreenQuad Implementation -----------------
JPostProcessor::JScreenQuad::JScreenQuad() {
    float quadVertices[] = {
        // positions(x, y)   // texCoords(u, v)
        -1.f,  1.f,          0.f, 1.f, // top-left
        -1.f, -1.f,          0.f, 0.f, // bottom-left
         1.f, -1.f,          1.f, 0.f, // bottom-right

        -1.f,  1.f,          0.f, 1.f, // top-left
         1.f, -1.f,          1.f, 0.f, // bottom-right
         1.f,  1.f,          1.f, 1.f  // top-right
    };

    // Upload the vertex data to the GPU.
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Set the attribute pointers
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

JPostProcessor::JScreenQuad::~JScreenQuad() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void JPostProcessor::JScreenQuad::Draw() const {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ----------------- JPostProcessor Implementation -----------------
JPostProcessor::JPostProcessor(int width, int height, const std::string& vertexPath, const std::string& fragmentPath)
    : FBO(width, height), ScreenQuad(), PostShader(vertexPath, fragmentPath)
{}

void JPostProcessor::Begin() {
    FBO.Bind();
}

void JPostProcessor::End(int screenWidth, int screenHeight) {
    FBO.Unbind(screenWidth, screenHeight);

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw post-processed quad
    glDisable(GL_DEPTH_TEST); // Depth testing is disable for rendering screen-space quad
    PostShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FBO.ColorTex);
    PostShader.SetInt("screenTexture", 0);
    ScreenQuad.Draw();
    glEnable(GL_DEPTH_TEST); // Restore depth testing
}

void JPostProcessor::Resize(int newWidth, int newHeight)
{
    FBO.Resize(newWidth, newHeight);
}

JShader& JPostProcessor::GetShader() {
    return PostShader;
}

unsigned int JPostProcessor::GetTexture() const {
    return FBO.ColorTex;
}