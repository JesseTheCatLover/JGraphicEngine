//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "FolderPickerDialog.h"

#include <unordered_set>
#include <algorithm>
#include <functional>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Assets/FAssetRecord.h"
#include "Utilities/UPath.h"
#include "EditorRuntime.h"

namespace
{
    static bool IsAncestorPath(const std::string& ancestor, const std::string& path)
    {
        if (ancestor.empty() || path.empty())
            return false;

        if (ancestor == path)
            return false;

        if (path.size() <= ancestor.size())
            return false;

        if (path.rfind(ancestor, 0) != 0)
            return false;

        return path[ancestor.size()] == '/';
    }
}

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
    m_OnAccepted = nullptr;
}

void FolderPickerDialog::OnOpen(EditorHost& /*host*/, EditorRuntime& /*runtime*/)
{
    m_bIsOpen = true;
    m_bJustOpened = true; // next Draw will SetNextWindowFocus()

    // Optional safety: clear stale callback for this run
    m_OnAccepted = nullptr;

    m_bDirty = true;

    SyncPathInputToCurrentPath();
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

    // Bottom bar: Create Folder / Cancel / Select
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

    if (ImGui::Button("Expand All"))
    {
        ExpandAll();
    }

    ImGui::SameLine();

    if (ImGui::Button("Collapse All"))
    {
        CollapseAll();
    }
}

void FolderPickerDialog::DrawDirectoryList()
{
    ImGui::TextUnformatted("Folders:");
    ImGui::BeginChild("##FolderPicker_DirTree",
                      ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2.0f),
                      true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    if (m_RootDirectories.empty())
    {
        ImGui::TextDisabled("No folders available.");
    }
    else
    {
        DrawDirectoryTree();
    }

    ImGui::EndChild();
}

void FolderPickerDialog::DrawBottomBar()
{
    ImGui::TextUnformatted("Destination:");

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
    ImGui::SetNextItemWidth(-1.f);

    if (ImGui::InputText("##FolderPickerDestination",
                         &m_PathInputBuffer,
                         ImGuiInputTextFlags_EnterReturnsTrue))
    {
        ApplyPathInput();
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        ApplyPathInput();
    }

    ImGui::PopStyleVar();

    float fullWidth = ImGui::GetContentRegionAvail().x;

    const float buttonWidth = 80.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float totalRightButtonsW = buttonWidth * 2.0f + spacing;

    if (ImGui::Button("Create Folder", ImVec2(0, 0)))
    {
        // TODO: Implement folder creation logic
    }

    const float rightButtonsX = ImGui::GetCursorPosX() + (fullWidth - totalRightButtonsW);
    ImGui::SameLine();
    ImGui::SetCursorPosX(rightButtonsX);

    bool bRequestClose = false;
    bool bAccepted = false;

    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
    {
        bRequestClose = true;
    }

    ImGui::SameLine();

    const bool bCanSelect = !m_CurrentPath.empty();
    if (!bCanSelect)
        ImGui::BeginDisabled();

    if (ImGui::Button("Select", ImVec2(buttonWidth, 0)))
    {
        if (ApplyPathInput())
        {
            bAccepted = true;
            bRequestClose = true;
        }
    }

    if (!bCanSelect)
        ImGui::EndDisabled();

    if (bAccepted && m_OnAccepted)
    {
        m_OnAccepted(m_CurrentPath);
    }

    if (bRequestClose)
    {
        OnClose();
    }
}

void FolderPickerDialog::RefreshIfDirty(EditorRuntime& runtime)
{
    if (!m_bDirty)
        return;

    m_RootDirectories.clear();
    BuildDirectoryTree(runtime);
    m_bDirty = false;
}

void FolderPickerDialog::SetCurrentPath(const std::string& path)
{
    std::string normalized = UPath::Normalize(path);
    if (normalized.empty())
        normalized = "/";

    if (m_CurrentPath == normalized)
    {
        m_PathInputBuffer = m_CurrentPath;
        return;
    }

    m_CurrentPath = normalized;
    m_PathInputBuffer = m_CurrentPath;
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

void FolderPickerDialog::BuildDirectoryTree(EditorRuntime& runtime)
{
    std::vector<const FAssetRecord*> assets = runtime.GetFile().GetAllUserVisibleAssets();

    for (const FAssetRecord* record : assets)
    {
        if (!record)
            continue;

        const std::string normalizedAssetPath = UPath::Normalize(record->virtualPath);
        if (normalizedAssetPath.empty() || normalizedAssetPath[0] != '/')
            continue;

        // Remove filename, keep only folder path
        const std::size_t lastSlash = normalizedAssetPath.find_last_of('/');
        if (lastSlash == std::string::npos || lastSlash == 0)
            continue;

        const std::string folderPath = normalizedAssetPath.substr(0, lastSlash);
        if (folderPath.empty() || folderPath[0] != '/')
            continue;

        std::vector<FDirectoryNode>* currentChildren = &m_RootDirectories;
        std::string currentPath;

        std::size_t pos = 1; // skip leading '/'

        while (pos < folderPath.size())
        {
            const std::size_t slash = folderPath.find('/', pos);
            const bool bLastSegment = (slash == std::string::npos);

            const std::size_t segmentEnd = bLastSegment ? folderPath.size() : slash;
            const std::string segmentName = folderPath.substr(pos, segmentEnd - pos);

            if (segmentName.empty())
                break;

            currentPath += "/";
            currentPath += segmentName;

            FDirectoryNode* node = FindOrAddChildNode(*currentChildren, segmentName, currentPath);
            currentChildren = &node->children;

            if (bLastSegment)
                break;

            pos = slash + 1;
        }
    }

    // Sort recursively
    std::function<void(std::vector<FDirectoryNode>&)> sortNodes =
        [&](std::vector<FDirectoryNode>& nodes)
        {
            std::sort(nodes.begin(), nodes.end(),
                      [](const FDirectoryNode& a, const FDirectoryNode& b)
                      {
                          return a.name < b.name;
                      });

            for (FDirectoryNode& node : nodes)
                sortNodes(node.children);
        };

    sortNodes(m_RootDirectories);
}

void FolderPickerDialog::DrawDirectoryTree()
{
    for (FDirectoryNode& node : m_RootDirectories)
        DrawDirectoryNode(node);
}

void FolderPickerDialog::DrawDirectoryNode(FDirectoryNode& node)
{
    const bool bIsSelected  = (m_CurrentPath == node.virtualPath);
    const bool bHasChildren = !node.children.empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (bIsSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (!bHasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool bShouldBeOpen = IsNodeExpanded(node.virtualPath);

    if (IsAncestorPath(node.virtualPath, m_CurrentPath))
        bShouldBeOpen = true;

    ImGui::SetNextItemOpen(bShouldBeOpen, ImGuiCond_Always);

    const bool bOpened = ImGui::TreeNodeEx(node.virtualPath.c_str(), flags, "%s", node.name.c_str());

    if (ImGui::IsItemClicked())
    {
        SetCurrentPath(node.virtualPath);
    }

    if (bHasChildren)
    {
        if (bOpened)
            m_ExpandedPaths.insert(node.virtualPath);
        else
            m_ExpandedPaths.erase(node.virtualPath);
    }

    if (bOpened)
    {
        for (FDirectoryNode& child : node.children)
            DrawDirectoryNode(child);

        ImGui::TreePop();
    }
}

FolderPickerDialog::FDirectoryNode* FolderPickerDialog::FindOrAddChildNode(
    std::vector<FDirectoryNode>& children,
    const std::string& name,
    const std::string& virtualPath)
{
    for (FDirectoryNode& child : children)
    {
        if (child.virtualPath == virtualPath)
            return &child;
    }

    FDirectoryNode node;
    node.name = name;
    node.virtualPath = virtualPath;
    children.emplace_back(std::move(node));
    return &children.back();
}

void FolderPickerDialog::ExpandAll()
{
    m_ExpandedPaths.clear();
    ExpandAllNodes(m_RootDirectories);
}

void FolderPickerDialog::ExpandAllNodes(const std::vector<FDirectoryNode>& nodes)
{
    for (const FDirectoryNode& node : nodes)
    {
        if (!node.children.empty())
            m_ExpandedPaths.insert(node.virtualPath);

        ExpandAllNodes(node.children);
    }
}

void FolderPickerDialog::CollapseAll()
{
    m_ExpandedPaths.clear();
}

bool FolderPickerDialog::IsNodeExpanded(const std::string& virtualPath) const
{
    return m_ExpandedPaths.find(virtualPath) != m_ExpandedPaths.end();
}

void FolderPickerDialog::SetNodeExpanded(const std::string& virtualPath, bool bExpanded)
{
    if (bExpanded)
        m_ExpandedPaths.insert(virtualPath);
    else
        m_ExpandedPaths.erase(virtualPath);
}

void FolderPickerDialog::SyncPathInputToCurrentPath()
{
    m_PathInputBuffer = m_CurrentPath;
}

bool FolderPickerDialog::ApplyPathInput()
{
    std::string normalized = UPath::Normalize(m_PathInputBuffer);
    if (normalized.empty())
        normalized = "/";

    if (PathExistsInTree(normalized) || normalized == "/")
    {
        SetCurrentPath(normalized);
        SyncPathInputToCurrentPath();
        return true;
    }

    SyncPathInputToCurrentPath();
    return false;
}

bool FolderPickerDialog::PathExistsInTree(const std::string& path) const
{
    return PathExistsInTreeRecursive(m_RootDirectories, path);
}

bool FolderPickerDialog::PathExistsInTreeRecursive(const std::vector<FDirectoryNode>& nodes,
                                                   const std::string& path) const
{
    for (const FDirectoryNode& node : nodes)
    {
        if (node.virtualPath == path)
            return true;

        if (PathExistsInTreeRecursive(node.children, path))
            return true;
    }

    return false;
}