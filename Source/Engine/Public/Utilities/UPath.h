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
        return NormalizeVirtual(p.string());
    }

    /**
     * @brief Lexically normalizes a virtual engine path.
     *
     * Virtual paths are engine-defined paths such as:
     *
     *     /Project/Textures/Diffuse.jasset
     *     /Engine/Materials/Default.jasset
     *
     * The function:
     *
     * - Converts all separators to forward slashes ('/')
     * - Forces the path to begin with '/'
     * - Collapses repeated slashes
     * - Resolves "." and ".." segments lexically
     * - Removes trailing slashes except for the virtual root ('/')
     *
     * The returned path is always in canonical virtual-path form and should
     * be used for asset references, mount points, registry paths, and other
     * engine-facing path systems.
     *
     * This function must NOT be used for operating-system filesystem paths
     * such as:
     *
     *     C:\Projects\MyGame
     *     /home/user/MyGame
     *
     * Examples:
     *
     *     "Textures\\UI//Button.png"
     *         -> "/Textures/UI/Button.png"
     *
     *     "/Project/Textures/../Materials/Default.jasset"
     *         -> "/Project/Materials/Default.jasset"
     *
     * @param path Virtual path to normalize.
     * @return Canonical virtual path using forward slashes.
     */
    static std::string NormalizeVirtual(std::string_view path);

    /**
     * @brief Lexically normalizes a physical filesystem path.
     *
     * Physical paths are operating-system paths such as:
     *
     *     - C:\Projects\MyGame
     *     - D:\RedleafEngine
     *     - /home/user/MyGame
     *
     * The function performs lexical normalization and preserves the path's
     * filesystem semantics.
     *
     * The function:
     *
     * - Resolves "." and ".." segments lexically
     * - Removes redundant separators where possible
     * - Preserves drive letters, root names, and absolute/relative state
     * - Does NOT prepend a virtual root slash
     * - Does NOT convert a filesystem path into a virtual engine path
     *
     * This function should be used whenever working with files, directories,
     * project locations, engine installations, or any path passed to the
     * operating system.
     *
     * Examples:
     *
     *     "C:\\Projects\\..\\MyGame\\Assets"
     *         -> "C:\\MyGame\\Assets"
     *
     *     "./Projects/MyGame/../Config"
     *         -> "Projects/Config"
     *
     * @param path Physical filesystem path.
     * @return Lexically normalized filesystem path.
     */
    static std::string NormalizePhysical(std::string_view path);

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
     * - "/Project/Textures/icon.png" -> "/Project/Textures/icon"
     * - "file.txt" -> "file"
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

    // ----------------- Name Validation -----------------

    /**
     * @brief Check whether a filesystem/asset name is valid.
     *
     * Validation is platform-safe and intended for engine asset/folder naming.
     *
     * Rules:
     * - Rejects empty names
     * - Rejects path separators
     * - Rejects reserved filesystem characters
     * - Rejects control characters
     * - Rejects trailing spaces/dots
     * - Rejects reserved Windows device names
     *
     * This validates a SINGLE path component only,
     * not an entire path.
     *
     * Examples:
     * - "Textures"      -> valid
     * - "My Folder"     -> valid
     * - "CON"           -> invalid
     * - "File?.png"     -> invalid
     * - "Folder/Sub"    -> invalid
     *
     * @param name Single filename/folder name.
     * @return True if valid.
     */
    static bool IsValidFileSystemName(std::string_view name);

    /**
     * @brief Check whether a filesystem name is reserved.
     *
     * Reserved names include Windows device names such as:
     * CON, PRN, AUX, NUL, COM1..COM9, LPT1..LPT9
     *
     * Comparison is case-insensitive.
     *
     * @param name Single filename/folder name.
     * @return True if reserved.
     */
    static bool IsReservedFileSystemName(std::string_view name);

    /**
     * @brief Sanitize a filesystem name into a safe engine-compatible form.
     *
     * Invalid filesystem characters are removed or replaced,
     * trailing spaces/dots are trimmed,
     * and empty results fall back to a default safe name.
     *
     * This operates on a SINGLE path component only,
     * not an entire path.
     *
     * Examples:
     * - "My:Folder?" -> "MyFolder"
     * - "  Test. "   -> "Test"
     *
     * @param name Input filename/folder name.
     * @return Sanitized filesystem-safe name.
     */
    static std::string SanitizeFileSystemName(std::string_view name);

    /**
     * @brief Check whether a path is identical to or contained within another path.
     *
     * Performs a lexical hierarchy check after normalizing both paths.
     *
     * Returns true when:
     * - path == ancestor
     * - path is located somewhere under ancestor
     *
     * Returns false when:
     * - path is outside ancestor
     * - the match is only a string prefix
     *
     * Examples:
     * - "/Project" + "/Project"                     -> true
     * - "/Project" + "/Project/Textures"            -> true
     * - "/Project" + "/Project/Textures/UI"         -> true
     * - "/Project" + "/ProjectX"                    -> false
     * - "/Project/Textures" + "/Project"            -> false
     *
     * The comparison is path-aware and enforces directory boundaries,
     * preventing false positives caused by simple string prefix matching.
     *
     * Both paths are normalized internally before comparison.
     *
     * @param ancestor The candidate ancestor folder path.
     * @param path The path being tested.
     * @return True if path is the same as or located under ancestor.
     */
    static bool IsSameOrUnder(const std::string& ancestor, const std::string& path);
};