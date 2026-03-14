// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Framework/AssetManager.h"

#include "Assets/AssetRegistrySubsystem.h"
#include "Core/Project/VirtualPathMounter.h"

bool AssetManager::Initialize(AssetRegistrySubsystem* registry, VirtualPathMounter* pathMounter)
{
    m_Registry = registry;
    m_PathMounter = pathMounter;
    return m_Registry != nullptr && m_PathMounter != nullptr;
}

void AssetManager::Shutdown()
{
    m_Registry = nullptr;
    m_PathMounter = nullptr;
}

bool AssetManager::RebuildRegistry()
{
    if (!m_Registry || !m_PathMounter)
        return false;

    return m_Registry->Rebuild(*m_PathMounter);
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