//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Viewport/ViewportAPI.h"

#include "Core/EngineContext.h"
#include "Rendering/RendererSubsystem.h"

EditorViewportAPI::EditorViewportAPI(EngineContext &ctx, RendererSubsystem &renderer):
m_Context(ctx),
m_Renderer(renderer)
{
}

FEditorFrameSnapshot EditorViewportAPI::GetFrameSnapshot() const
{
    FEditorFrameSnapshot info{};

    info.deltaTime = m_Context.GetDeltaTime();
    info.framebufferWidth = m_Context.GetFramebufferWidth();
    info.framebufferHeight = m_Context.GetFramebufferHeight();
    info.aspectRatio = (info.framebufferHeight > 0)
                          ? float(info.framebufferWidth) / float(info.framebufferHeight)
                          : 0.0f;

    return info;
}