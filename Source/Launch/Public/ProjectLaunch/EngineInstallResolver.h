// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

struct FResolvedEngineInstall
{
    std::string engineRootPath;
    std::string executablePath;
};

class EngineInstallResolver
{
public:
    EngineInstallResolver() = default;
    ~EngineInstallResolver() = default;

    /**
     * @brief Resolves an engine install from either an engine root path
     *        or a direct executable path.
     *
     * @param inputPath Engine root directory path or executable path.
     * @param outInstall Resolved engine install info.
     * @return True if resolution succeeded.
     */
    bool Resolve(const std::string& inputPath, FResolvedEngineInstall& outInstall) const;

    /**
     * @brief Checks whether a directory is a valid engine root.
     */
    bool IsValidEngineRoot(const std::string& engineRootPath) const;

    /**
     * @brief Checks whether a file path looks like a valid engine executable.
     */
    bool IsValidEngineExecutable(const std::string& executablePath) const;

    /**
     * @brief Attempts to find the engine executable under a valid engine root.
     */
    bool FindExecutableUnderRoot(const std::string& engineRootPath, std::string& outExecutablePath) const;

    /**
     * @brief Gets the engine root from a direct executable path.
     */
    bool TryGetRootFromExecutable(const std::string& executablePath, std::string& outEngineRootPath) const;

private:
    bool LooksLikeExecutableName(const std::string& fileName) const;
};