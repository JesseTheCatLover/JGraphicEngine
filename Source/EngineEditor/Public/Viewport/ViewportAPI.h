//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FEditorFrameSnapshot.h"
#include "Rendering/RHandles.h"
#include "Utilities/UDynamicID.h"

class RendererSubsystem;
class EngineContext;

class EditorViewportAPI
{
private:
    EngineContext& m_Context;
    RendererSubsystem& m_Renderer;

public:
    EditorViewportAPI(EngineContext& ctx, RendererSubsystem& renderer);

    [[nodiscard]] FEditorFrameSnapshot GetFrameSnapshot() const;
};
