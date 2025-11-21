//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <initializer_list>
#include <cstdint>

enum class EMetaKind : uint8_t
{
    Category,
    Range,
    Tooltip,
    VisibleToScript,
    Custom
};

struct FMetaEntry
{
    EMetaKind kind;
    std::string key;   // mainly for Custom
    std::string value; // encoded as text ("0.0,100.0", etc.)
};

struct FPropertyMetadata
{
    std::vector<FMetaEntry> entries;
};

inline FPropertyMetadata REMakeMeta(std::initializer_list<FMetaEntry> list)
{
    FPropertyMetadata m;
    m.entries.insert(m.entries.end(), list.begin(), list.end());
    return m;
}

// Meta constructors

inline FMetaEntry REMetaCategory(const char* name)
{
    return { EMetaKind::Category, {}, name };
}

inline FMetaEntry REMetaTooltip(const char* tip)
{
    return { EMetaKind::Tooltip, {}, tip };
}

inline FMetaEntry REMetaRange(double minVal, double maxVal)
{
    return { EMetaKind::Range, {}, std::to_string(minVal) + "," + std::to_string(maxVal) };
}

inline FMetaEntry REMetaVisibleToScript()
{
    return { EMetaKind::VisibleToScript, {}, {} };
}

// Public-facing macro helpers
#define Category(name)        REMetaCategory(name)
#define Tooltip(tip)          REMetaTooltip(tip)
#define Range(minVal, maxVal) REMetaRange(minVal, maxVal)
#define VisibleToScript     REMetaVisibleToScript()

// Packing
#define J_META_PACK(...)  REMakeMeta({ __VA_ARGS__ })
#define J_META_NONE()     REMakeMeta({})