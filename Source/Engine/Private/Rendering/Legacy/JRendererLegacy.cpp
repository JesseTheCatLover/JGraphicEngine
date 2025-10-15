//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JRendererLegacy.h"
#include "JFramebufferTarget.h"
#include <glad/gl.h>
#include <memory>

JRendererLegacy::JRendererLegacy(int screenWidth, int screenHeight, int samples)
    : ScreenWidth(screenWidth), ScreenHeight(screenHeight), Samples(samples)
{
    SceneTarget = std::make_unique<JFramebufferTarget>(screenWidth, screenHeight, samples);
    ResolveTarget = std::make_unique<JFramebufferTarget>(screenWidth, screenHeight, 1); // always single-sample
}

JRendererLegacy::~JRendererLegacy()
{
}

void JRendererLegacy::BeginScene() {
    SceneTarget->Bind();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, ScreenWidth, ScreenHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void JRendererLegacy::EndScene() {
    SceneTarget->Unbind(ScreenWidth, ScreenHeight);

    // Resolve multi-sampled scene to single-sample texture
    if (SceneTarget->GetSamples() > 1)
        SceneTarget->ResolveTo(*ResolveTarget);
}

void JRendererLegacy::Resize(int newWidth, int newHeight) {
    ScreenWidth = newWidth;
    ScreenHeight = newHeight;
    SceneTarget->Resize(newWidth, newHeight);
    ResolveTarget->Resize(newWidth, newHeight);
}

unsigned int JRendererLegacy::GetSceneTargetTexture() const {
    // If multi-sampled, return resolved texture; otherwise, return scene texture
    return SceneTarget->GetSamples() > 1 ? ResolveTarget->GetTexture() : SceneTarget->GetTexture();
}
