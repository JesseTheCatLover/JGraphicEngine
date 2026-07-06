//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <filesystem>
#include <vector>
#include <string>

namespace UProcess
{
    struct FLaunchOptions
    {
        // Launch independently from the current process.
        bool bDetached = true;

        // Hide any console window (where supported).
        bool bHidden = false;

        // Wait until the launched process exits.
        bool bWait = false;

        // Optional working directory.
        std::filesystem::path WorkingDirectory;
    };

    // Builds a platform-appropriate command line string for logging/debugging.
    std::string BuildCommandLine(
        const std::filesystem::path& executable,
        const std::vector<std::string>& arguments);

    bool Launch(
        const std::filesystem::path& executable,
        const std::vector<std::string>& arguments = {},
        const FLaunchOptions& options = {});
}
