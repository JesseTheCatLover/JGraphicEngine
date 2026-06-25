//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

class ProjectContext;
class VirtualPathMounter;
class AssetImportSubsystem;

class EngineSourceAssetBootstrapper // TODO: Make the source file processing smarter: - Detect old vs new version, - Remove old version
{
public:
    static bool Bootstrap(AssetImportSubsystem& assetImporter,
                          const ProjectContext& context,
                          const VirtualPathMounter& pathMounter);

private:
    static bool ProcessSourceFile(AssetImportSubsystem& assetImporter,
                                  const ProjectContext& context,
                                  const VirtualPathMounter& pathMounter,
                                  const std::string& sourcePath);
};
