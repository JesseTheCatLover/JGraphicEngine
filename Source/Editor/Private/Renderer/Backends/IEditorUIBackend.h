//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class IEditorUIBackend
{
public:
    virtual ~IEditorUIBackend() = default;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Shutdown() = 0;
};