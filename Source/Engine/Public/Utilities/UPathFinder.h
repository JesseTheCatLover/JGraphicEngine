// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <filesystem>

/**
 * @class UPathFinder
 * @brief Utility for handling file paths and directories in the engine.
 *
 * Provides cross-platform helpers for resolving paths, listing files,
 * checking existence, joining paths, and normalizing paths.
 */
class UPathFinder
{
public:
    // ----------------- Path Queries -----------------

    /** @brief Check if a file exists at the given path. */
    static bool FileExists(const std::string& path);

    /** @brief Check if a directory exists at the given path. */
    static bool DirectoryExists(const std::string& path);

    /** @brief Get all files in a directory, optionally recursive. */
    static std::vector<std::string> ListFiles(const std::string& directory, const std::string& extension = "",
    bool bRecursive = false, bool bCaseInsensitive = false);

    /** @brief Get all directories in a directory, optionally recursive. */
    static std::vector<std::string> ListDirectories(const std::string& directory, bool bRecursive = false);

    /** @brief Join multiple path segments into a single normalized path. */
    template<typename... Args>
    std::string Join(const Args&... args)
    {
        std::filesystem::path p;
        (p.append(args), ...);
        return Normalize(p.string());
    }

    /** @brief Normalize slashes and remove redundant components. */
    static std::string Normalize(const std::string& path);

    /** @brief Get the parent directory of a path. */
    static std::string GetParent(const std::string& path);

    /** @brief Get the filename from a path (with or without extension). */
    static std::string GetFileName(const std::string& path, bool bIncludeExtension = true);

    /** @brief Get the file extension (without dot). */
    static std::string GetExtension(const std::string& path);
};
