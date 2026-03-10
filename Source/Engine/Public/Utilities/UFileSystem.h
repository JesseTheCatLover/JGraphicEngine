// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <filesystem>

/**
 * @class UFileSystem
 * @brief Utility for file I/O in the engine.
 *
 * Provides cross-platform helpers for:
 * - Reading and writing text and binary files
 * - Creating, deleting, and moving files/directories
 * - High-level wrappers around std::filesystem with engine-specific safety
 */
class UFileSystem
{
public:
    // ----------------- Read -----------------

    /**
     * @brief Read the entire file contents as a string.
     *
     * @param path Relative or absolute path to the file.
     * @return File contents as string, or std::nullopt if failed.
     */
    static std::optional<std::string> ReadTextFile(const std::string& path);

    /**
     * @brief Read the entire file contents as raw bytes.
     *
     * @param path Relative or absolute path to the file.
     * @return File contents as vector of bytes, or empty optional if failed.
     */
    static std::optional<std::vector<uint8_t>> ReadBinaryFile(const std::string& path);

    // ----------------- Write -----------------

    /**
     * @brief Write a string to a file (overwrite by default).
     *
     * @param path Path to file.
     * @param data Text to write.
     * @param bAppend If true, append to file instead of overwriting.
     * @return True if successful, false otherwise.
     */
    static bool WriteTextFile(const std::string& path, const std::string& data, bool bAppend = false);

    /**
     * @brief Write raw bytes to a file (overwrite by default).
     *
     * @param path Path to file.
     * @param data Bytes to write.
     * @param bAppend If true, append to file instead of overwriting.
     * @return True if successful, false otherwise.
     */
    static bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data, bool bAppend = false);

    // ----------------- File Ops -----------------

    /**
     * @brief Delete a file.
     *
     * @param path Path to file.
     * @return True if successful, false if file didn’t exist or couldn’t be removed.
     */
    static bool DeleteFile(const std::string& path);

    /**
     * @brief Move (or rename) a file.
     *
     * @param source Source path.
     * @param destination Destination path.
     * @return True if successful, false otherwise.
     */
    static bool MoveFile(const std::string& source, const std::string& destination);

    /**
     * @brief Rename a file (alias for MoveFile).
     *
     * @param source Source file path.
     * @param newName New file path or filename.
    * @return True if successful, false otherwise.
    */
    static bool RenameFile(const std::string& source, const std::string& newName);

    // ----------------- Directory Ops -----------------

    /**
     * @brief Create a directory (including parents).
     *
     * @param path Path to directory.
     * @return True if successful, false otherwise.
     */
    static bool CreateDirectory(const std::string& path);

    /**
     * @brief Delete a directory (optionally recursive).
     *
     * @param path Path to directory.
     * @param bRecursive If true, remove all contents recursively.
     * @return True if successful, false otherwise.
     */
    static bool DeleteDirectory(const std::string& path, bool bRecursive = false);

    static std::filesystem::path GetExecutablePath();

    // ----------------- Info & Listing -----------------

    /**
     * @brief Check if a file exists.
     *
     * @param path Path to the file (relative or absolute).
     * @return True if file exists, false otherwise.
     */
    static bool FileExists(const std::string& path);

    /**
     * @brief Check if a directory exists.
     *
     * @param path Path to the directory (relative or absolute).
     * @return True if directory exists, false otherwise.
     */
    static bool DirectoryExists(const std::string& path);

    /**
     * @brief List all files in a directory, optionally filtered by extension.
     *
     * @param directory Directory to search in.
     * @param extension Optional file extension filter (without dot).
     * @param bRecursive If true, list files recursively.
     * @param bCaseInsensitive If true, match extension case-insensitively.
     * @return Vector of absolute file paths.
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
     * @param directory Directory to search in.
     * @param bRecursive If true, list recursively.
     * @return Vector of absolute directory paths.
     */
    static std::vector<std::string> ListDirectories(
        const std::string& directory,
        bool bRecursive = false
    );
};
