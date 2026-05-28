// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UPath.h"

namespace
{
    static std::string ToGenericString(const std::filesystem::path& path)
    {
        return path.generic_string();
    }

    static inline void ReplaceAll(std::string& s, const std::string& a, const std::string& b)
    {
        size_t pos = 0;
        while ((pos = s.find(a, pos)) != std::string::npos)
        {
            s.replace(pos, a.size(), b);
            pos += b.size();
        }
    }
}

// ----------------- Path Manipulation -----------------

std::string UPath::Normalize(const std::string& path)
{
    if (path.empty())
        return {};

    // Ensure generic separators first
    std::string s = path;
    std::replace(s.begin(), s.end(), '\\', '/');

    // If you want to force absolute-virtual paths:
    if (!s.empty() && s.front() != '/')
        s.insert(s.begin(), '/');

    // Collapse repeated slashes early
    while (s.find("//") != std::string::npos)
        ReplaceAll(s, "//", "/");

    // Lexical normalize dot segments using filesystem (ok if we keep it purely lexical)
    std::filesystem::path p(s);
    s = ToGenericString(p.lexically_normal());

    // Collapse slashes again (lexically_normal can reintroduce things in odd inputs)
    while (s.find("//") != std::string::npos)
        ReplaceAll(s, "//", "/");

    // Remove trailing slash except root
    while (s.size() > 1 && s.back() == '/')
        s.pop_back();

    return s;
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
    std::filesystem::path p(path);
    p.replace_extension();
    return p.string();
}

bool UPath::IsAbsolute(const std::string& path)
{
    return std::filesystem::path(path).is_absolute();
}
