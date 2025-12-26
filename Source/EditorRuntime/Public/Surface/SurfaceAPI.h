//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

enum class ECursorMode;
class IPlatformSurface;
class EngineContext;

class EditorSurfaceAPI
{
private:
    EngineContext& m_Context;
    IPlatformSurface& m_PlatformSurface;

public:
    EditorSurfaceAPI(EngineContext& ctx, IPlatformSurface& surface);

    void SetCursorDisabled();
    void SetCursorHidden();
    void SetCursorVisible();
};
