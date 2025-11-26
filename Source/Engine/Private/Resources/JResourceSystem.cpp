// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Resources/JResourceSystem.h"

void JResourceSystem::Shutdown()
{
    UnloadAll();
}

std::shared_ptr<JCoreObject> JResourceSystem::Get(const JAssetID& assetId) const
{
    std::shared_lock rlock(m_Mutex);
    auto it = m_ByAsset.find(assetId);
    return it == m_ByAsset.end() ? nullptr : it->second.ptr;
}

bool JResourceSystem::Has(const JAssetID& assetId) const
{
    std::shared_lock rlock(m_Mutex);
    return m_ByAsset.find(assetId) != m_ByAsset.end();
}

bool JResourceSystem::Unload(const JAssetID& assetId)
{
    BasePtr ptr;
    bool lastOwner = false;

    {
        std::unique_lock wlock(m_Mutex);
        auto it = m_ByAsset.find(assetId);
        if (it == m_ByAsset.end())
            return false;

        // Check refcount before copying out
        lastOwner = (it->second.ptr.use_count() == 1);
        ptr = it->second.ptr; // keep alive outside
        m_ByAsset.erase(it);
    }

    if (ptr && lastOwner)
    {
        if (auto* gpuResource = dynamic_cast<IGpuResource*>(ptr.get()))
            gpuResource->DestroyGpuResources(m_Device);
    }

    return true;
}

size_t JResourceSystem::UnloadUnused()
{
    std::vector<BasePtr> toDestroy;

    {
        std::unique_lock wlock(m_Mutex);
        for (auto it = m_ByAsset.begin(); it != m_ByAsset.end();)
        {
            BasePtr& sp = it->second.ptr;
            if (sp && sp.use_count() == 1) // only manager holds it
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

    // Outside the lock, release GPU caches
    for (auto& pointer : toDestroy)
        if (auto* gpuResource = dynamic_cast<IGpuResource*>(pointer.get()))
            gpuResource->DestroyGpuResources(m_Device);

    return toDestroy.size();
}

void JResourceSystem::UnloadAll()
{
    std::vector<BasePtr> toDestroy;
    {
        std::unique_lock wlock(m_Mutex);
        for (auto& [id, entry] : m_ByAsset)
            toDestroy.push_back(entry.ptr);

        m_ByAsset.clear();
    }

    for (auto& p : toDestroy)
        if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
            gpu->DestroyGpuResources(m_Device);
}

void JResourceSystem::DebugDump() const
{
    std::shared_lock rlock(m_Mutex);
    std::cout << "[JResourceSystem]: Cache list:\n";
    for (const auto& [id, entry] : m_ByAsset)
    {
        if (!entry.ptr) continue;
        std::cout << " assetId='" << id << "'"
                  << " type=" << entry.type.name()
                  << " refs=" << entry.ptr.use_count()
                  << "\n";
    }
}
