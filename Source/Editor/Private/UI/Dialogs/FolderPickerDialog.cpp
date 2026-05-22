//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "FolderPickerDialog.h"

#include <unordered_set>
#include <algorithm>

#include "imgui.h"
#include "Assets/FAssetRecord.h"
#include "Utilities/UPath.h"
#include "EditorRuntime.h"

bool FolderPickerDialog::IsRootPath(const std::string& path)
{
    std::string p = UPath::Normalize(path);
    return p == "/" || p.empty();
}

void FolderPickerDialog::OnCreate(EditorHost &host, EditorRuntime &runtime)
{
}

void FolderPickerDialog::OnDestroy(EditorHost &host, EditorRuntime &runtime)
{
    m_Result = FFolderPickerResult{};
}

void FolderPickerDialog::OnOpen(EditorHost& /*host*/, EditorRuntime& /*runtime*/)
{
    m_bIsOpen = true;
    m_bJustOpened = true; // next Draw will SetNextWindowFocus()
    m_Result = FFolderPickerResult{}; // reset result for this run
}

void FolderPickerDialog::OnClose()
{
    m_bIsOpen = false;
}

void FolderPickerDialog::OnRequestFocus(EditorHost& /*host*/, EditorRuntime& /*runtime*/)
{
    // Bring to front without re-running OnOpen logic
    m_bJustOpened = true;
}

void FolderPickerDialog::Draw(EditorHost& host, EditorRuntime& runtime)
{
    if (!m_bIsOpen)
        return;

    // One-shot focus to bring the dialog to front
    if (m_bJustOpened)
    {
        ImGui::SetNextWindowFocus();
        m_bJustOpened = false;
    }

    // Initial size (first time)
    ImGui::SetNextWindowSize(ImVec2(m_InitialWidth, m_InitialHeight), ImGuiCond_FirstUseEver);

    // Size limits
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(m_MinWidth, m_MinHeight), // min size
        ImVec2(m_MaxWidth, m_MaxHeight)  // max size
    );

    // Center on first appearance
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(),
        ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f)); // pivot = center

    const ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoCollapse;


    bool open = m_bIsOpen;
    const std::string windowTitle = m_Title.empty() ? "Folder Picker" : "Folder Picker - " + m_Title;
    if (!ImGui::Begin(windowTitle.c_str(), &open, windowFlags))
    {
        m_bIsOpen = open;
        ImGui::End();
        return;
    }
    m_bIsOpen = open;

    RefreshIfDirty(runtime);
    DrawContent(host, runtime);

    ImGui::End();
}

void FolderPickerDialog::DrawContent(EditorHost& host, EditorRuntime& runtime)
{
    // Top bar: Up, Home, current path
    DrawTopBar();

    ImGui::Separator();

    // Directory list
    DrawDirectoryList();

    ImGui::Separator();

    // Bottom bar: Cancel / Select
    DrawBottomBar();
}

void FolderPickerDialog::DrawTopBar()
{
    if (ImGui::Button("Home"))
    {
        SetCurrentPath("/Project");
    }

    ImGui::SameLine();

    if (ImGui::Button("Up"))
    {
        SetCurrentPath(ComputeParentPath(m_CurrentPath));
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("Current:");

    ImGui::SameLine();
    ImGui::TextUnformatted(m_CurrentPath.c_str());
}

void FolderPickerDialog::DrawDirectoryList()
{
    ImGui::TextUnformatted("Subfolders:");
    ImGui::BeginChild("##FolderPicker_DirList",
                      ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2.0f),
                      true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    if (m_Directories.empty())
    {
        ImGui::TextDisabled("No subfolders here.");
    }
    else
    {
        for (const FDirectoryEntry& dir : m_Directories)
        {
            // Show as selectable row
            bool dummySelected = false;
            if (ImGui::Selectable(dir.name.c_str(), dummySelected,
                                  ImGuiSelectableFlags_AllowDoubleClick))
            {
                // Single-click navigates into it.
                SetCurrentPath(dir.virtualPath);
            }
        }
    }

    ImGui::EndChild();
}

void FolderPickerDialog::DrawBottomBar()
{
    float contentWidth = ImGui::GetContentRegionAvail().x;

    // Left: display "Destination"
    ImGui::TextUnformatted("Destination:");
    ImGui::SameLine();
    ImGui::TextUnformatted(m_CurrentPath.c_str());

    // Right: buttons
    float buttonWidth   = 80.0f;
    float spacing       = ImGui::GetStyle().ItemSpacing.x;
    float totalButtonsW = buttonWidth * 2.0f + spacing;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (contentWidth - totalButtonsW));

    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
    {
        m_Result.bAccepted = false;
        m_Result.selectedPath.clear();
        OnClose();
    }

    ImGui::SameLine();

    bool canSelect = !m_CurrentPath.empty();
    if (!canSelect)
        ImGui::BeginDisabled();

    if (ImGui::Button("Select", ImVec2(buttonWidth, 0)))
    {
        m_Result.bAccepted = true;
        m_Result.selectedPath = m_CurrentPath;
        OnClose();
    }

    if (!canSelect)
        ImGui::EndDisabled();
}

void FolderPickerDialog::RefreshIfDirty(EditorRuntime& runtime)
{
    if (!m_bDirty)
        return;

    m_Directories.clear();
    BuildDirectories(runtime);
    m_bDirty = false;
}

void FolderPickerDialog::SetCurrentPath(const std::string& path)
{
    std::string normalized = UPath::Normalize(path);
    if (normalized.empty())
        normalized = "/";

    if (m_CurrentPath == normalized)
        return;

    m_CurrentPath = normalized;
    m_bDirty = true;
}

std::string FolderPickerDialog::ComputeParentPath(const std::string& path) const
{
    std::string p = UPath::Normalize(path);
    if (IsRootPath(p))
        return "/";

    while (p.size() > 1 && p.back() == '/')
        p.pop_back();

    std::size_t lastSlash = p.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash == 0)
        return "/";

    return p.substr(0, lastSlash);
}

void FolderPickerDialog::BuildDirectories(EditorRuntime& runtime)
{
    std::vector<const FAssetRecord*> assets = runtime.GetFile().GetUserVisibleAssets();

    const std::string parentDir = UPath::Normalize(m_CurrentPath);

    std::unordered_set<std::string> seenDirVirtualPaths;
    seenDirVirtualPaths.reserve(32);

    for (const FAssetRecord* record : assets)
    {
        if (!record)
            continue;

        const std::string& assetPathRaw = record->virtualPath;
        if (assetPathRaw.empty())
            continue;

        const std::string assetVirtualPath = UPath::Normalize(assetPathRaw);
        if (assetVirtualPath.empty() || assetVirtualPath[0] != '/')
            continue;

        std::string normalizedParent = parentDir;

        if (normalizedParent == "/")
        {
            // Root: get first segment /Project from /Project/Textures/Wood.jasset
            std::size_t secondSlash = assetVirtualPath.find('/', 1);
            if (secondSlash == std::string::npos)
                continue; // asset directly in root, no folder

            std::string childName = assetVirtualPath.substr(1, secondSlash - 1);
            std::string childVirtualPath = assetVirtualPath.substr(0, secondSlash);

            if (!seenDirVirtualPaths.insert(childVirtualPath).second)
                continue;

            FDirectoryEntry entry;
            entry.name = std::move(childName);
            entry.virtualPath = std::move(childVirtualPath);
            m_Directories.emplace_back(std::move(entry));
        }
        else
        {
            // Non-root: require asset path starts with "/ParentDir/"
            std::string prefix = normalizedParent;
            prefix.push_back('/');

            if (assetVirtualPath.rfind(prefix, 0) != 0)
                continue;

            std::string remainder = assetVirtualPath.substr(prefix.size());
            if (remainder.empty())
                continue;

            std::size_t slashPos = remainder.find('/');
            if (slashPos == std::string::npos)
            {
                // asset directly under parent, no child folder
                continue;
            }

            std::string childName = remainder.substr(0, slashPos);
            std::string childVirtualPath = normalizedParent;
            childVirtualPath.push_back('/');
            childVirtualPath += childName;

            if (!seenDirVirtualPaths.insert(childVirtualPath).second)
                continue;

            FDirectoryEntry entry;
            entry.name = std::move(childName);
            entry.virtualPath = std::move(childVirtualPath);
            m_Directories.emplace_back(std::move(entry));
        }
    }

    std::sort(m_Directories.begin(), m_Directories.end(),
              [](const FDirectoryEntry& a, const FDirectoryEntry& b)
              {
                  return a.name < b.name;
              });
}