// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JFramebufferTarget.h"
#include <iostream>

JFramebufferTarget::JFramebufferTarget(int width, int height, int samples)
    : Width(width), Height(height), Samples(samples)
{
    CreateBuffers();
}

JFramebufferTarget::~JFramebufferTarget()
{
    Destroy();
}

// --------------------- Internal Helpers ---------------------
void JFramebufferTarget::CreateBuffers()
{
    // Generate and bind framebuffer
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Generate color attachment texture
    glGenTextures(1, &ColorTex);

    if (Samples > 1) {
        // Multi-sampled color texture for MSAA
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ColorTex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GL_RGB, Width, Height, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ColorTex, 0);
    } else {
        // Standard single-sample texture
        glBindTexture(GL_TEXTURE_2D, ColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ColorTex, 0);
    }

    // Create depth-stencil renderbuffer
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    if (Samples > 1) {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, Samples, GL_DEPTH24_STENCIL8, Width, Height);
    } else {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Width, Height);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::JFramebufferTarget: Framebuffer is not complete!" << std::endl;
    }

    // Unbind to avoid accidental rendering
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void JFramebufferTarget::Destroy()
{
    if (RBO) glDeleteRenderbuffers(1, &RBO);
    if (ColorTex) glDeleteTextures(1, &ColorTex);
    if (FBO) glDeleteFramebuffers(1, &FBO);

    // Reset IDs to 0 to prevent dangling references
    FBO = 0;
    ColorTex = 0;
    RBO = 0;
}

// --------------------- Public Methods ---------------------
void JFramebufferTarget::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, Width, Height);
}

void JFramebufferTarget::Unbind(int screenWidth, int screenHeight) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void JFramebufferTarget::Resize(int newWidth, int newHeight)
{
    if (newWidth == Width && newHeight == Height) return; // Avoid unnecessary recreation

    Width = newWidth;
    Height = newHeight;

    Destroy();
    CreateBuffers();
}

void JFramebufferTarget::ResolveTo(JFramebufferTarget& target) const
{
    if (FBO == 0 || target.FBO == 0) return; // Safety check

    glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.FBO);

    glBlitFramebuffer(
        0, 0, Width, Height,
        0, 0, target.Width, target.Height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    // Restore default framebuffer binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
