//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <utility>
#include <initializer_list>

struct REMeta
{
    std::string key;
    std::string value; // empty => flag
};

using REMetaList = std::vector<REMeta>;

inline void REAddMeta(REMetaList& out, std::string key, std::string value = {})
{
    out.push_back({ std::move(key), std::move(value) });
}

inline const char* REMetaFindValue(const REMetaList& meta, const char* key)
{
    if (!key) return nullptr;
    for (const auto& m : meta)
        if (m.key == key)
            return m.value.c_str();
    return nullptr;
}

inline bool REMetaHasFlag(const REMetaList& meta, const char* key)
{
    if (!key) return false;
    for (const auto& m : meta)
        if (m.key == key && m.value.empty())
            return true;
    return false;
}

// Optional ergonomic builders (only for hand-written tests/tools)
inline REMeta Category(const char* name) { return { "Category", name ? name : "" }; }
inline REMeta Tooltip(const char* tip) { return { "Tooltip", tip ? tip : "" }; }
inline REMeta Range(double minVal, double maxVal)
{
    return { "Range", std::to_string(minVal) + "," + std::to_string(maxVal) };
}
inline REMeta VisibleToScript() { return { "VisibleToScript", "" }; }

inline REMetaList MakeMeta(std::initializer_list<REMeta> list)
{
    return REMetaList(list.begin(), list.end());
}