//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorAssetCache.h"

#include <iostream>
#include <filesystem>

#include "EditorRuntime.h"
#include "Core/EngineGlobals.h"
#include "Core/Project/VirtualPathMounter.h"
#include "File/FileAPI.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace fs = std::filesystem;

static bool IsTextureExt(const std::string& extLowerNoDot)
{
    return extLowerNoDot == "png" || extLowerNoDot == "jpg" || extLowerNoDot == "jpeg";
}

void EditorAssetCache::PreloadAll(EditorRuntime& runtime)
{
    m_TextureMap.clear();
    m_Textures.clear();

    if (!GEngine)
    {
        std::cerr << "[EditorAssetCache]: GEngine is null.\n";
        return;
    }

    ScanAndLoadTextures(runtime, "/Engine/Editor/Textures");
}

void EditorAssetCache::ScanAndLoadTextures(EditorRuntime& runtime, const std::string& rootVirtualDir)
{
    std::string rootAbs;
    if (!GEngine->GetVirtualPathMounter().ResolveVirtualToPhysical(rootVirtualDir, rootAbs))
    {
        std::cerr << "[EditorAssetCache]: Failed to resolve texture root: " << rootVirtualDir << "\n";
        return;
    }

    if (!UFileSystem::DirectoryExists(rootAbs))
    {
        std::cerr << "[EditorAssetCache]: Texture dir missing: " << rootAbs << "\n";
        return;
    }

    const std::vector<std::string> filesAbs = UFileSystem::ListFiles(
        rootAbs,
        "",
        true,
        true
    );

    const fs::path rootAbsPath(rootAbs);

    size_t loaded = 0;

    for (const std::string& absStr : filesAbs)
    {
        const std::string ext = UPath::GetExtension(absStr);
        std::string extLower = ext;
        for (char& c : extLower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (!IsTextureExt(extLower))
            continue;

        const fs::path absPath(absStr);

        fs::path relFile = fs::relative(absPath, rootAbsPath);

        fs::path relNoExt = relFile;
        relNoExt.replace_extension();

        std::string keyNorm = relNoExt.generic_string();
        for (char& c : keyNorm)
            if (c == '\\') c = '/';

        const std::string srcVirtual = UPath::Join(rootVirtualDir, relFile.generic_string());

        RTextureHandle handle = runtime.GetFile().LoadEditorTextureFromFile(srcVirtual.c_str(), true);
        if (!handle.IsValid())
        {
            std::cerr << "[EditorAssetCache]: Failed to load: " << srcVirtual << "\n";
            continue;
        }

        m_TextureMap[keyNorm] = handle;
        m_Textures.push_back(FEditorTextureAsset{ keyNorm, srcVirtual, handle });
        ++loaded;
    }
}

RTextureHandle EditorAssetCache::GetTexture(std::string_view key) const
{
    auto it = m_TextureMap.find(std::string(key));
    if (it == m_TextureMap.end())
        return {};
    return it->second;
}