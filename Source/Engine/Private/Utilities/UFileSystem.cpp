// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UFileSystem.h"
#include "Utilities/UPathFinder.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// ----------------- Read -----------------

std::optional<std::string> UFileSystem::ReadTextFile(const std::string& path)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::ifstream file(fullPath, std::ios::in);
    if (!file.is_open())
        return std::nullopt;

    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return contents;
}

std::optional<std::vector<uint8_t>> UFileSystem::ReadBinaryFile(const std::string& path)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (size > 0 && !file.read(reinterpret_cast<char*>(buffer.data()), size))
        return std::nullopt;

    return buffer;
}

// ----------------- Write -----------------

bool UFileSystem::WriteTextFile(const std::string& path, const std::string& data, bool bAppend)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::ofstream file(fullPath, bAppend ? std::ios::app : std::ios::trunc);
    if (!file.is_open())
        return false;

    file << data;
    return true;
}

bool UFileSystem::WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data, bool bAppend)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::ofstream file(fullPath, std::ios::binary | (bAppend ? std::ios::app : std::ios::trunc));
    if (!file.is_open())
        return false;

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

// ----------------- File Ops -----------------

bool UFileSystem::DeleteFile(const std::string& path)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::error_code ec;
    return fs::remove(fullPath, ec);
}

bool UFileSystem::MoveFile(const std::string& source, const std::string& destination)
{
    std::string fullSrc = UPathFinder::Normalize(source);
    std::string fullDst = UPathFinder::Normalize(destination);

    std::error_code ec;
    fs::rename(fullSrc, fullDst, ec);
    return !ec;
}

bool UFileSystem::RenameFile(const std::string& source, const std::string& newName)
{
    return MoveFile(source, newName);
}

// ----------------- Directory Ops -----------------

bool UFileSystem::CreateDirectory(const std::string& path)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::error_code ec;
    return fs::create_directories(fullPath, ec);
}

bool UFileSystem::DeleteDirectory(const std::string& path, bool bRecursive)
{
    std::string fullPath = UPathFinder::Normalize(path);
    std::error_code ec;

    if (bRecursive)
        return fs::remove_all(fullPath, ec) > 0 && !ec;
    else
        return fs::remove(fullPath, ec);
}

bool UFileSystem::FileExists(const std::string &path)
{
    try
    {
        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    }
    catch (...)
    {
        return false;
    }
}

bool UFileSystem::DirectoryExists(const std::string &path)
{
    try
    {
        return std::filesystem::exists(path) && std::filesystem::is_directory(path);
    }
    catch (...)
    {
        return false;
    }
}

std::vector<std::string> UFileSystem::ListFiles(
    const std::string &directory,
    const std::string &extension,
    bool bRecursive,
    bool bCaseInsensitive)
{
    std::vector<std::string> files;

    try
    {
        if (!std::filesystem::exists(directory))
            return files;

        auto toLower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        };

        std::string extFilter = extension;
        if (bCaseInsensitive)
            extFilter = toLower(extFilter);

        auto iterType = bRecursive
                            ? std::filesystem::recursive_directory_iterator(directory)
                            : std::filesystem::directory_iterator(directory);

        for (const auto &entry : iterType)
        {
            if (!entry.is_regular_file())
                continue;

            std::string filePath = entry.path().string();

            if (!extension.empty())
            {
                std::string fileExt = entry.path().extension().string();
                if (!fileExt.empty() && fileExt.front() == '.')
                    fileExt.erase(fileExt.begin()); // remove leading '.'

                if (bCaseInsensitive)
                    fileExt = toLower(fileExt);

                if (fileExt != extFilter)
                    continue;
            }

            files.push_back(filePath);
        }
    }
    catch (...)
    {
        // Fail silently, just return empty
    }

    return files;
}

std::vector<std::string> UFileSystem::ListDirectories(const std::string &directory, bool bRecursive)
{
    std::vector<std::string> dirs;

    try
    {
        if (!std::filesystem::exists(directory))
            return dirs;

        auto iterType = bRecursive
                            ? std::filesystem::recursive_directory_iterator(directory)
                            : std::filesystem::directory_iterator(directory);

        for (const auto &entry : iterType)
        {
            if (entry.is_directory())
                dirs.push_back(entry.path().string());
        }
    }
    catch (...)
    {
        // ignore
    }

    return dirs;
}

