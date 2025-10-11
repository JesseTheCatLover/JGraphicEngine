//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class IEditorBridge;

/**
 * @class JApplication
 * @brief Top-level entry point for launching the engine in editor or game mode.
 *
 * This class encapsulates all engine bootstrapping logic. It determines
 * which environment to launch (Editor, Game, etc.) and initializes JEngine accordingly.
 */
class JApplication
{
public:
    JApplication() = delete;
    ~JApplication() = delete;

    /** Launches in editor mode (e.g. from the Editor executable). */
    static bool RunEditor();

    /** Launches in standalone game mode (no editor bridge). */
    static bool RunGame();

    /** Launches based on a command-line flag (e.g., --editor or --game). */
    static bool RunFromArgs(int argc, char** argv);

private:
    static bool LaunchEngine(IEditorBridge* editor);
};
