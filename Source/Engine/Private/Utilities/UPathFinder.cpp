//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UPathFinder.h"

bool UPathFinder::FileExists(const std::string& path)
{
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

bool UPathFinder::DirectoryExists(const std::string& path)
{
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

std::vector<std::string> UPathFinder::ListFiles(const std::string& directory, const std::string& extension,
    bool bRecursive, bool bCaseInsensitive)
{
    std::vector<std::string> files;
    if (!DirectoryExists(directory)) return files;

    std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;

    auto dirIter = bRecursive ?
        std::filesystem::recursive_directory_iterator(directory, options) :
        std::filesystem::directory_iterator(directory, options);

    auto toLower = [](std::string str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return str;
    };

    for (auto& entry : dirIter)
    {
        if (!entry.is_regular_file()) continue;

        std::string filePath = entry.path().string();
        if (extension.empty())
        {
            files.push_back(filePath);
            continue;
        }

        std::string fileExt = entry.path().extension().string();
        if (bCaseInsensitive)
        {
            if (toLower(fileExt) == toLower(extension))
                files.push_back(filePath);
        }
        else
        {
            if (fileExt == extension)
                files.push_back(filePath);
        }
    }
    return files;
}

std::vector<std::string> UPathFinder::ListDirectories(const std::string& directory, bool bRecursive)
{
    std::vector<std::string> dirs;
    if (!DirectoryExists(directory)) return dirs;

    std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;

    auto dirIter = bRecursive ?
        std::filesystem::recursive_directory_iterator(directory, options) :
        std::filesystem::directory_iterator(directory, options);

    for (auto& entry : dirIter)
    {
        if (entry.is_directory())
            dirs.push_back(entry.path().string());
    }
    return dirs;
}

std::string UPathFinder::Normalize(const std::string& path)
{
    std::filesystem::path p(path);
    return p.lexically_normal().string();
}

std::string UPathFinder::GetParent(const std::string& path)
{
    return std::filesystem::path(path).parent_path().string();
}

std::string UPathFinder::GetFileName(const std::string& path, bool bIncludeExtension)
{
    std::filesystem::path p(path);
    return bIncludeExtension ? p.filename().string() : p.stem().string();
}

std::string UPathFinder::GetExtension(const std::string& path)
{
    std::string ext = std::filesystem::path(path).extension().string();
    if (!ext.empty() && ext[0] == '.') ext.erase(0,1);
    return ext;
}
