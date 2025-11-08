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
    const std::string norm = NormalizeKey(key);
    std::unique_lock wlock(m_Mutex);

    auto it = m_ByKey.find(norm);
    if (it == m_ByKey.end()) return false;

    BasePtr ptr = it->second.ptr;
    m_ByKey.erase(it);
    m_ByID.erase(ptr->GetID());

    if (ptr.use_count() == 1)
        FGpuLifecycle::TryOnDestroyGpuResources(*ptr, m_Device, 0);

    return true;
}

size_t JResourceManager::UnloadUnused()
{
    std::unique_lock wlock(m_Mutex);
    size_t freed = 0;

    std::vector<std::string> dropKeys;
    dropKeys.reserve(m_ByKey.size());

    for (auto& [key, entry] : m_ByKey)
    {
        if (entry.ptr.use_count() == 1)
        {
            FGpuLifecycle::TryOnDestroyGpuResources(*entry.ptr, m_Device, 0);
            m_ByID.erase(entry.ptr->GetID());
            dropKeys.push_back(key);
            ++freed;
        }
    }

    for (const auto& k : dropKeys)
        m_ByKey.erase(k);

    return freed;
}

void JResourceManager::UnloadAll()
{
    std::unique_lock wlock(m_Mutex);

    for (auto& [key, entry] : m_ByKey)
    {
        if (entry.ptr)
            FGpuLifecycle::TryOnDestroyGpuResources(*entry.ptr, m_Device, 0);
    }

    m_ByKey.clear();
    m_ByID.clear();
}

std::string JResourceManager::NormalizeKey(std::string s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
    std::transform(s.begin(), s.end(), s.begin(), ToLowerASCII);
    return s;
}
