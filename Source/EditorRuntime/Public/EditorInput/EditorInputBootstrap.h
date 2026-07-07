//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class InputSubsystem;

namespace EditorInputBootstrap
{
    // Installs editor input mapping (axis + hotkeys) into the input subsystem.
    bool InstallEditorInputMapping(InputSubsystem& inputSubsystem);
}