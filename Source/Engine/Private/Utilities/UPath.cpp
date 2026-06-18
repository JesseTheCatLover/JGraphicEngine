// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UPath.h"

#include <iostream>

namespace
{
    static std::string ToGenericString(const std::filesystem::path& path)
    {
        return path.generic_string();
    }
}

// ----------------- Path Manipulation -----------------

std::string UPath::NormalizeVirtual(std::string_view path)
{
    if (path.empty())
        return {};

    std::string cleaned;
    cleaned.reserve(path.size() + 1);

    // Only treat as virtual path if it is NOT a Windows absolute path
    if (!IsPhysicalPath(path) && path.front() != '/')
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

std::string UPath::NormalizePhysical(std::string_view path)
{
    if (path.empty())
        return {};

    std::filesystem::path p(path);

    p = p.lexically_normal();

    std::string result = p.string();

#ifdef JENGINE_PLATFORM_WINDOWS
    for (char& c : result)
        if (c == '/')
            c = '\\';
#endif

    std::cerr << "result: " << result << std::endl;
    return result;
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

bool UPath::IsValidFileSystemName(std::string_view name)
{
    if (name.empty())
        return false;

    // No leading/trailing spaces
    if (name.front() == ' ' || name.back() == ' ')
        return false;

    // No trailing periods
    if (name.back() == '.')
        return false;

    // Single component only
    for (char c : name)
    {
        const unsigned char uc = static_cast<unsigned char>(c);

        // Control chars
        if (uc < 32)
            return false;

        switch (c)
        {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            return false;

        default:
            break;
        }
    }

    if (IsReservedFileSystemName(name))
        return false;

    return true;
}

bool UPath::IsReservedFileSystemName(std::string_view name)
{
    if (name.empty())
        return false;

    // Windows treats "CON.txt" as reserved too,
    // so compare stem only.
    std::string stem =
        std::filesystem::path(std::string(name))
            .stem()
            .string();

    std::transform(stem.begin(), stem.end(), stem.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::toupper(c));
        });

    static constexpr std::string_view reserved[] =
    {
        "CON",
        "PRN",
        "AUX",
        "NUL",

        "COM1",
        "COM2",
        "COM3",
        "COM4",
        "COM5",
        "COM6",
        "COM7",
        "COM8",
        "COM9",

        "LPT1",
        "LPT2",
        "LPT3",
        "LPT4",
        "LPT5",
        "LPT6",
        "LPT7",
        "LPT8",
        "LPT9"
    };

    for (std::string_view r : reserved)
    {
        if (stem == r)
            return true;
    }

    return false;
}

std::string UPath::SanitizeFileSystemName(std::string_view name)
{
    std::string result;
    result.reserve(name.size());

    bool previousWasReplacement = false;

    for (char c : name)
    {
        const unsigned char uc = static_cast<unsigned char>(c);

        // Skip control chars
        if (uc < 32)
            continue;

        bool invalid = false;

        switch (c)
        {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            invalid = true;
            break;

        default:
            break;
        }

        if (invalid)
        {
            // Replace with single underscore
            if (!previousWasReplacement)
            {
                result.push_back('_');
                previousWasReplacement = true;
            }

            continue;
        }

        previousWasReplacement = false;
        result.push_back(c);
    }

    // Trim leading spaces
    while (!result.empty() && result.front() == ' ')
        result.erase(result.begin());

    // Trim trailing spaces/dots
    while (!result.empty() &&
          (result.back() == ' ' || result.back() == '.'))
    {
        result.pop_back();
    }

    // Reserved names
    if (IsReservedFileSystemName(result))
        result.push_back('_');

    // Empty fallback
    if (result.empty())
        result = "Unnamed";

    return result;
}

bool UPath::IsSameOrUnder(const std::string& ancestor, const std::string& path)
{
    const std::string a = NormalizeVirtual(ancestor);
    const std::string p = NormalizeVirtual(path);

    if (a.empty() || p.empty())
        return false;

    if (a == "/")
        return true;

    if (a == p)
        return true;

    if (p.size() <= a.size())
        return false;

    if (p.compare(0, a.size(), a) != 0)
        return false;

    return p[a.size()] == '/';
}

bool UPath::IsPhysicalPath(std::string_view p)
{
#ifdef JENGINE_PLATFORM_WINDOWS
    return p.size() > 1 && std::isalpha(p[0]) && p[1] == ':';
#else
    return !p.empty() && p.front() == '/';
#endif
}
