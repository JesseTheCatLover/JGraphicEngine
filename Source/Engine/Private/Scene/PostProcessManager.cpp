// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/PostProcessManager.h"

#include "Rendering/JFramebufferTarget.h"
#include <glad/gl.h>
#include <iostream>
#include "Rendering/JPostProcessor.h"

PostProcessManager::PostProcessManager(int width, int height)
    : Width(width), Height(height)
{
    Ping = std::make_unique<JFramebufferTarget>(width, height, 1);
    Pong = std::make_unique<JFramebufferTarget>(width, height, 1);
}

void PostProcessManager::AddProcessor(std::unique_ptr<JPostProcessor> processor) {
    Processors.push_back(std::move(processor));
}

void PostProcessManager::ApplyChain(unsigned int inputTexture, int screenWidth, int screenHeight) {
    if (Processors.empty()) {
        AddProcessor(std::make_unique<JPostProcessor>());
    }

    bool horizontal = true;
    unsigned int currentTexture = inputTexture;

    for (size_t i = 0; i < Processors.size(); ++i) {
        const JFramebufferTarget* target = horizontal ? Ping.get() : Pong.get();

        // Bind target framebuffer and clear
        target->Bind();
        glClear(GL_COLOR_BUFFER_BIT);

        // Apply the processor using the current input texture
        Processors[i]->Apply(currentTexture, Width, Height);

        // Unbind framebuffer
        target->Unbind(screenWidth, screenHeight);

        // Output of this pass becomes input for next
        currentTexture = target->GetTexture();
        horizontal = !horizontal;
    }

    // After the last pass, render the final texture to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    Processors.back()->Apply(currentTexture, screenWidth, screenHeight);
}

void PostProcessManager::Resize(int newWidth, int newHeight) {
    Width = newWidth;
    Height = newHeight;

    Ping->Resize(newWidth, newHeight);
    Pong->Resize(newWidth, newHeight);
}
