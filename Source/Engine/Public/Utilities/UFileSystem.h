// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <filesystem>

/**
 * @class UFileSystem
 * @brief Utility for filesystem I/O in the engine.
 *
 * Provides cross-platform helpers for:
 * - Reading and writing text and binary files
 * - Creating, deleting, moving, and renaming files/directories
 * - Querying existence and listing files/directories
 *
 * Important:
 * - UFileSystem operates on physical filesystem paths.
 * - Callers should resolve engine/project roots through ProjectContext and
 *   resolve asset-space paths through VirtualPathMounter before using this class.
 */
class UFileSystem
{
public:
    // ----------------- Read -----------------

    /**
     * @brief Read the entire file contents as a string.
     *
     * @param path Physical filesystem path to the file.
     * @return File contents as string, or std::nullopt if failed.
     */
    static std::optional<std::string> ReadTextFile(const std::string& path);

    /**
     * @brief Read the entire file contents as raw bytes.
     *
     * @param path Physical filesystem path to the file.
     * @return File contents as vector of bytes, or std::nullopt if failed.
     */
    static std::optional<std::vector<uint8_t>> ReadBinaryFile(const std::string& path);

    // ----------------- Write -----------------

    /**
     * @brief Write a string to a file (overwrite by default).
     *
     * Parent directories are created automatically if needed.
     *
     * @param path Physical filesystem path to the file.
     * @param data Text to write.
     * @param bAppend If true, append to file instead of overwriting.
     * @return True if successful, false otherwise.
     */
    static bool WriteTextFile(const std::string& path, const std::string& data, bool bAppend = false);

    /**
     * @brief Write raw bytes to a file (overwrite by default).
     *
     * Parent directories are created automatically if needed.
     *
     * @param path Physical filesystem path to the file.
     * @param data Bytes to write.
     * @param bAppend If true, append to file instead of overwriting.
     * @return True if successful, false otherwise.
     */
    static bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data, bool bAppend = false);

    // ----------------- File Ops -----------------

    /**
     * @brief Delete a file.
     *
     * @param path Physical filesystem path to the file.
     * @return True if successful, false if the file didn’t exist or couldn’t be removed.
     */
    static bool DeleteFile(const std::string& path);

    /**
     * @brief Move (or rename) a file.
     *
     * Parent directories for the destination are created automatically if needed.
     *
     * @param source Physical source file path.
     * @param destination Physical destination file path.
     * @return True if successful, false otherwise.
     */
    static bool MoveFile(const std::string& source, const std::string& destination);

    /**
     * @brief Rename a file (alias for MoveFile).
     *
     * @param source Physical source file path.
     * @param newName New physical file path or filename.
     * @return True if successful, false otherwise.
     */
    static bool RenameFile(const std::string& source, const std::string& newName);

    // ----------------- Directory Ops -----------------

    /**
     * @brief Create a directory (including parents).
     *
     * If the directory already exists, this function returns true.
     *
     * @param path Physical filesystem path to the directory.
     * @return True if successful, false otherwise.
     */
    static bool CreateDirectory(const std::string& path);

    /**
     * @brief Delete a directory (optionally recursive).
     *
     * @param path Physical filesystem path to the directory.
     * @param bRecursive If true, remove all contents recursively.
     * @return True if successful, false otherwise.
     */
    static bool DeleteDirectory(const std::string& path, bool bRecursive = false);

    /**
     * @brief Get the directory containing the currently running executable.
     *
     * @return Executable directory path, or empty path if unavailable.
     */
    static std::filesystem::path GetExecutablePath();

    // ----------------- Info & Listing -----------------

    /**
     * @brief Check if a file exists.
     *
     * @param path Physical filesystem path to the file.
     * @return True if the file exists, false otherwise.
     */
    static bool FileExists(const std::string& path);

    /**
     * @brief Check if a directory exists.
     *
     * @param path Physical filesystem path to the directory.
     * @return True if the directory exists, false otherwise.
     */
    static bool DirectoryExists(const std::string& path);

    /**
     * @brief Get the last modification time of a file.
     *
     * @param path Physical filesystem path to the file.
     * @return Timestamp in milliseconds since epoch, or std::nullopt if unavailable.
     */
    static std::optional<uint64_t> GetLastWriteTime(const std::string& path);

    /**
     * @brief List all files in a directory, optionally filtered by extension.
     *
     * @param directory Physical filesystem path to the directory to search in.
     * @param extension Optional file extension filter (without dot).
     * @param bRecursive If true, list files recursively.
     * @param bCaseInsensitive If true, match extensions case-insensitively.
     * @return Vector of lexically normalized file paths.
     */
    static std::vector<std::string> ListFiles(
        const std::string& directory,
        const std::string& extension = "",
        bool bRecursive = false,
        bool bCaseInsensitive = false
    );

    /**
     * @brief List all directories within a directory.
     *
     * @param directory Physical filesystem path to the directory to search in.
     * @param bRecursive If true, list recursively.
     * @return Vector of lexically normalized directory paths.
     */
    static std::vector<std::string> ListDirectories(
        const std::string& directory,
        bool bRecursive = false
    );
};