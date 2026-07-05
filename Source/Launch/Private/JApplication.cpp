//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "JApplication.h"

#include <iostream>
#include <algorithm>
#include <string>

#include "Core/JEngine.h"
#include "InkBlueEditorApp.h"
#include "Core/Project/FProjectOpenRequest.h"
#include "ProjectLaunch/ProjectLaunchResolver.h"
#include "ProjectLaunch/ConsoleProjectLaunchUI.h"
#include "ProjectLaunch/ImGuiProjectLaunchUI.h"
#include "Core/Project/LaunchSettings.h"
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
        std::filesystem::path exePath = UFileSystem::GetExecutablePath();

        // <EngineRoot>/Binaries/<exe>
        std::filesystem::path engineRoot = exePath.parent_path();

        return UPath::NormalizePhysical(engineRoot.string());
    }
}

bool JApplication::RunEditor(int argc, char** argv)
{
    std::cout << "[JApplication]: Launching in EDITOR mode...\n";
    InkBlueEditorApp editor;
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

    if (!engine.InitializeRuntime())
    {
        std::cerr << "[JApplication]: Failed to initialize engine runtime.\n";
        return false;
    }

    const std::string currentEngineRoot = GuessCurrentEngineRoot();

    TUniquePtr<IProjectLaunchUI> launchUI;
    if (editor)
    {
        launchUI = MakeUnique<ImGuiProjectLaunchUI>(engine.GetPlatformSurface(), engine.GetRenderBackend(),
            currentEngineRoot);
    }
    else
    {
        launchUI = MakeUnique<ConsoleProjectLaunchUI>();
    }

    if (!launchUI)
    {
        std::cerr << "[JApplication]: Failed to create launch UI.\n";
        return false;
    }

    ProjectLaunchResolver launchResolver(*launchUI);

    FProjectOpenRequest openRequest{};

    std::string explicitProjectPath;
    TryGetArgValue(argc, argv, "--project", explicitProjectPath);

    // v1 behavior:
    // - if --project is passed, use it
    // - otherwise prompt via launch UI
    if (!launchResolver.ResolveDirectLaunch(explicitProjectPath, currentEngineRoot, openRequest))
    {
        std::cerr << "[JApplication]: Failed to resolve project launch request.\n";
        engine.Shutdown();
        return false;
    }

    launchUI->Shutdown();
    launchUI.reset();

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

        // Make sure to load first so we don't overwrite the existing list!
        settings.Load(currentEngineRoot);

        settings.RegisterOpenedProject(openRequest.projectFilePath);

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