// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Framework/AssetManager.h"

#include <iostream>

#include "Assets/AssetImportSubsystem.h"
#include "Assets/AssetRegistrySubsystem.h"
#include "Core/Project/VirtualPathMounter.h"

bool AssetManager::Initialize(AssetRegistrySubsystem* registry,
                              AssetImportSubsystem* importer,
                              VirtualPathMounter* pathMounter)
{
    m_Registry = registry;
    m_Importer = importer;
    m_PathMounter = pathMounter;
    return m_Registry != nullptr && m_Importer != nullptr && m_PathMounter != nullptr;
}

void AssetManager::Shutdown()
{
    m_Registry = nullptr;
    m_Importer = nullptr;
    m_PathMounter = nullptr;
}

bool AssetManager::RebuildRegistry()
{
    if (!m_Registry || !m_PathMounter)
        return false;

    return m_Registry->Rebuild(*m_PathMounter);
}

bool AssetManager::ImportAsset(const FAssetImportRequest& request, FAssetImportResult& outResult)
{
    outResult = {};

    if (!m_Importer || !m_Registry || !m_PathMounter)
    {
        std::cerr << "[AssetManager]: Asset manager is not initialized" << "\n";
        outResult.bSuccess = false;
        return false;
    }

    if (!m_Importer->Import(request, *m_PathMounter, outResult))
        return false;

    // simplest v1: rebuild whole registry after import
    if (!m_Registry->Rebuild(*m_PathMounter))
    {
        outResult.warnings.emplace_back("Asset imported, but registry rebuild reported errors.");
    }

    if (!outResult.errors.empty())
    {
        std::cerr << "[AssetManager]: Error(s) while loading " << request.sourceFilePath << ":\n";
        for (const auto& error : outResult.errors)
            std::cerr << error << "\n";

        outResult.bSuccess = false;
        return false;
    }

    if (!outResult.warnings.empty())
    {
        std::cout << "[AssetManager]: Warning(s) while loading " << request.sourceFilePath << ":\n";
        for (const auto& warning : outResult.warnings)
            std::cout << warning << "\n";
    }

    outResult.bSuccess = true;
    return true;
}

const FAssetRecord* AssetManager::FindByAssetID(const std::string& assetID) const
{
    return m_Registry ? m_Registry->FindByAssetID(assetID) : nullptr;
}

const FAssetRecord* AssetManager::FindByVirtualPath(const std::string& virtualPath) const
{
    return m_Registry ? m_Registry->FindByVirtualPath(virtualPath) : nullptr;
}

const FAssetRecord* AssetManager::FindByPhysicalPath(const std::string& physicalPath) const
{
    return m_Registry ? m_Registry->FindByPhysicalPath(physicalPath) : nullptr;
}

const std::vector<FAssetRecord>* AssetManager::GetAllAssets() const
{
    return m_Registry ? &m_Registry->GetAllAssets() : nullptr;
}

std::vector<const FAssetRecord*> AssetManager::GetAssetsByPrefix(const std::string& virtualPrefix) const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null" << "\n";
        return {};
    }

    return m_Registry->GetAssetsByPrefix(virtualPrefix);
}
