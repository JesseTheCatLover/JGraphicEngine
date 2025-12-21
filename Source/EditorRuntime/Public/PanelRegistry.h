//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>

#include "Utilities/UDynamicID.h"

using PanelID = UDynamicID::IDType;

class PanelRegistry
{
public:
    PanelID GetOrCreate(const char* panelKey)
    {
        auto it = m_KeyToID.find(panelKey);
        if (it != m_KeyToID.end())
            return it->second;

        PanelID id = m_IDs.Allocate();
        m_KeyToID.emplace(panelKey, id);
        m_IDToKey.emplace(id, panelKey);
        return id;
    }

    PanelID Find(const char* panelKey) const
    {
        auto it = m_KeyToID.find(panelKey);
        return (it != m_KeyToID.end()) ? it->second : UDynamicID::InvalidID;
    }

    void Release(const char* panelKey)
    {
        auto it = m_KeyToID.find(panelKey);
        if (it == m_KeyToID.end())
            return;

        PanelID id = it->second;
        m_KeyToID.erase(it);
        m_IDToKey.erase(id);

        if (id != UDynamicID::InvalidID)
            m_IDs.Free(id);
    }

    const std::string* FindKey(PanelID id) const
    {
        auto it = m_IDToKey.find(id);
        return (it != m_IDToKey.end()) ? &it->second : nullptr;
    }

private:
    UDynamicID m_IDs;
    std::unordered_map<std::string, PanelID> m_KeyToID;
    std::unordered_map<PanelID, std::string> m_IDToKey;
};
