// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UProcess.h"

#include <sstream>

#ifdef JENGINE_PLATFORM_WINDOWS

    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>

#elif defined(JENGINE_PLATFORM_LINUX)

    #include <cstdlib>

#elif defined(JENGINE_PLATFORM_MACOS)

    #include <cstdlib>

#endif

// TODO:
// Implement proper Windows command-line escaping.
// This implementation is sufficient for simple arguments such as:
// --editor
// --force-launcher
// --project=C:/Project
std::string UProcess::BuildCommandLine(
    const std::filesystem::path &executable,
    const std::vector<std::string> &arguments)
{
    std::ostringstream ss;

    ss << '"' << executable.string() << '"';

    for (const auto &arg: arguments)
    {
        ss << ' ';

        if (arg.find(' ') != std::string::npos)
            ss << '"' << arg << '"';
        else
            ss << arg;
    }

    return ss.str();
}

bool UProcess::Launch(
    const std::filesystem::path& executable,
    const std::vector<std::string>& arguments,
    const FLaunchOptions& options)
{
#ifdef JENGINE_PLATFORM_WINDOWS

    std::string commandLine = BuildCommandLine(executable, arguments);

    // CreateProcess may modify the command-line buffer.
    std::vector<char> commandLineBuffer(commandLine.begin(), commandLine.end());
    commandLineBuffer.push_back('\0');

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    if (options.bHidden)
    {
        startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION processInfo{};

    DWORD creationFlags = 0;

    if (options.bDetached && !options.bWait)
        creationFlags |= DETACHED_PROCESS;

    if (options.bHidden)
        creationFlags |= CREATE_NO_WINDOW;

    std::string workingDirectoryString;

    LPCSTR workingDirectory = nullptr;

    if (!options.WorkingDirectory.empty())
    {
        workingDirectoryString = options.WorkingDirectory.string();
        workingDirectory = workingDirectoryString.c_str();
    }

    BOOL success = CreateProcessA(
        nullptr,
        commandLineBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        creationFlags,
        nullptr,
        workingDirectory,
        &startupInfo,
        &processInfo);

    if (!success)
        return false;

    if (options.bWait)
    {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    return true;

#else

    std::string command = BuildCommandLine(executable, arguments);

    if (options.bDetached)
    {
        command += " &";
    }

    return std::system(command.c_str()) == 0;

#endif
}