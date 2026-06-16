//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/EditorAssetCache.h"

#include "EditorRuntime.h"
#include "Assets/FAssetRecord.h"
#include "EditorAPI/File/FileAPI.h"
#include "Utilities/UPath.h"

void EditorAssetCache::PreloadAll(EditorRuntime& runtime)
{
    m_TextureMap.clear();
    m_Textures.clear();

    ScanAndLoadTextures(runtime);
}

void EditorAssetCache::ScanAndLoadTextures(EditorRuntime& runtime)
{
    const std::string kTexturePathInEngine = "/Engine/Editor/Textures/";
    auto assets = runtime.GetFile().FindAllAssetsByVirtualPathPrefix(kTexturePathInEngine);

    for (const FAssetRecord* asset : assets)
    {
        RTextureHandle handle =
            runtime.GetFile().LoadEditorTextureFromFile(asset->virtualPath.c_str(), true);

        if (!handle.IsValid())
            continue;

        std::string rel = asset->virtualPath.substr(kTexturePathInEngine.length());
        std::string key = UPath::RemoveExtension(rel);

        m_TextureMap[key] = handle;
        m_Textures.push_back({ key, asset->virtualPath, handle });
    }
}

RTextureHandle EditorAssetCache::GetTexture(std::string_view key) const
{
    auto it = m_TextureMap.find(std::string(key));
    if (it == m_TextureMap.end())
        return {};
    return it->second;
}