// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/EngineInstallResolver.h"

#include <algorithm>

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    static std::string ToLowerCopy(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }
}

bool EngineInstallResolver::Resolve(const std::string& inputPath, FResolvedEngineInstall& outInstall) const
{
    outInstall = {};

    if (inputPath.empty())
        return false;

    const std::string normalized = UPath::NormalizeVirtual(inputPath);

    if (UFileSystem::FileExists(normalized))
    {
        if (!IsValidEngineExecutable(normalized))
            return false;

        std::string root;
        if (!TryGetRootFromExecutable(normalized, root))
            return false;

        outInstall.engineRootPath = root;
        outInstall.executablePath = normalized;
        return true;
    }

    if (UFileSystem::DirectoryExists(normalized))
    {
        if (!IsValidEngineRoot(normalized))
            return false;

        std::string exe;
        if (!FindExecutableUnderRoot(normalized, exe))
            return false;

        outInstall.engineRootPath = normalized;
        outInstall.executablePath = exe;
        return true;
    }

    return false;
}

bool EngineInstallResolver::IsValidEngineRoot(const std::string& engineRootPath) const
{
    if (!UFileSystem::DirectoryExists(engineRootPath))
        return false;

    std::string exe;
    return FindExecutableUnderRoot(engineRootPath, exe);
}

bool EngineInstallResolver::IsValidEngineExecutable(const std::string& executablePath) const
{
    if (!UFileSystem::FileExists(executablePath))
        return false;

    const std::string fileName = ToLowerCopy(UPath::GetFileName(executablePath, true));
    return LooksLikeExecutableName(fileName);
}

bool EngineInstallResolver::FindExecutableUnderRoot(const std::string& engineRootPath, std::string& outExecutablePath) const
{
    outExecutablePath.clear();

    // Check common candidates first
#ifdef _WIN32
    const std::string editorCandidate = UPath::Join(engineRootPath, "Binaries", "JEditor.exe");
    const std::string gameCandidate   = UPath::Join(engineRootPath, "Binaries", "JGame.exe");
#else
    const std::string editorCandidate = UPath::Join(engineRootPath, "Binaries", "JEditor");
    const std::string gameCandidate   = UPath::Join(engineRootPath, "Binaries", "JGame");
#endif

    if (UFileSystem::FileExists(editorCandidate))
    {
        outExecutablePath = UPath::NormalizeVirtual(editorCandidate);
        return true;
    }

    if (UFileSystem::FileExists(gameCandidate))
    {
        outExecutablePath = UPath::NormalizeVirtual(gameCandidate);
        return true;
    }

    // Fallback: scan under Binaries
    const std::string binariesDir = UPath::Join(engineRootPath, "Binaries");
    if (!UFileSystem::DirectoryExists(binariesDir))
        return false;

    for (const std::string& file : UFileSystem::ListFiles(binariesDir, "", true, true))
    {
        const std::string fileName = ToLowerCopy(UPath::GetFileName(file, true));
        if (LooksLikeExecutableName(fileName))
        {
            outExecutablePath = UPath::NormalizeVirtual(file);
            return true;
        }
    }

    return false;
}

bool EngineInstallResolver::TryGetRootFromExecutable(const std::string& executablePath, std::string& outEngineRootPath) const
{
    outEngineRootPath.clear();

    if (!IsValidEngineExecutable(executablePath))
        return false;

    const std::string binariesDir = UPath::GetParent(executablePath);
    const std::string rootDir     = UPath::GetParent(binariesDir);

    if (!UFileSystem::DirectoryExists(rootDir))
        return false;

    outEngineRootPath = UPath::NormalizeVirtual(rootDir);
    return true;
}

bool EngineInstallResolver::LooksLikeExecutableName(const std::string& fileName) const
{
#ifdef _WIN32
    return fileName == "jeditor.exe" || fileName == "jgame.exe";
#else
    return fileName == "jeditor" || fileName == "jgame";
#endif
}