//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/AssetCacheService.h"

#include "EditorRuntime.h"
#include "Assets/FAssetRecord.h"
#include "EditorAPI/File/FileAPI.h"
#include "Utilities/UPath.h"

AssetCacheService::~AssetCacheService()
{
}

AssetCacheService::AssetCacheService(EditorRuntime& runtime)
    : m_Runtime(runtime)
{
}

void AssetCacheService::PreloadAll()
{
    m_TextureMap.clear();
    m_Textures.clear();

    ScanAndLoadTextures(m_Runtime);
}

void AssetCacheService::ScanAndLoadTextures(EditorRuntime& runtime)
{
    const std::string kTexturePathInEngine = "/Engine/Editor/Textures/";
    auto assets = runtime.GetFile().FindAllAssetsByVirtualPathPrefix(kTexturePathInEngine);

    for (const FAssetRecord* asset : assets)
    {
        RTextureHandle handle = runtime.GetFile().LoadEditorTextureFromFile(asset->virtualPath.c_str(), true);

        if (!handle.IsValid())
            continue;

        std::string rel = asset->virtualPath.substr(kTexturePathInEngine.length());
        std::string key = UPath::RemoveExtension(rel);

        m_TextureMap[key] = handle;
        m_Textures.push_back({ key, asset->virtualPath, handle });
    }
}

RTextureHandle AssetCacheService::GetTexture(std::string_view key) const
{
    auto it = m_TextureMap.find(UPath::RemoveExtension(std::string(key)));
    if (it == m_TextureMap.end())
        return {};
    return it->second;
}