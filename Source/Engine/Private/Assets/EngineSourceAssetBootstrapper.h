//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

class ProjectContext;
class AssetManager;

class EngineSourceAssetBootstrapper
{
public:
    static bool Bootstrap(AssetManager& assetManager,
                          const ProjectContext& context);

private:
    static bool ProcessSourceFile(AssetManager& assetManager,
                                  const ProjectContext& context,
                                  const std::string& sourcePath);
};
