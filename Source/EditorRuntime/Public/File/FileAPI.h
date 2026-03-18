//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Rendering/RHandles.h"

struct FAssetRecord;
class VirtualPathMounter;
class AssetManager;
class ResourceSubsystem;
class EngineContext;

class EditorFileAPI
{
    private:
    EngineContext& m_Context;
    ResourceSubsystem& m_Resource;
    AssetManager& m_AssetManager;
    VirtualPathMounter& m_PathMounter;

public:
    EditorFileAPI(EngineContext& ctx, ResourceSubsystem& resource, VirtualPathMounter& pathMounter, AssetManager& assetManager);

    RTextureHandle LoadEditorTextureFromFile(const char* virtualFilePath, bool bSRGB = true);

    [[nodiscard]] const std::vector<FAssetRecord>* GetAllAssets() const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByPrefix(const std::string& virtualPrefix) const;
};
