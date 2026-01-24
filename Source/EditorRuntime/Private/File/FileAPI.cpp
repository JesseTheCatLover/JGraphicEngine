//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "File/FileAPI.h"

#include "Resources/ResourceSubsystem.h"
#include "Resources/GpuResources/Texture2DResource.h"

EditorFileAPI::EditorFileAPI(EngineContext &ctx, ResourceSubsystem &resource):
m_Context(ctx),
m_Resource(resource)
{
}

RTextureHandle EditorFileAPI::LoadEditorTextureFromFile(const char *filePath, bool bSRGB)
{
    if (!filePath || !filePath[0]) return {};

    std::string path = filePath;
    for (char& c : path) if (c == '\\') c = '/';

    std::string key = "EditorTex:" + path;

    Texture2DResource::FDesc desc;
    desc.path = path;
    desc.bSRGB = bSRGB;
    desc.bFlipY = true;

    auto res = m_Resource.Load<Texture2DResource>(key.c_str(), desc);

    return res ? res->GetTexture() : RTextureHandle{};
}
