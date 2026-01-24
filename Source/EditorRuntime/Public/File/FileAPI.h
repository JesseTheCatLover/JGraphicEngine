//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Rendering/RHandles.h"

class ResourceSubsystem;
class EngineContext;

class EditorFileAPI
{
    private:
    EngineContext& m_Context;
    ResourceSubsystem& m_Resource;

public:
    EditorFileAPI(EngineContext& ctx, ResourceSubsystem& resource);

    RTextureHandle LoadEditorTextureFromFile(const char* filePath, bool bSRGB = true);
};
