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
