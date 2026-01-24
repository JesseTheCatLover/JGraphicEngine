//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorAssetCache.h"
#include <filesystem>
#include <cctype>

#include "EditorRuntime.h"
#include "File/FileAPI.h"

static bool IsTextureFile(const std::filesystem::path& p)
{
    std::string ext = p.extension().string();
    for (char& c : ext) c = (char)std::tolower((unsigned char)c);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
}

static std::string NormalizeKey(std::string s)
{
    for (char& c : s) if (c == '\\') c = '/';
    return s;
}

void EditorAssetCache::PreloadAll(EditorRuntime& runtime)
{
    m_TextureMap.clear();
    m_Textures.clear();

    ScanAndLoadTextures(runtime, "Assets/Editor/Textures");
}

void EditorAssetCache::ScanAndLoadTextures(EditorRuntime& runtime, const std::string& rootDir)
{
    namespace fs = std::filesystem;

    fs::path root(rootDir);
    if (!fs::exists(root) || !fs::is_directory(root))
        return;

    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path path = entry.path();
        if (!IsTextureFile(path))
            continue;

        fs::path rel = fs::relative(path, root);
        rel.replace_extension(); // drop .png/.jpg/.jpeg

        std::string key = NormalizeKey(rel.string());   // Icons/Move
        std::string src = NormalizeKey(path.string());  // Assets/Editor/Icons/Move.png

        // ✅ load through dedicated API
        RTextureHandle handle = runtime.GetFile().LoadEditorTextureFromFile(src.c_str(), /*srgb*/ true);
        if (!handle.IsValid())
            continue;

        // if duplicates exist, keep first
        if (m_TextureMap.find(key) == m_TextureMap.end())
            m_TextureMap[key] = handle;

        m_Textures.push_back(FEditorTextureAsset{ key, src, handle });
    }
}

RTextureHandle EditorAssetCache::GetTexture(std::string_view key) const
{
    auto it = m_TextureMap.find(std::string(key));
    if (it == m_TextureMap.end())
        return {};
    return it->second;
}
