//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FEditorFrameSnapshot.h"
#include "Rendering/RHandles.h"

class RendererSubsystem;
class EngineContext;

class EditorViewportAPI
{
    friend class EditorCore;
private:
    EngineContext& m_Context;
    RendererSubsystem& m_Renderer;

public:
    EditorViewportAPI(EngineContext& ctx, RendererSubsystem& renderer);

    [[nodiscard]] FEditorFrameSnapshot GetFrameSnapshot() const;

    [[nodiscard]] RTextureHandle GetViewportColor() const;

    [[nodiscard]] void* GetNativeTextureHandle(RTextureHandle handle) const;
private:
};
