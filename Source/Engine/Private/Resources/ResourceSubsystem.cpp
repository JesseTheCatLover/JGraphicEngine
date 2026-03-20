// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Resources/ResourceSubsystem.h"
#include <vector>

void ResourceSubsystem::Shutdown()
{
    UnloadAll();
}

TSharedPtr<ICpuResource> ResourceSubsystem::Get(const JAssetID& assetId) const
{
    std::shared_lock rlock(m_Mutex);
    auto it = m_ByAsset.find(assetId);
    return it == m_ByAsset.end() ? nullptr : it->second.ptr;
}

bool ResourceSubsystem::Has(const JAssetID& assetId) const
{
    std::shared_lock rlock(m_Mutex);
    return m_ByAsset.find(assetId) != m_ByAsset.end();
}

bool ResourceSubsystem::Unload(const JAssetID& assetId)
{
    BasePtr ptr;
    bool lastOwner = false;

    {
        std::unique_lock wlock(m_Mutex);
        auto it = m_ByAsset.find(assetId);
        if (it == m_ByAsset.end())
            return false;

        lastOwner = (it->second.ptr.use_count() == 1);
        ptr = it->second.ptr;
        m_ByAsset.erase(it);
    }

    if (ptr && lastOwner && m_Device)
    {
        if (auto* gpuResource = dynamic_cast<IGpuResource*>(ptr.get()))
            gpuResource->DestroyGpuResources();
    }

    return true;
}

size_t ResourceSubsystem::UnloadUnused()
{
    std::vector<BasePtr> toDestroy;

    {
        std::unique_lock wlock(m_Mutex);
        for (auto it = m_ByAsset.begin(); it != m_ByAsset.end();)
        {
            BasePtr& sp = it->second.ptr;
            if (sp && sp.use_count() == 1)
            {
                toDestroy.push_back(sp);
                it = m_ByAsset.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto& pointer : toDestroy)
    {
        if (m_Device)
            if (auto* gpuResource = dynamic_cast<IGpuResource*>(pointer.get()))
                gpuResource->DestroyGpuResources();
    }

    return toDestroy.size();
}

void ResourceSubsystem::UnloadAll()
{
    std::vector<BasePtr> toDestroy;

    {
        std::unique_lock wlock(m_Mutex);
        for (auto& [id, entry] : m_ByAsset)
            toDestroy.push_back(entry.ptr);

        m_ByAsset.clear();
    }

    for (auto& p : toDestroy)
    {
        if (m_Device)
            if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
                gpu->DestroyGpuResources();
    }
}

void ResourceSubsystem::DebugDump() const
{
    std::shared_lock rlock(m_Mutex);

    std::cout << "[ResourceSubsystem]: Cache list:\n";

    for (const auto& [id, entry] : m_ByAsset)
    {
        if (!entry.ptr) continue;

        std::cout << " assetId='" << id << "'"
                  << " type=" << entry.type.name()
                  << " refs=" << entry.ptr.use_count()
                  << "\n";
    }
}
