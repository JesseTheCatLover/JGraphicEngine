//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "EditorCore/IEditorService.h"
#include "Rendering/RHandles.h"

class EditorRuntime;

struct FEditorTextureAsset
{
    std::string key;         // e.g. "Icons/Move"
    std::string sourcePath;  // absolute or relative path
    RTextureHandle handle{};
};

class AssetCacheService : public IEditorService
{
private:
    EditorRuntime& m_Runtime;

    std::unordered_map<std::string, RTextureHandle> m_TextureMap;
    std::vector<FEditorTextureAsset> m_Textures;

public:
    ~AssetCacheService() override;
    explicit AssetCacheService(EditorRuntime& runtime);

    // Call once at editor startup
    void PreloadAll();

    // Lookup
    [[nodiscard]] RTextureHandle GetTexture(std::string_view key) const;

    [[nodiscard]] const std::vector<FEditorTextureAsset>& GetAllTextures() const { return m_Textures; }

private:
    void ScanAndLoadTextures(EditorRuntime& runtime);
};
