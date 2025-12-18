//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <algorithm>

#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"
#include "Rendering/RHandles.h"

struct FPassParam
{
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, int32_t> ints;

    std::unordered_map<std::string, FVector2> vec2s;
    std::unordered_map<std::string, FVector4> vec4s;

    // IMPORTANT: per-pass textures (LUTs, noise, blur kernels, etc.)
    // Frame textures can still override these in the renderer bind step.
    std::unordered_map<std::string, RTextureHandle> textures;
};

struct FPostPassDesc
{
    std::string name;
    bool bEnabled = true;
    FPassParam params;
};

struct FPostProfile
{
    std::vector<FPostPassDesc> chain;
    bool bDirty = true;
};

struct FFramePostParams
{
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, int32_t> ints;

    std::unordered_map<std::string, FVector2> vec2s;
    std::unordered_map<std::string, FVector4> vec4s;

    std::unordered_map<std::string, RTextureHandle> textures;
};

class PostProcessManager
{
    friend class JEngine;

private:
    PostProcessManager() = default;

    // profileId -> profile struct
    std::unordered_map<uint32_t, FPostProfile> m_Profiles;

    static const std::vector<FPostPassDesc>& EmptyChain()
    {
        static const std::vector<FPostPassDesc> kEmpty;
        return kEmpty;
    }

    FPostProfile* FindProfile(uint32_t profileId)
    {
        auto it = m_Profiles.find(profileId);
        return (it == m_Profiles.end()) ? nullptr : &it->second;
    }

    const FPostProfile* FindProfile(uint32_t profileId) const
    {
        auto it = m_Profiles.find(profileId);
        return (it == m_Profiles.end()) ? nullptr : &it->second;
    }

    FPostProfile& GetOrCreateProfile(uint32_t profileId)
    {
        auto [it, inserted] = m_Profiles.try_emplace(profileId, FPostProfile{});
        return it->second;
    }

    void MarkDirty(FPostProfile& p) { p.bDirty = true; }

public:
    ~PostProcessManager() = default;

    // Non-copyable / non-movable
    PostProcessManager(const PostProcessManager&) = delete;
    PostProcessManager& operator=(const PostProcessManager&) = delete;
    PostProcessManager(PostProcessManager&&) = delete;
    PostProcessManager& operator=(PostProcessManager&&) = delete;

    // Read-only access. Does NOT create profiles.
    [[nodiscard]] const std::vector<FPostPassDesc>& GetChain(uint32_t profileId = 0) const
    {
        const FPostProfile* p = FindProfile(profileId);
        return p ? p->chain : EmptyChain();
    }

    // Mutating access: creates profile + marks dirty.
    std::vector<FPostPassDesc>& EditChain(uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        MarkDirty(p);
        return p.chain;
    }

    // Query dirty without creating profiles.
    [[nodiscard]] bool IsDirty(uint32_t profileId = 0) const
    {
        const FPostProfile* p = FindProfile(profileId);
        return p ? p->bDirty : false;
    }

    // Renderer-friendly: clear dirty if profile exists.
    bool IsDirtyAndClear(uint32_t profileId = 0)
    {
        FPostProfile* p = FindProfile(profileId);
        if (!p) return false;
        const bool d = p->bDirty;
        p->bDirty = false;
        return d;
    }

    void AddPass(FPostPassDesc pass, uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        p.chain.push_back(std::move(pass));
        MarkDirty(p);
    }

    // Useful for UI insertion
    void InsertPass(size_t index, FPostPassDesc pass, uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        index = std::min(index, p.chain.size());
        p.chain.insert(p.chain.begin() + index, std::move(pass));
        MarkDirty(p);
    }

    bool RemovePass(size_t i, uint32_t profileId = 0)
    {
        FPostProfile* p = FindProfile(profileId);
        if (!p) return false;
        if (i >= p->chain.size()) return false;

        p->chain.erase(p->chain.begin() + i);
        MarkDirty(*p);
        return true;
    }

    // Correct move semantics + correct index adjustment
    bool MovePass(size_t from, size_t to, uint32_t profileId = 0)
    {
        FPostProfile* p = FindProfile(profileId);
        if (!p) return false;

        const size_t n = p->chain.size();
        if (from >= n) return false;

        // Allow "move to end" behavior if UI drags past last slot.
        to = std::min(to, n - 1);

        if (from == to) return true;

        FPostPassDesc pass = std::move(p->chain[from]);
        p->chain.erase(p->chain.begin() + from);

        // If we removed an earlier element, the target index shifts left by 1.
        if (from < to) --to;

        p->chain.insert(p->chain.begin() + to, std::move(pass));
        MarkDirty(*p);
        return true;
    }

    // Optional utilities (highly useful in editor tooling)
    bool RemoveProfile(uint32_t profileId)
    {
        return m_Profiles.erase(profileId) > 0;
    }

    void ClearProfile(uint32_t profileId = 0)
    {
        auto& p = GetOrCreateProfile(profileId);
        p.chain.clear();
        MarkDirty(p);
    }
};
