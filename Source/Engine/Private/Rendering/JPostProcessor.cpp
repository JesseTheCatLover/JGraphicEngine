// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JPostProcessor.h"

#include <glad/gl.h>
#include <iostream>

// ----------------- JScreenQuad Implementation -----------------
JPostProcessor::JScreenQuad::JScreenQuad() {
    // Fullscreen quad: pos.xy, texcoord.xy
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f, 0.0f, // bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f, // top-left
         1.0f, -1.0f,  1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f, 1.0f  // top-right
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // texcoord (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

JPostProcessor::JScreenQuad::~JScreenQuad() {
    if (VBO)  glDeleteBuffers(1, &VBO);
    if (VAO)  glDeleteVertexArrays(1, &VAO);
}

void JPostProcessor::JScreenQuad::Draw() const {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ----------------- JPostProcessor Implementation -----------------
JPostProcessor::JPostProcessor(const std::string& vertexPath, const std::string& fragmentPath)
    : ScreenQuad(), PostShader(vertexPath, fragmentPath)
{
    // Ensure the shader has the expected sampler uniform (optional sanity check).
    // You can optionally set a default texture unit here:
    PostShader.Use();
    PostShader.SetInt("screenTexture", 0);
}

void JPostProcessor::Apply(unsigned int inputTexture, int screenWidth, int screenHeight) {
    // Render a fullscreen quad that samples from inputTexture using PostShader.
    // The caller is responsible for binding an output framebuffer (if any).
    // We set the viewport to the provided size so the quad covers the target.
    glViewport(0, 0, screenWidth, screenHeight);

    // Disable depth test for screen-space quad rendering
    glDisable(GL_DEPTH_TEST);

    PostShader.Use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    // The constructor already set the uniform to 0, but set again in case users changed it.
    PostShader.SetInt("screenTexture", 0);

    ScreenQuad.Draw();

    // Unbind texture and restore depth test.
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
}

JShader& JPostProcessor::GetShader() {
    return PostShader;
}
