// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UPath.h"

namespace
{
    static std::string ToGenericString(const std::filesystem::path& path)
    {
        return path.generic_string();
    }
}

// ----------------- Path Manipulation -----------------

std::string UPath::Normalize(const std::string& path)
{
    const std::filesystem::path p(path);
    return ToGenericString(p.lexically_normal());
}

std::string UPath::GetParent(const std::string& path)
{
    const std::filesystem::path p(path);
    return ToGenericString(p.parent_path().lexically_normal());
}

std::string UPath::GetFileName(const std::string& path, bool bIncludeExtension)
{
    const std::filesystem::path p(path);
    return bIncludeExtension ? p.filename().string() : p.stem().string();
}

std::string UPath::GetExtension(const std::string& path)
{
    std::string ext = std::filesystem::path(path).extension().string();
    if (!ext.empty() && ext[0] == '.')
        ext.erase(0, 1);
    return ext;
}

std::string UPath::RemoveExtension(const std::string &path)
{
    return std::filesystem::path(path).stem().string();
}

bool UPath::IsAbsolute(const std::string& path)
{
    return std::filesystem::path(path).is_absolute();
}
