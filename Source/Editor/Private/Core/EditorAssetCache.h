//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Rendering/RHandles.h"

class EditorRuntime;

struct FEditorTextureAsset
{
    std::string Key;         // e.g. "Icons/Move"
    std::string SourcePath;  // absolute or relative path
    RTextureHandle Handle{};
};

class EditorAssetCache
{
public:
    // Call once at editor startup
    void PreloadAll(EditorRuntime& runtime);

    // Lookup
    [[nodiscard]] RTextureHandle GetTexture(std::string_view key) const;

    [[nodiscard]] const std::vector<FEditorTextureAsset>& GetAllTextures() const { return m_Textures; }

private:
    void ScanAndLoadTextures(EditorRuntime& runtime, const std::string& rootVirtualDir);

private:
    std::unordered_map<std::string, RTextureHandle> m_TextureMap;
    std::vector<FEditorTextureAsset> m_Textures;
};
