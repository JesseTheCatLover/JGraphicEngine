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

std::string UPath::Normalize(std::string_view path)
{
    if (path.empty())
        return {};

    std::string cleaned;
    cleaned.reserve(path.size() + 1);

    // Force virtual absolute path
    if (path.front() != '/')
        cleaned.push_back('/');

    bool previousWasSlash = false;

    for (char c : path)
    {
        // Convert separators
        if (c == '\\')
            c = '/';

        // Collapse repeated slashes in one pass
        if (c == '/')
        {
            if (previousWasSlash)
                continue;

            previousWasSlash = true;
        }
        else
        {
            previousWasSlash = false;
        }

        cleaned.push_back(c);
    }

    // Lexical normalization
    cleaned = std::filesystem::path(cleaned)
        .lexically_normal()
        .generic_string();

    // Remove trailing slash except root
    if (cleaned.size() > 1 && cleaned.back() == '/')
        cleaned.pop_back();

    return cleaned;
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
