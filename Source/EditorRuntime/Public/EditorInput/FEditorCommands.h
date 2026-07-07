//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

namespace FEditorCommands
{
    namespace App
    {
        constexpr const char* Quit = "Editor.App.Quit";
    }

    namespace File
    {
        constexpr const char* NewScene = "Editor.File.NewScene";
        constexpr const char* OpenScene = "Editor.File.OpenScene";
        constexpr const char* Save = "Editor.File.Save";
        constexpr const char* SaveAs = "Editor.File.SaveAs";
        constexpr const char* SaveAll = "Editor.File.SaveAll";
    }

    namespace Edit
    {
        constexpr const char* Copy = "Editor.Edit.Copy";
        constexpr const char* Paste = "Editor.Edit.Paste";
        constexpr const char* Delete = "Editor.Edit.Delete";
    }

    namespace History
    {
        constexpr const char* Undo = "Editor.History.Undo";
        constexpr const char* Redo = "Editor.History.Redo";
        constexpr const char* Clear = "Editor.History.Clear";
    }

    namespace View
    {
        constexpr const char* ToggleSceneHierarchy = "Editor.View.ToggleSceneHierarchy";
        constexpr const char* ToggleInspector = "Editor.View.ToggleInspector";
        constexpr const char* ToggleAssetBrowser = "Editor.View.ToggleAssetBrowser";
        constexpr const char* ToggleConsole = "Editor.View.ToggleConsole";
    }

    namespace Viewport
    {
        constexpr const char* FocusSelection = "Editor.Viewport.FocusSelection";
        constexpr const char* ToggleGameView = "Editor.Viewport.ToggleGameView";
        constexpr const char* SetSingleView = "Editor.Viewport.SetSingleView";
        constexpr const char* SetDoubleView = "Editor.Viewport.SetDoubleView";
        constexpr const char* SetTripleView = "Editor.Viewport.SetTripleView";
        constexpr const char* SetQuadView = "Editor.Viewport.SetQuadView";
        constexpr const char* ToggleTabVisibility = "Editor.Viewport.ToggleTabVisibility";
    }

    namespace Tools
    {
        constexpr const char* Translate = "Editor.Tools.Translate";
        constexpr const char* Rotate = "Editor.Tools.Rotate";
        constexpr const char* Scale = "Editor.Tools.Scale";
    }
}