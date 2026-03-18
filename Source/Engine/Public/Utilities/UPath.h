// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <filesystem>

/**
 * @class UPath
 * @brief Utility for path manipulation in the engine.
 *
 * UPath is intended to provide lightweight, cross-platform helpers for:
 * - Joining path segments
 * - Lexically normalizing paths
 * - Querying parent/name/extension information
 *
 * Important:
 * - UPath should be treated primarily as a path utility, not as a source of truth
 *   for project or engine roots.
 * - ProjectContext and VirtualPathMounter are now the authoritative owners of
 *   engine/project layout and virtual asset path resolution.
 */
class UPath
{
public:
    // ----------------- Path Manipulation -----------------

    /**
     * @brief Joins multiple path segments into a single lexically normalized path.
     *
     * Concatenates all provided path segments using std::filesystem::path semantics,
     * then performs lexical normalization. This function does not resolve relative paths
     * against any project or engine root.
     *
     * @tparam Args Variadic list of path segment types convertible to std::filesystem::path.
     * @param args Path segments to join (e.g. folder names, filenames).
     * @return Joined and lexically normalized path as a string using forward slashes.
     */
    template<typename... Args>
    static std::string Join(const Args&... args)
    {
        std::filesystem::path p;
        ((p /= std::filesystem::path(args)), ...);
        return Normalize(p.string());
    }

    /**
     * @brief Lexically normalize a path.
     *
     * Normalizes slashes and resolves "." and ".." lexically without consulting
     * project-root state or requiring the path to exist on disk.
     *
     * @param path Relative or absolute path.
     * @return Lexically normalized path string using forward slashes.
     */
    static std::string Normalize(const std::string& path);

    /**
     * @brief Get the parent directory of a path.
     *
     * This is a pure path operation and does not resolve the path relative to any root.
     *
     * @param path Relative or absolute path.
     * @return Parent path as a lexically normalized string using forward slashes.
     */
    static std::string GetParent(const std::string& path);

    /**
     * @brief Get the filename from a path (optionally including the extension).
     *
     * This is a pure path operation and does not resolve the path relative to any root.
     *
     * @param path Relative or absolute path.
     * @param bIncludeExtension If true, include extension in the result.
     * @return Filename string.
     */
    static std::string GetFileName(const std::string& path, bool bIncludeExtension = true);

    /**
     * @brief Get the file extension (without the dot) from a path.
     *
     * This is a pure path operation and does not resolve the path relative to any root.
     *
     * @param path Relative or absolute path.
     * @return File extension string.
     */
    static std::string GetExtension(const std::string& path);

    /**
     * @brief Remove the file extension from a path.
     *
     * Returns the input path with its final file extension removed. The directory
     * portion of the path is preserved and only the filename extension (the portion
     * after the last '.') is stripped.
     *
     * This is a pure path operation and does not resolve the path relative to any
     * project or engine root.
     *
     * Examples:
     * - "/Project/Textures/icon.png" → "/Project/Textures/icon"
     * - "file.txt" → "file"
     *
     * @param path Relative or absolute path.
     * @return Path string with the file extension removed.
     */
    static std::string RemoveExtension(const std::string &path);

    /**
     * @brief Check whether a path is absolute.
     *
     * @param path Relative or absolute path.
     * @return true if the path is absolute, false otherwise.
     */
    static bool IsAbsolute(const std::string& path);
};