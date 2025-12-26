//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Viewport/ViewportAPI.h"

#include "Core/EngineContext.h"
#include "Rendering/FViewportRT.h"
#include "Rendering/RendererSubsystem.h"

EditorViewportAPI::EditorViewportAPI(EngineContext &ctx, RendererSubsystem &renderer):
m_Context(ctx),
m_Renderer(renderer)
{
}

void EditorViewportAPI::SubmitRenderView(const FRenderView &view) const
{
    m_Context.SubmitViewSource(view);
}

void* EditorViewportAPI::GetNativeTexture(const RTextureHandle &handle)
{
    return m_Renderer.GetNativeTextureHandle(handle);
}

void EditorViewportAPI::CreateViewportTarget(int width, int height, FViewportRT &outRT)
{
    if (width <= 0 || height <= 0)
        return;

    // Destroy old if still alive
    DestroyViewportTarget(outRT);

    RTextureHandle color{};
    RFramebufferHandle fbo = m_Renderer.CreateColorTarget(width, height, color);

    outRT.fbo = fbo;
    outRT.color = color;
    outRT.width = width;
    outRT.height = height;
}

void EditorViewportAPI::DestroyViewportTarget(FViewportRT &rt)
{
    if (rt.fbo.IsValid())
    {
        m_Renderer.DestroyColorTarget(rt.fbo);
        rt = {};
    }
}
