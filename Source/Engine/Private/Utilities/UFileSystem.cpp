// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UFileSystem.h"

#include "Utilities/UPath.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

#ifdef JENGINE_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <windows.h>

    #undef CreateDirectory
    #undef DeleteFile
    #undef MoveFile
#endif

#ifdef JENGINE_PLATFORM_MACOS
    #include <mach-o/dyld.h>
#endif

#ifdef JENGINE_PLATFORM_LINUX
    #include <unistd.h>
    #include <limits.h>
#endif

namespace fs = std::filesystem;

namespace
{
    static std::string NormalizePhysicalPath(const std::string& path)
    {
        return UPath::Normalize(path);
    }

    static std::string ToLowerCopy(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return s;
    }
}

// ----------------- Read -----------------

std::optional<std::string> UFileSystem::ReadTextFile(const std::string& path)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    std::ifstream file(fullPath, std::ios::in | std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return contents;
}

std::optional<std::vector<uint8_t>> UFileSystem::ReadBinaryFile(const std::string& path)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0)
        return std::nullopt;

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (size > 0 && !file.read(reinterpret_cast<char*>(buffer.data()), size))
        return std::nullopt;

    return buffer;
}

// ----------------- Write -----------------

bool UFileSystem::WriteTextFile(const std::string& path, const std::string& data, bool bAppend)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    const std::string parent = UPath::GetParent(fullPath);
    if (!parent.empty() && !CreateDirectory(parent))
        return false;

    std::ofstream file(fullPath, std::ios::out | std::ios::binary | (bAppend ? std::ios::app : std::ios::trunc));
    if (!file.is_open())
        return false;

    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    return file.good();
}

bool UFileSystem::WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data, bool bAppend)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    const std::string parent = UPath::GetParent(fullPath);
    if (!parent.empty() && !CreateDirectory(parent))
        return false;

    std::ofstream file(fullPath, std::ios::out | std::ios::binary | (bAppend ? std::ios::app : std::ios::trunc));
    if (!file.is_open())
        return false;

    if (!data.empty())
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

    return file.good();
}

// ----------------- File Ops -----------------

bool UFileSystem::DeleteFile(const std::string& path)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    std::error_code ec;
    return fs::remove(fullPath, ec);
}

bool UFileSystem::MoveFile(const std::string& source, const std::string& destination)
{
    const std::string fullSrc = NormalizePhysicalPath(source);
    const std::string fullDst = NormalizePhysicalPath(destination);

    const std::string parent = UPath::GetParent(fullDst);
    if (!parent.empty() && !CreateDirectory(parent))
        return false;

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
    const std::string fullPath = NormalizePhysicalPath(path);

    try
    {
        if (fs::exists(fullPath))
            return fs::is_directory(fullPath);

        std::error_code ec;
        return fs::create_directories(fullPath, ec) && !ec;
    }
    catch (...)
    {
        return false;
    }
}

bool UFileSystem::DeleteDirectory(const std::string& path, bool bRecursive)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    std::error_code ec;

    if (bRecursive)
        return fs::remove_all(fullPath, ec) > 0 && !ec;

    return fs::remove(fullPath, ec);
}

std::filesystem::path UFileSystem::GetExecutablePath()
{
#ifdef JENGINE_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH)
        return fs::path(buffer).parent_path();

    std::cerr << "Failed to get executable path (Windows)\n";
    return {};

#elif defined(JENGINE_PLATFORM_MACOS)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
        return fs::canonical(fs::path(buffer)).parent_path();

    std::cerr << "Buffer too small, required size: " << size << '\n';
    return {};

#elif defined(JENGINE_PLATFORM_LINUX)
    char buffer[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        return fs::canonical(fs::path(buffer)).parent_path();
    }

    std::cerr << "Failed to read /proc/self/exe\n";
    return {};

#else
    std::cerr << "Unsupported platform.\n";
    return {};
#endif
}

// ----------------- Info & Listing -----------------

bool UFileSystem::FileExists(const std::string& path)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    try
    {
        return fs::exists(fullPath) && fs::is_regular_file(fullPath);
    }
    catch (...)
    {
        return false;
    }
}

bool UFileSystem::DirectoryExists(const std::string& path)
{
    const std::string fullPath = NormalizePhysicalPath(path);

    try
    {
        return fs::exists(fullPath) && fs::is_directory(fullPath);
    }
    catch (...)
    {
        return false;
    }
}

std::optional<uint64_t> UFileSystem::GetLastWriteTime(const std::string& path)
{
    try
    {
        if (!std::filesystem::exists(path))
            return std::nullopt;

        auto time = std::filesystem::last_write_time(path);

        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            time - std::filesystem::file_time_type::clock::now()
            + std::chrono::system_clock::now());

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            sctp.time_since_epoch()).count();

        return static_cast<uint64_t>(ms);
    }
    catch (...)
    {
        return std::nullopt;
    }
}


std::vector<std::string> UFileSystem::ListFiles(
    const std::string& directory,
    const std::string& extension,
    bool bRecursive,
    bool bCaseInsensitive)
{
    std::vector<std::string> files;

    const std::string fullDirectory = NormalizePhysicalPath(directory);

    try
    {
        if (!fs::exists(fullDirectory) || !fs::is_directory(fullDirectory))
            return files;

        std::string extFilter = extension;
        if (bCaseInsensitive)
            extFilter = ToLowerCopy(extFilter);

        const auto matchesExtension = [&](const fs::path& p) -> bool
        {
            if (extension.empty())
                return true;

            std::string fileExt = p.extension().string();
            if (!fileExt.empty() && fileExt.front() == '.')
                fileExt.erase(fileExt.begin());

            if (bCaseInsensitive)
                fileExt = ToLowerCopy(fileExt);

            return fileExt == extFilter;
        };

        if (bRecursive)
        {
            for (const auto& entry : fs::recursive_directory_iterator(fullDirectory))
            {
                if (!entry.is_regular_file())
                    continue;

                if (!matchesExtension(entry.path()))
                    continue;

                files.push_back(NormalizePhysicalPath(entry.path().string()));
            }
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(fullDirectory))
            {
                if (!entry.is_regular_file())
                    continue;

                if (!matchesExtension(entry.path()))
                    continue;

                files.push_back(NormalizePhysicalPath(entry.path().string()));
            }
        }
    }
    catch (...)
    {
        // Ignore filesystem exceptions and return what was collected so far.
    }

    return files;
}

std::vector<std::string> UFileSystem::ListDirectories(const std::string& directory, bool bRecursive)
{
    std::vector<std::string> dirs;

    const std::string fullDirectory = NormalizePhysicalPath(directory);

    try
    {
        if (!fs::exists(fullDirectory) || !fs::is_directory(fullDirectory))
            return dirs;

        if (bRecursive)
        {
            for (const auto& entry : fs::recursive_directory_iterator(fullDirectory))
            {
                if (entry.is_directory())
                    dirs.push_back(NormalizePhysicalPath(entry.path().string()));
            }
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(fullDirectory))
            {
                if (entry.is_directory())
                    dirs.push_back(NormalizePhysicalPath(entry.path().string()));
            }
        }
    }
    catch (...)
    {
        // Ignore filesystem exceptions and return what was collected so far.
    }

    return dirs;
}