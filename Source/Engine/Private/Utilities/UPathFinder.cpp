//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UPathFinder.h"
#include <algorithm>

// ----------------- Automatic Getter -----------------
std::string UPathFinder::GetProjectRootByFolder()
{
    if (!GProjectRootFolderCached.empty())
        return GProjectRootFolderCached;

    GProjectRootFolderCached = FindProjectRootByFolder(
        std::filesystem::current_path().string(), DefaultMarkerFolder);
    return GProjectRootFolderCached;
}

std::string UPathFinder::GetProjectRootByFile()
{
    if (!GProjectRootFileCached.empty())
        return GProjectRootFileCached;

    GProjectRootFileCached = FindProjectRootByFile(
        std::filesystem::current_path().string(), DefaultMarkerFile);
    return GProjectRootFileCached;
}

// ----------------- Flexible Finder -----------------
std::string UPathFinder::FindProjectRootByFolder(const std::string& startPath, const std::string& markerFolder)
{
    std::filesystem::path path = startPath;

    while (!path.empty())
    {
        if (std::filesystem::exists(path / markerFolder) &&
            std::filesystem::is_directory(path / markerFolder))
        {
            return path.string();
        }
        path = path.parent_path();
    }

    return startPath;
}

std::string UPathFinder::FindProjectRootByFile(const std::string& startPath, const std::string& markerFile)
{
    std::filesystem::path path = startPath;

    while (!path.empty())
    {
        if (std::filesystem::exists(path / markerFile) &&
            std::filesystem::is_regular_file(path / markerFile))
        {
            return path.string();
        }
        path = path.parent_path();
    }

    return startPath;
}

bool UPathFinder::FileExists(const std::string& path)
{
    auto resolved = ResolvePath(path);
    return std::filesystem::exists(resolved) && std::filesystem::is_regular_file(resolved);
}

bool UPathFinder::DirectoryExists(const std::string& path)
{
    auto resolved = ResolvePath(path);
    return std::filesystem::exists(resolved) && std::filesystem::is_directory(resolved);
}

std::vector<std::string> UPathFinder::ListFiles(
    const std::string& directory,
    const std::string& extension,
    bool bRecursive,
    bool bCaseInsensitive)
{
    std::vector<std::string> result;
    std::filesystem::path dirPath = ResolvePath(directory);

    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
        return result;

    std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;

    auto matchesExtension = [&](const std::string& fileExt) -> bool
    {
        if (extension.empty())
            return true;

        std::string ext = fileExt;
        if (!ext.empty() && ext[0] == '.')
            ext.erase(0, 1);

        if (bCaseInsensitive)
        {
            std::string extLower = ext;
            std::string filterLower = extension;
            std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
            return extLower == filterLower;
        }

        return ext == extension;
    };

    try
    {
        if (bRecursive)
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath, options))
            {
                if (entry.is_regular_file() && matchesExtension(entry.path().extension().string()))
                    result.push_back(entry.path().string());
            }
        }
        else
        {
            for (const auto& entry : std::filesystem::directory_iterator(dirPath, options))
            {
                if (entry.is_regular_file() && matchesExtension(entry.path().extension().string()))
                    result.push_back(entry.path().string());
            }
        }
    }
    catch (...)
    {
        // Ignore exceptions silently
    }

    return result;
}

std::string UPathFinder::Normalize(const std::string& path)
{
    return std::filesystem::weakly_canonical(ResolvePath(path)).string();
}

std::string UPathFinder::GetParent(const std::string& path)
{
    return ResolvePath(path).empty() ? "" : std::filesystem::path(ResolvePath(path)).parent_path().string();
}

std::string UPathFinder::GetFileName(const std::string& path, bool bIncludeExtension)
{
    std::filesystem::path p(ResolvePath(path));
    return bIncludeExtension ? p.filename().string() : p.stem().string();
}

std::string UPathFinder::GetExtension(const std::string& path)
{
    std::string ext = std::filesystem::path(ResolvePath(path)).extension().string();
    if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);
    return ext;
}

std::filesystem::path UPathFinder::ResolvePath(const std::string& path)
{
    std::filesystem::path p(path);
    if (p.is_relative())
        p = std::filesystem::path(GetProjectRootByFolder()) / p;
    return p;
}

std::string UPathFinder::FindProjectRoot(const std::string& startPath, const std::string& markerFolder)
{
    std::filesystem::path path = startPath;
    while (!path.empty())
    {
        if (std::filesystem::exists(path / markerFolder))
            return path.string();
        path = path.parent_path();
    }
    return startPath;
}
