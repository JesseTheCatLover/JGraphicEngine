//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class RendererSubsystem;
class EngineContext;

class EditorViewportAPI
{
private:
    EngineContext& m_Context;
    RendererSubsystem& m_Renderer;

public:
    EditorViewportAPI(EngineContext& ctx, RendererSubsystem& renderer);

    void SubmitRenderView(const struct FRenderView& view) const;

    void* GetNativeTexture(const struct RTextureHandle& handle);

    void CreateViewportTarget(int width, int height, struct FViewportRT& outRT);
    void DestroyViewportTarget(FViewportRT& rt);
};
