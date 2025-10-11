//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JApplication.h"
#include "Core/JEngine.h"
#include "EditorApp.h"
#include <iostream>
#include <algorithm>

bool JApplication::RunEditor()
{
    std::cout << "[JApplication]: Launching in EDITOR mode...\n";
    EditorApp editor;
    return LaunchEngine(&editor);
}

bool JApplication::RunGame()
{
    std::cout << "[JApplication]: Launching in GAME mode...\n";
    return LaunchEngine(nullptr);
}

bool JApplication::RunFromArgs(int argc, char** argv)
{
    // Parse args manually for simplicity
    bool bEditor = false;
    for (int i = 0; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
        if (arg == "--editor") bEditor = true;
    }

    return bEditor ? RunEditor() : RunGame();
}

bool JApplication::LaunchEngine(IEditorBridge* editor)
{
    auto& engine = JEngine::Get();

    if (editor)
        engine.SetEditorBridge(editor);

    bool success = engine.Run();

    if (!success)
        std::cerr << "[JApplication]: Engine failed to run.\n";

    return success;
}