//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include "UI/IEditorDialog.h"

class EditorHost;
class EditorRuntime;

struct FFolderPickerResult
{
    bool bAccepted = false;
    std::string selectedPath;
};

class FolderPickerDialog final : public IEditorDialog
{
private:
    bool m_bIsOpen = false;
    bool m_bJustOpened = false;  // for one-shot focus
    bool m_bDirty = true;

    std::string m_Title = "Select Destination Folder";
    std::string m_CurrentPath = "/Project";  // default virtual folder path

    struct FDirectoryEntry
    {
        std::string name;        // "Textures"
        std::string virtualPath; // "/Project/Textures"
    };

    std::vector<FDirectoryEntry> m_Directories;

    // Selection is implicit: "currentPath" is the selected folder
    FFolderPickerResult m_Result;

    // Layout
    float m_InitialWidth = 300.0f;
    float m_InitialHeight = 400.0f;

    float m_MinWidth = 400.0f;
    float m_MinHeight = 300.0f;
    float m_MaxWidth = 600.0f;
    float m_MaxHeight = 900.0f;

public:
    FolderPickerDialog() = default;

    void OnCreate(EditorHost& host, EditorRuntime& runtime) override;
    void OnDestroy(EditorHost& host, EditorRuntime& runtime) override;
    void OnOpen(EditorHost& host, EditorRuntime& runtime) override;
    void OnClose() override;
    void OnRequestFocus(EditorHost& host, EditorRuntime& runtime) override;
    void Draw(EditorHost& host, EditorRuntime& runtime) override;

    [[nodiscard]] const char* GetName() const override { return "FolderPickerDialog"; }
    [[nodiscard]] bool IsOpen() const override { return m_bIsOpen; }

    [[nodiscard]] const FFolderPickerResult& GetResult() const { return m_Result; }

    // Optionally let caller override title
    void SetTitle(const std::string& title) { m_Title = title; }
    void SetInitialPath(const std::string& path) { SetCurrentPath(path); }

private:
    void RefreshIfDirty(EditorRuntime& runtime);
    void BuildDirectories(EditorRuntime& runtime);

    void DrawContent(EditorHost& host, EditorRuntime& runtime);
    void DrawTopBar();
    void DrawDirectoryList();
    void DrawBottomBar();

    void SetCurrentPath(const std::string& path);
    [[nodiscard]] std::string ComputeParentPath(const std::string& path) const;

    static bool IsRootPath(const std::string& path);
};
