//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorAssetCache.h"

#include <iostream>
#include <filesystem>

#include "EditorRuntime.h"
#include "File/FileAPI.h"
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

    ScanAndLoadTextures(runtime, "Assets/Editor/Textures");
}

void EditorAssetCache::ScanAndLoadTextures(EditorRuntime& runtime, const std::string& rootDirRel)
{
    if (!UPath::DirectoryExists(rootDirRel))
    {
        std::cerr << "[EditorAssetCache]: Texture dir missing: " << rootDirRel << "\n";
        return;
    }

    // UPath returns ABSOLUTE paths
    const std::vector<std::string> filesAbs = UPath::ListFiles(
        rootDirRel,
        /*extension*/ "",
        /*recursive*/ true,
        /*case-insensitive*/ true
    );

    const fs::path rootAbs = UPath::ResolvePath(rootDirRel);

    size_t loaded = 0;

    for (const std::string& absStr : filesAbs)
    {
        const std::string ext = UPath::GetExtension(absStr); // no dot
        std::string extLower = ext;
        for (char& c : extLower) c = (char)std::tolower((unsigned char)c);

        if (!IsTextureExt(extLower))
            continue;

        const fs::path absPath(absStr);

        // rel file path WITH extension relative to root (Toolbar/Save.png)
        fs::path relFile = fs::relative(absPath, rootAbs);

        // Key WITHOUT extension (Toolbar/Save)
        fs::path relNoExt = relFile;
        relNoExt.replace_extension();

        std::string keyNorm = relNoExt.generic_string();
        for (char& c : keyNorm) if (c == '\\') c = '/'; // stable key: Toolbar/Run

        // Project-relative source path (Assets/Editor/Textures/Toolbar/Save.png)
        const std::string srcRel = UPath::Join(rootDirRel, relFile.generic_string());

        RTextureHandle handle = runtime.GetFile().LoadEditorTextureFromFile(srcRel.c_str(), true);
        if (!handle.IsValid())
        {
            std::cerr << "[EditorAssetCache]: Failed to load: " << srcRel << "\n";
            continue;
        }

        m_TextureMap[keyNorm] = handle;
        m_Textures.push_back(FEditorTextureAsset{ keyNorm, srcRel, handle });
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