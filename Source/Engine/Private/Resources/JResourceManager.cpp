// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Resources/JResourceManager.h"
#include <iostream>

std::optional<JResourceManager::ResourcePtr> JResourceManager::Get(const std::string& key) const
{
    auto it = m_ResourcesByKey.find(key);
    if (it != m_ResourcesByKey.end())
        return it->second;
    return std::nullopt;
}

std::optional<JResourceManager::ResourcePtr> JResourceManager::GetByID(uint64_t id) const
{
    auto it = m_ResourcesByID.find(id);
    if (it != m_ResourcesByID.end())
        return it->second;
    return std::nullopt;
}

bool JResourceManager::Has(const std::string& key) const
{
    return m_ResourcesByKey.find(key) != m_ResourcesByKey.end();
}

bool JResourceManager::Unload(const std::string& key)
{
    auto it = m_ResourcesByKey.find(key);
    if (it == m_ResourcesByKey.end())
        return false;

    uint64_t id = it->second->GetID();
    m_ResourcesByKey.erase(it);
    m_ResourcesByID.erase(id);
    return true;
}

void JResourceManager::UnloadAll()
{
    m_ResourcesByKey.clear();
    m_ResourcesByID.clear();
}