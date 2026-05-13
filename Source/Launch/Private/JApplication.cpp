//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "JApplication.h"

#include <iostream>
#include <algorithm>
#include <string>

#include "Core/JEngine.h"
#include "EditorApp.h"
#include "Core/Project/FProjectOpenRequest.h"
#include "ProjectLaunch/ProjectLaunchResolver.h"
#include "ProjectLaunch/ConsoleProjectLaunchUI.h"
#include "ProjectLaunch/LaunchSettings.h"
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

    static bool TryGetArgValue(int argc, char** argv, const std::string& key, std::string& outValue)
    {
        outValue.clear();

        const std::string loweredKey = ToLowerCopy(key);

        for (int i = 0; i < argc; ++i)
        {
            std::string arg = argv[i];
            const std::string loweredArg = ToLowerCopy(arg);

            // --project=SomePath
            const std::string prefix = loweredKey + "=";
            if (loweredArg.rfind(prefix, 0) == 0)
            {
                outValue = arg.substr(prefix.size());
                return !outValue.empty();
            }

            // --project SomePath
            if (loweredArg == loweredKey && i + 1 < argc)
            {
                outValue = argv[i + 1];
                return !outValue.empty();
            }
        }

        return false;
    }

    static bool HasFlag(int argc, char** argv, const std::string& flag)
    {
        const std::string loweredFlag = ToLowerCopy(flag);
        for (int i = 0; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (ToLowerCopy(arg) == loweredFlag)
                return true;
        }
        return false;
    }

    static std::string GuessCurrentEngineRoot()
    {
        const std::filesystem::path exePath = UFileSystem::GetExecutablePath();

        // expected layout: <EngineRoot>/Binaries/<Executable>
        const std::string binariesDir = UPath::Normalize(exePath.string());
        const std::string engineRoot  = UPath::GetParent(binariesDir);

        return UPath::Normalize(engineRoot);
    }
}

bool JApplication::RunEditor(int argc, char** argv)
{
    std::cout << "[JApplication]: Launching in EDITOR mode...\n";
    EditorApp editor;
    return LaunchEngine(&editor, argc, argv);
}

bool JApplication::RunGame(int argc, char** argv)
{
    std::cout << "[JApplication]: Launching in GAME mode...\n";
    return LaunchEngine(nullptr, argc, argv);
}

bool JApplication::RunFromArgs(int argc, char** argv)
{
    const bool bEditor = HasFlag(argc, argv, "--editor");
    return bEditor ? RunEditor(argc, argv) : RunGame(argc, argv);
}

bool JApplication::LaunchEngine(IEditorBridge* editor, int argc, char** argv)
{
    auto& engine = JEngine::Get();

    engine.InitializeRuntime();

    TUniquePtr<IProjectLaunchUI> launchUI;
    if (editor)
    {
        //launchUI = editor->CreateProjectLaunchUI();
        launchUI = MakeUnique<ConsoleProjectLaunchUI>();
    }
    else
    {
        launchUI = MakeUnique<ConsoleProjectLaunchUI>();
    }

    ProjectLaunchResolver launchResolver(*launchUI);

    FProjectOpenRequest openRequest{};

    std::string explicitProjectPath;
    TryGetArgValue(argc, argv, "--project", explicitProjectPath);

    const std::string currentEngineRoot = GuessCurrentEngineRoot();

    // v1 behavior:
    // - if --project is passed, use it
    // - otherwise prompt via launch UI
    if (!launchResolver.ResolveDirectLaunch(explicitProjectPath, currentEngineRoot, openRequest))
    {
        std::cerr << "[JApplication]: Failed to resolve project launch request.\n";
        return false;
    }

    if (editor)
        engine.SetEditorBridge(editor);

    if (!engine.OpenProject(openRequest))
    {
        std::cerr << "[JApplication]: Failed to open project.\n";
        return false;
    }

    // Remember the project we successfully opened
    {
        LaunchSettings settings;
        settings.SetLastOpenedProjectFilePath(openRequest.projectFilePath);
        if (!settings.Save(currentEngineRoot))
        {
            std::cerr << "[JApplication]: Failed to save launch settings.\n";
        }
    }

    const bool success = engine.Run();

    if (!success)
        std::cerr << "[JApplication]: Engine failed to run.\n";

    return success;
}