//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>

struct FPassParam {
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, int> ints;
};

struct FPostPassDesc {
    std::string name;
    bool bEnabled = true;
    FPassParam params;
};

struct FPostProfile
{
    std::vector<FPostPassDesc> chains;
    bool bDirty = true;
};

class PostProcessManager
{
    friend class JEngine;
private:
    PostProcessManager()  = default;

    // profileId -> profile struct
    std::unordered_map<uint32_t, FPostProfile> m_Profiles;

    // Ensure a profile exists and return it
    FPostProfile& GetOrCreateProfile(uint32_t profileId)
    {
        auto it = m_Profiles.find(profileId);
        if (it == m_Profiles.end())
        {
            auto [insertIt, _] = m_Profiles.emplace(profileId, FPostProfile{});
            return insertIt->second;
        }
        return it->second;
    }

public:
    ~PostProcessManager() = default;

    // Non-copyable / non-movable
    PostProcessManager(const PostProcessManager&) = delete;
    PostProcessManager& operator=(const PostProcessManager&) = delete;
    PostProcessManager(PostProcessManager&&) = delete;
    PostProcessManager& operator=(PostProcessManager&&) = delete;

    [[nodiscard]] const std::vector<FPostPassDesc>& GetChain(uint32_t profileId = 0) const
    {
        return const_cast<PostProcessManager*>(this)->GetOrCreateProfile(profileId).chains;
    }

    std::vector<FPostPassDesc>& EditChain(uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        p.bDirty = true;
        return p.chains;
    }

    bool IsDirtyAndClear(uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        bool d = p.bDirty;
        p.bDirty = false;
        return d;
    }

    void AddPass(FPostPassDesc pass, uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        p.chains.push_back(std::move(pass));
        p.bDirty = true;
    }

    void RemovePass(size_t i, uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        if (i < p.chains.size())
        {
            p.chains.erase(p.chains.begin() + i);
            p.bDirty = true;
        }
    }

    void MovePass(size_t from, size_t to, uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        if (from < p.chains.size() && to < p.chains.size())
        {
            auto pass = p.chains[from];
            p.chains.erase(p.chains.begin() + from);
            p.chains.insert(p.chains.begin() + to, pass);
            p.bDirty = true;
        }
    }
};