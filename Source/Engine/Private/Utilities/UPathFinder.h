// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <filesystem>

/**
 * @class UPathFinder
 * @brief Utility for handling file paths and directories in the engine.
 *
 * Provides cross-platform helpers for:
 * - Resolving paths relative to the project root
 * - Listing files and directories (recursive and filtered)
 * - Path manipulations
 */
class UPathFinder
{
private:
    /** Default marker folder for automatic project root detection */
    inline static const std::string DefaultMarkerFolder = "Assets";

    /** Default marker file for automatic project root detection */
    inline static const std::string DefaultMarkerFile = "JProject.config";

    static std::string GProjectRootFolderCached;
    static std::string GProjectRootFileCached;

public:
    // ----------------- Root Handling -----------------

    /**
     * @brief Automatically get the project root by searching for a marker folder from the current path.
     * Default marker folder is "Assets".
     *
     * @return Absolute path to the project root.
     */
    static std::string GetProjectRootByFolder();

    /**
     * @brief Automatically get the project root by searching for a marker file from the current path.
     * Default marker file is "JProject.config".
     *
     * @return Absolute path to the project root.
     */
    static std::string GetProjectRootByFile();

    /**
     * @brief Find the project root by scanning upward from a custom start path using a folder marker.
     *
     * @param startPath Directory to start scanning from.
     * @param markerFolder Folder that indicates the project root.
     * @return Absolute path to the project root; fallback to startPath if not found.
     */
    static std::string FindProjectRootByFolder(const std::string& startPath, const std::string& markerFolder);

    /**
     * @brief Find the project root by scanning upward from a custom start path using a file marker.
     *
     * @param startPath Directory to start scanning from.
     * @param markerFile File that indicates the project root.
     * @return Absolute path to the project root; fallback to startPath if not found.
     */
    static std::string FindProjectRootByFile(const std::string& startPath, const std::string& markerFile);

    // ----------------- Path Queries -----------------

    /**
     * @brief Check if a file exists (relative to project root).
     *
     * @param path Relative or absolute path to the file.
     * @return true if the file exists, false otherwise.
     */
    static bool FileExists(const std::string& path);

    /**
     * @brief Check if a directory exists (relative to project root).
     *
     * @param path Relative or absolute path to the directory.
     * @return true if the directory exists, false otherwise.
     */
    static bool DirectoryExists(const std::string& path);

    /**
     * @brief Get all files in a directory, optionally filtered by extension.
     *
     * @param directory Relative or absolute path to directory.
     * @param extension Optional extension filter (without dot, e.g., "png"). Empty means all files.
     * @param bRecursive If true, search subdirectories recursively.
     * @param bCaseInsensitive If true, match extensions case-insensitively.
     * @return Vector of file paths (absolute) matching the criteria.
     */
    static std::vector<std::string> ListFiles(
        const std::string& directory,
        const std::string& extension = "",
        bool bRecursive = false,
        bool bCaseInsensitive = false
    );

    /**
     * @brief Get all directories inside a directory.
     *
     * @param directory Relative or absolute path to directory.
     * @param bRecursive If true, list directories recursively.
     * @return Vector of directory paths (absolute).
     */
    static std::vector<std::string> ListDirectories(
        const std::string& directory,
        bool bRecursive = false
    );

    // ----------------- Path Manipulation -----------------

    /**
     * @brief Join multiple path segments into a single normalized path (relative to project root).
     *
     * @tparam Args Variadic list of path segments.
     * @param args Path segments to join.
     * @return Normalized absolute path.
     */
    template<typename... Args>
    static std::string Join(const Args&... args)
    {
        std::filesystem::path p;
        (p.append(args), ...);
        return Normalize(p.string());
    }

    /**
     * @brief Normalize slashes, resolve "." and "..", and convert to absolute path.
     *
     * @param path Relative or absolute path.
     * @return Normalized absolute path.
     */
    static std::string Normalize(const std::string& path);

    /**
     * @brief Get the parent directory of a path.
     *
     * @param path Relative or absolute path.
     * @return Absolute path to the parent directory.
     */
    static std::string GetParent(const std::string& path);

    /**
     * @brief Get the filename from a path (optionally including the extension).
     *
     * @param path Relative or absolute path.
     * @param bIncludeExtension If true, include extension in the result.
     * @return Filename string.
     */
    static std::string GetFileName(const std::string& path, bool bIncludeExtension = true);

    /**
     * @brief Get the file extension (without the dot) from a path.
     *
     * @param path Relative or absolute path.
     * @return File extension string.
     */
    static std::string GetExtension(const std::string& path);

private:
    /** Resolve a path relative to the project root (if not absolute). */
    static std::filesystem::path ResolvePath(const std::string& path);

    /** Find project root by scanning upward for a marker folder (like "Assets"). */
    static std::string FindProjectRoot(const std::string& startPath, const std::string& markerFolder);
};