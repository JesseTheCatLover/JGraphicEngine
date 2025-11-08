// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Resources/JResourceManager.h"
#include <algorithm>

/**
 * @brief Converts uppercase ASCII to lowercase.
 */
static inline char ToLowerASCII(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

std::shared_ptr<JCoreObject> JResourceManager::Get(const std::string& key) const
{
    const std::string norm = NormalizeKey(key);
    std::shared_lock rlock(m_Mutex);
    auto it = m_ByKey.find(norm);
    return it == m_ByKey.end() ? nullptr : it->second.ptr;
}

std::shared_ptr<JCoreObject> JResourceManager::GetByID(uint64_t id) const
{
    std::shared_lock rlock(m_Mutex);
    auto it = m_ByID.find(id);
    return it == m_ByID.end() ? nullptr : it->second;
}

bool JResourceManager::Has(const std::string& key) const
{
    const std::string norm = NormalizeKey(key);
    std::shared_lock rlock(m_Mutex);
    return m_ByKey.find(norm) != m_ByKey.end();
}

bool JResourceManager::Unload(const std::string& key)
{
    const std::string normalizedKey = NormalizeKey(key);
    std::unique_lock wlock(m_Mutex);

    auto it = m_ByKey.find(normalizedKey);
    if (it == m_ByKey.end()) return false;

    BasePtr ptr = it->second.ptr;
    m_ByKey.erase(it);
    m_ByID.erase(ptr->GetID());

    // If the cache held the last strong ref, allow GPU teardown.
    if (ptr.use_count() == 1)
    {
        if (auto* gpuResource = dynamic_cast<IGpuResource*>(ptr.get()))
            gpuResource->DestroyGpuResources(m_Device);
    }

    return true;
}

size_t JResourceManager::UnloadUnused()
{
    std::vector<BasePtr> toDestroy;
    {
        std::unique_lock wlock(m_Mutex);
        for (auto it = m_ByKey.begin(); it != m_ByKey.end();)
        {
            if (it->second.ptr.use_count() == 1)
            {
                m_ByID.erase(it->second.ptr->GetID());
                toDestroy.push_back(it->second.ptr);
                it = m_ByKey.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Outside the lock
    for (auto& pointer : toDestroy)
        if (auto* gpuResource = dynamic_cast<IGpuResource*>(pointer.get()))
            gpuResource->DestroyGpuResources(m_Device);

    return toDestroy.size();
}

void JResourceManager::UnloadAll()
{
    std::vector<BasePtr> toDestroy;
    {
        std::unique_lock wlock(m_Mutex);
        for (auto& [_, entry] : m_ByKey) toDestroy.push_back(entry.ptr);
        m_ByKey.clear();
        m_ByID.clear();
    }

    for (auto& p : toDestroy)
        if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
            gpu->DestroyGpuResources(m_Device);
}

std::string JResourceManager::NormalizeKey(std::string s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
    std::transform(s.begin(), s.end(), s.begin(), ToLowerASCII);
    return s;
}