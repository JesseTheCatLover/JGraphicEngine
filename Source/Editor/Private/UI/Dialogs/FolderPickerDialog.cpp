//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "FolderPickerDialog.h"

#include <unordered_set>
#include <algorithm>
#include <functional>
#include <iostream>

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Utilities/UPath.h"
#include "EditorRuntime.h"
#include "Core/EditorHost.h"
#include "Core/Services/AssetBrowser/AssetBrowserService.h"

namespace
{
    static bool IsAncestorPath(const std::string& normalizedAncestor, const std::string& normalizedPath)
    {
        if (normalizedAncestor.empty() || normalizedPath.empty())
            return false;

        if (normalizedAncestor == normalizedPath)
            return false;

        if (normalizedPath.size() <= normalizedAncestor.size())
            return false;

        if (normalizedPath.rfind(normalizedAncestor, 0) != 0)
            return false;

        return normalizedPath[normalizedAncestor.size()] == '/';
    }

    static std::string MakeNumberedName(const std::string& base, int n)
    {
        if (n <= 1) return base;
        return base + " (" + std::to_string(n) + ")";
    }

    static bool IsValidSingleFolderName(const std::string& name)
    {
        return UPath::IsValidFileSystemName(name);
    }
}

void FolderPickerDialog::OnCreate(EditorHost &host, EditorRuntime &runtime)
{
}

void FolderPickerDialog::OnDestroy(EditorHost &host, EditorRuntime &runtime)
{
    if (m_AssetsMutatedHandle.IsValid())
    {
        host.GetService<AssetBrowserService>().OnAssetsMutated().Remove(m_AssetsMutatedHandle);
        m_AssetsMutatedHandle = {};
    }

    m_OnAccepted = nullptr;
}

void FolderPickerDialog::OnOpen(EditorHost& host, EditorRuntime& runtime)
{
    m_Host = &host;
    m_bIsOpen = true;
    m_bJustOpened = true;

    m_bDirty = true;

    m_View.bShowFolders = true;
    m_View.bShowAssets  = false;

    m_View.projectionMode = EAssetBrowserProjectionMode::Tree;
    m_View.rootPath = "/Project";

    m_CurrentPath = "/Project";

    // dialog-specific expansion state is stored in the view:
    m_Host->GetService<AssetBrowserService>().CollapseAllFolders(m_View);
    m_Host->GetService<AssetBrowserService>().ExpandFolderNode(m_View, GetRootNodeID(host)); // ensure root opens

    m_View.selectionPolicy = EAssetBrowserSelectionPolicy::LocalSelection;

    // Make input buffer match
    SyncPathInputToCurrentPath();

    // subscribe to mutations so dialog refreshes when file tree changes
    auto& browserService = host.GetService<AssetBrowserService>();
    m_AssetsMutatedHandle = browserService.OnAssetsMutated().AddLambda([this](const FAssetOpResult& r)
    {
        // simplest safe policy: refresh on any changes
        const bool bAnyChange =
            !r.deletedPaths.empty() ||
            !r.pathRemappings.empty() ||
            !r.affectedVirtualFolders.empty();
        if (bAnyChange)
            m_bDirty = true;
    });
}

void FolderPickerDialog::OnClose()
{
    m_Pending = {};
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

    RefreshViewIfDirty(host, runtime);
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
    DrawBottomBar(host);
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
        const std::string parent = UPath::GetParent(m_CurrentPath);
        if (UPath::IsSameOrUnder("/Project", parent))
            SetCurrentPath(parent);
        else
            SetCurrentPath("/Project");
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

    // Ensure we have something projected
    auto itRoot = m_View.pathToID.find(UPath::Normalize(m_View.rootPath));
    if (itRoot == m_View.pathToID.end())
    {
        ImGui::TextDisabled("No folders available.");
        ImGui::EndChild();
        return;
    }

    DrawNodeByID(itRoot->second);

    ImGui::EndChild();
}

void FolderPickerDialog::DrawBottomBar(EditorHost& host)
{
    ImGui::TextUnformatted("Destination:");

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
    ImGui::SetNextItemWidth(-1.f);

    if (ImGui::InputText("##FolderPickerDestination",
                         &m_PathInputBuffer,
                         ImGuiInputTextFlags_EnterReturnsTrue))
    {
        ApplyPathInput(host);
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        ApplyPathInput(host);
    }

    ImGui::PopStyleVar();

    float fullWidth = ImGui::GetContentRegionAvail().x;

    const float buttonWidth = 80.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float totalRightButtonsW = buttonWidth * 2.0f + spacing;

    if (ImGui::Button("Create Folder", ImVec2(0, 0)))
    {
        std::string created = OnCreateFolderClicked();

        m_Pending.renameTargetID = MakePendingNodeID(created);
        m_Pending.renameBuffer = UPath::GetFileName(created);
        m_Pending.bStartRenameFocus = true;
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
        if (ApplyPathInput(host))
        {
            if (CommitPendingFolders(host))
            {
                bAccepted = true;
                bRequestClose = true;
            }
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

void FolderPickerDialog::SetDestinationPathOnPicker(const std::string &path)
{
    m_CurrentPath = UPath::Normalize(path.empty() ? m_CurrentPath : path);
    if (!UPath::IsSameOrUnder("/Project", m_CurrentPath))
        m_CurrentPath = "/Project";

    // Make input buffer match
    SyncPathInputToCurrentPath();

    if (m_Host)
    {
        ApplyPathInput(*m_Host); // will AddPendingFolderPath if missing, expand parents, etc.
    }
}

void FolderPickerDialog::RefreshViewIfDirty(EditorHost& host, EditorRuntime& runtime)
{
    if (!m_bDirty) return;

    auto& abs = host.GetService<AssetBrowserService>();

    m_View.projectionMode = EAssetBrowserProjectionMode::Tree;
    m_View.rootPath = "/Project";
    m_View.currentPath = m_CurrentPath;

    abs.RefreshView(m_View);

    InjectPendingFoldersIntoProjection();

    ApplyExpansionRequests();

    m_bDirty = false;
}

void FolderPickerDialog::ApplyExpansionRequests()
{
    if (m_ExpandedPendingNodes.empty())
        return;

    for (const std::string& path : m_ExpandedPendingNodes)
    {
        // Walk upward: leaf -> root
        std::string cur = path;

        while (true)
        {
            auto it = m_View.pathToID.find(cur);
            if (it != m_View.pathToID.end())
            {
                m_Host->GetService<AssetBrowserService>().ExpandFolderNode(m_View, it->second);
            }

            if (cur == "/Project")
                break;

            std::string parent = UPath::GetParent(cur);
            if (parent.empty() || parent == cur)
                break;

            cur = parent;
        }
    }

    m_ExpandedPendingNodes.clear();
}

void FolderPickerDialog::SetCurrentPath(const std::string& path)
{
    std::string normalizedPath = UPath::Normalize(path);

    if (normalizedPath.empty())
        normalizedPath = "/Project";

    if (!UPath::IsSameOrUnder("/Project", normalizedPath))
        normalizedPath = "/Project";

    m_CurrentPath = normalizedPath;
    m_PathInputBuffer = m_CurrentPath;

    // Defer expansion
    m_ExpandedPendingNodes.insert(normalizedPath);

    m_bDirty = true;
}

void FolderPickerDialog::DrawNodeByID(AssetBrowserNodeID id)
{
    const auto itNode = m_View.nodeCache.find(id);
    if (itNode == m_View.nodeCache.end())
        return;

    const FAssetBrowserNode& node = itNode->second;

    if (node.type != EAssetBrowserNodeType::Folder)
        return;

    const bool bIsPending =
        IsPendingNodeID(id);

    const bool bIsRenaming =
        bIsPending &&
        m_Pending.renameTargetID == id;

    const std::string nodePath = UPath::Normalize(node.virtualPath);

    const bool bIsSelected = (m_CurrentPath == nodePath);

    const bool bExpandable = FolderHasChildren(id);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (bIsSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (bExpandable)
    {
        flags |=
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick;
    }
    else
    {
        flags |=
            ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    const bool bExpanded = bIsPending ? m_ExpandedPendingNodes.count(nodePath) :
    m_Host->GetService<AssetBrowserService>().IsFolderExpanded(m_View, id) || IsAncestorPath(nodePath, m_CurrentPath);

    ImGui::SetNextItemOpen(bExpanded, ImGuiCond_Once);

    const std::string label = BuildNodeLabel(id, node);

    if (bIsPending)
        PushPendingNodeStyle();

    bool bOpened = false;

    if (!bIsRenaming)
    {
        bOpened = ImGui::TreeNodeEx((void*)(intptr_t)id, flags, "%s", label.c_str());
    }
    else
    {
        bOpened = ImGui::TreeNodeEx((void*)(intptr_t)id, flags, "%s", "");

        DrawPendingRenameWidget(id);
    }

    if (bIsPending)
        PopPendingNodeStyle();

    if (ImGui::IsItemClicked())
        SetCurrentPath(nodePath);

    if (bIsPending && !bIsRenaming)
    {
        HandlePendingNodeInteractions(id, node);

        if (HandlePendingDeleteButton(nodePath))
            return;
    }

    if (bExpandable && ImGui::IsItemToggledOpen())
    {
        std::cout
    << "Toggled "
    << node.virtualPath
    << std::endl;
        if (bIsPending)
        {
            if (bOpened)
            {
                if (!m_ExpandedPendingNodes.count(nodePath))
                {
                    m_ExpandedPendingNodes.insert(nodePath);
                }
            }
            else
            {
                m_ExpandedPendingNodes.erase(nodePath);
            }
        }
        else
        {
            if (bOpened)
                m_Host->GetService<AssetBrowserService>().ExpandFolderNode(m_View, id);
            else
                m_Host->GetService<AssetBrowserService>().CollapseFolderNode(m_View, id);
        }

        m_bDirty = true;
    }

    if (bExpandable && bOpened)
    {
        if (auto it = m_View.children.find(id);
            it != m_View.children.end())
        {
            for (AssetBrowserNodeID childID : it->second)
            {
                DrawNodeByID(childID);
            }
        }

        ImGui::TreePop();
    }
}

std::string FolderPickerDialog::BuildNodeLabel(AssetBrowserNodeID id, const FAssetBrowserNode& node) const
{
    if (IsPendingNodeID(id))
        return node.displayName + " *";

    return node.displayName;
}

void FolderPickerDialog::DrawPendingRenameWidget(AssetBrowserNodeID id)
{
    if (m_Pending.bStartRenameFocus)
    {
        ImGui::SetKeyboardFocusHere();
        m_Pending.bStartRenameFocus = false;
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg,        IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border,         IM_COL32(0,0,0,0));

    ImGui::SameLine();

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(ImGui::GetStyle().FramePadding.x, 0.0f));

    ImGui::SetNextItemWidth(220.0f);

    const bool bEnter =
        ImGui::InputText(
            "##PendingRenameInput",
            &m_Pending.renameBuffer,
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    const bool enterPressed = bEnter;
    const bool deactivatedAfterEdit = ImGui::IsItemDeactivatedAfterEdit();

    const bool deactivated = ImGui::IsItemDeactivated();

    const bool clickAway =
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsItemHovered() &&
        !ImGui::IsAnyItemActive();

    const bool commit = enterPressed || deactivatedAfterEdit;

    const bool cancel =
        (!enterPressed) &&
        deactivated &&
        !deactivatedAfterEdit;

    if (commit)
    {
        CommitPendingRename(id, m_Pending.renameBuffer);
        EndPendingRename();
    }
    else if (cancel || clickAway)
    {
        CancelPendingRename();
    }
}

bool FolderPickerDialog::HandlePendingDeleteButton(const std::string &nodePath)
{
    ImGui::SameLine();

    const float xW =
        ImGui::CalcTextSize("X").x +
        ImGui::GetStyle().FramePadding.x * 2.0f;

    const float rightLocal =
        ImGui::GetWindowContentRegionMax().x;

    float x = rightLocal - xW;

    if (x > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(x);

    ImGui::SetItemAllowOverlap();

    if (ImGui::SmallButton("X"))
    {
        RemovePendingFolderPathAndDescendants(nodePath);
        return true;
    }
    return false;
}

void FolderPickerDialog::PushPendingNodeStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
}

void FolderPickerDialog::PopPendingNodeStyle()
{
    ImGui::PopStyleColor();
}

void FolderPickerDialog::HandlePendingNodeInteractions(AssetBrowserNodeID id, const FAssetBrowserNode& node)
{
    const bool hovered =
        ImGui::IsItemHovered();

    const bool dbl =
        hovered &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    if (dbl)
    {
        m_Pending.renameTargetID = id;
        m_Pending.renameBuffer = node.displayName;
        m_Pending.bStartRenameFocus = true;
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Rename"))
        {
            m_Pending.renameTargetID = id;
            m_Pending.renameBuffer = node.displayName;
            m_Pending.bStartRenameFocus = true;
        }

        if (ImGui::MenuItem("Delete"))
        {
            RemovePendingFolderPathAndDescendants(
                node.virtualPath);
        }

        ImGui::EndPopup();
    }
}

void FolderPickerDialog::ExpandAll()
{
    m_Host->GetService<AssetBrowserService>().ExpandAllFolders(m_View);

    // also ensure pending roots
    for (const auto& path : m_Pending.pendingPaths)
    {
        m_ExpandedPendingNodes.insert(path);
    }

    m_bDirty = true;
}

void FolderPickerDialog::CollapseAll()
{
    m_Host->GetService<AssetBrowserService>().CollapseAllFolders(m_View);

    if (m_Host)
        m_Host->GetService<AssetBrowserService>().ExpandFolderNode(m_View, GetRootNodeID(*m_Host)); // Keep root open

    m_bDirty = true;
}

void FolderPickerDialog::SyncPathInputToCurrentPath()
{
    m_PathInputBuffer = m_CurrentPath;
}

bool FolderPickerDialog::ApplyPathInput(EditorHost& host)
{
    std::string normalized = UPath::Normalize(m_PathInputBuffer);
    if (normalized.empty())
        normalized = "/";

    if (!UPath::IsSameOrUnder("/Project", normalized))
    {
        SyncPathInputToCurrentPath();
        return false;
    }

    // If it doesn't exist yet, queue it for creation
    if (!IsFolderPresentInUI(normalized))
        AddPendingFolderPath(normalized);

    auto& browser = host.GetService<AssetBrowserService>();

    // Expand parent
    std::string parentPath = UPath::GetParent(normalized);

    if (!parentPath.empty())
    {
        AssetBrowserNodeID parentID = browser.GetRootNodeID(parentPath);
        m_Host->GetService<AssetBrowserService>().ExpandFolderNode(m_View, parentID);
    }

    m_CurrentPath = normalized;
    SyncPathInputToCurrentPath();
    m_bDirty = true;
    return true;
}

bool FolderPickerDialog::AddPendingFolderPath(const std::string& rawPath)
{
    std::string path = UPath::Normalize(rawPath);
    if (path.empty())
        return false;

    if (!UPath::IsSameOrUnder("/Project", path))
        return false;

    if (path == "/Project")
        return false;

    // Build full chain (parents -> leaf)
    std::vector<std::string> chain;

    for (std::string cur = path;
         !cur.empty() && cur != "/" && cur != "/Project";
         cur = UPath::GetParent(cur))
    {
        chain.push_back(cur);
    }

    std::reverse(chain.begin(), chain.end());

    bool bAdded = false;

    for (const std::string& p : chain)
    {
        if (m_Pending.pendingPaths.insert(p).second)
        {
            bAdded = true;
        }
    }

    if (bAdded)
    {
        m_bDirty = true;
    }

    return true;
}

void FolderPickerDialog::RemovePendingFolderPathAndDescendants(const std::string& path)
{
    std::vector<std::string> remaining;
    remaining.reserve(m_Pending.pendingPaths.size());

    for (const auto& p : m_Pending.pendingPaths)
    {
        if (p == path || IsAncestorPath(path, p))
            continue;

        remaining.push_back(p);
    }

    m_Pending.pendingPaths.clear();
    for (auto& p : remaining)
        m_Pending.pendingPaths.insert(std::move(p));

    m_bDirty = true;
}

bool FolderPickerDialog::CommitPendingFolders(EditorHost& host)
{
    auto& abs = host.GetService<AssetBrowserService>();

    std::vector<std::string> paths(
        m_Pending.pendingPaths.begin(),
        m_Pending.pendingPaths.end());

    std::sort(paths.begin(), paths.end(),
        [](const std::string& a, const std::string& b)
        {
            return std::count(a.begin(), a.end(), '/') <
                   std::count(b.begin(), b.end(), '/');
        });

    for (const std::string& path : paths)
    {
        if (auto* node = abs.GetNodeByPath(m_View, path))
        {
            if (!IsPendingNodeID(node->nodeID))
                continue; // already real folder
        }

        auto result = abs.CreateFolder(m_View, path);

        if (!result.bSuccess)
            return false;
    }

    m_Pending.pendingPaths.clear();
    m_bDirty = true;

    return true;
}

bool FolderPickerDialog::CommitPendingRename(AssetBrowserNodeID nodeID, const std::string& newNameRaw)
{
    auto it = m_View.nodeCache.find(nodeID);
    if (it == m_View.nodeCache.end())
        return false;

    const std::string oldPath =
        UPath::Normalize(it->second.virtualPath);

    // Only pending folders are renameable
    if (!IsPendingNodeID(nodeID))
        return false;

    std::string newName =
        UPath::SanitizeFileSystemName(newNameRaw);

    if (!IsValidSingleFolderName(newName))
        return false;

    const std::string parent = UPath::GetParent(oldPath);

    std::string desiredNewPath = UPath::Normalize(UPath::Join(parent, newName));

    if (desiredNewPath != oldPath && IsFolderPresentInUI(desiredNewPath))
    {
        desiredNewPath = MakeUniqueChildPath(parent, newName);
    }

    if (desiredNewPath == oldPath)
        return true;

    std::unordered_set<std::string> rewritten;

    rewritten.reserve(m_Pending.pendingPaths.size());

    for (const std::string& path : m_Pending.pendingPaths)
    {
        std::string out = path;

        if (path == oldPath || IsAncestorPath(oldPath, path))
        {
            out = desiredNewPath + path.substr(oldPath.size());
        }

        rewritten.insert(std::move(out));
    }

    m_Pending.pendingPaths = std::move(rewritten);

    if (m_CurrentPath == oldPath ||IsAncestorPath(oldPath, m_CurrentPath))
    {
        SetCurrentPath(desiredNewPath + m_CurrentPath.substr(oldPath.size()));
    }

    EndPendingRename();

    m_bDirty = true;

    return true;
}

void FolderPickerDialog::EndPendingRename()
{
    m_Pending.renameTargetID = InvalidNodeID;
    m_Pending.renameBuffer.clear();
    m_Pending.bStartRenameFocus = false;
}

void FolderPickerDialog::CancelPendingRename()
{
    // If we want cancel to revert, just discard buffer:
    EndPendingRename();
}

std::string FolderPickerDialog::MakeUniqueChildPath(const std::string& parent, const std::string& baseName) const
{
    const std::string p = UPath::Normalize(parent);

    for (int n = 1; n < 10000; ++n)
    {
        const std::string name = MakeNumberedName(baseName, n);
        const std::string candidate = UPath::Join(p, name);
        if (!IsFolderPresentInUI(candidate))
            return candidate;
    }

    // Fallback (should never happen)
    return UPath::Join(p, baseName);
}

std::string FolderPickerDialog::OnCreateFolderClicked()
{
    std::string base = UPath::Normalize(m_CurrentPath.empty() ? "/Project" : m_CurrentPath);
    if (!UPath::IsSameOrUnder("/Project", base)) base = "/Project";

    const std::string uniquePath = MakeUniqueChildPath(base, "NewFolder");

    AddPendingFolderPath(uniquePath);
    SetCurrentPath(uniquePath);

    return uniquePath;
}

bool FolderPickerDialog::IsRootPath(const std::string& path)
{
    std::string p = UPath::Normalize(path);
    return p == "/" || p.empty();
}

bool FolderPickerDialog::IsFolderPresentInUI(const std::string& path) const
{
    return m_View.pathToID.contains(path);
}

bool FolderPickerDialog::FolderHasChildren(AssetBrowserNodeID id) const
{
    auto it = m_View.nodeCache.find(id);

    if (it == m_View.nodeCache.end())
        return false; // node not found in the view

    return it->second.HasFolderChildren();
}

void FolderPickerDialog::InjectPendingFoldersIntoProjection()
{
    for (auto it = m_View.nodeCache.begin();
     it != m_View.nodeCache.end();)
    {
        if (IsPendingNodeID(it->first))
            it = m_View.nodeCache.erase(it);
        else
            ++it;
    }
    for (auto it = m_View.pathToID.begin();
     it != m_View.pathToID.end();)
    {
        if (IsPendingNodeID(it->second))
            it = m_View.pathToID.erase(it);
        else
            ++it;
    }

    for (auto& [id, children] : m_View.children)
    {
        children.erase(
            std::remove_if(
                children.begin(),
                children.end(),
                [](AssetBrowserNodeID child)
                {
                    return IsPendingNodeID(child);
                }),
            children.end());
    }
        for (auto& [id, children] : m_View.children)
    {
        children.erase(
            std::remove_if(
                children.begin(),
                children.end(),
                [](AssetBrowserNodeID child)
                {
                    return IsPendingNodeID(child);
                }),
            children.end());
    }

    std::cout
    << "pendingPaths="
    << m_Pending.pendingPaths.size()
    << std::endl;

    std::cout
        << "nodeCache="
        << m_View.nodeCache.size()
        << std::endl;

    // IMPORTANT: deterministic order for hierarchy correctness
    std::vector<std::string> paths(
        m_Pending.pendingPaths.begin(),
        m_Pending.pendingPaths.end());

    std::sort(paths.begin(), paths.end(),
        [](const std::string& a, const std::string& b)
        {
            return std::count(a.begin(), a.end(), '/') <
                   std::count(b.begin(), b.end(), '/');
        });

    // 1. Create nodes
    for (const std::string& path : paths)
    {
        if (m_View.pathToID.contains(path))
            continue;

        AssetBrowserNodeID id = MakePendingNodeID(path);

        FAssetBrowserNode node;
        node.nodeID = id;
        node.type = EAssetBrowserNodeType::Folder;
        node.virtualPath = path;
        node.displayName = UPath::GetFileName(path);

        node.childFolderState = EAssetBrowserChildState::Unknown;

        m_View.nodeCache[id] = node;
        m_View.pathToID[path] = id;
    }

    // 2. Link hierarchy (safe because sorted by depth)
    for (const std::string& path : paths)
    {
        auto itChild = m_View.pathToID.find(path);
        if (itChild == m_View.pathToID.end())
            continue;

        AssetBrowserNodeID childID = itChild->second;

        std::string parentPath = UPath::GetParent(path);
        auto itParent = m_View.pathToID.find(parentPath);

        if (itParent == m_View.pathToID.end())
            continue;

        AssetBrowserNodeID parentID = itParent->second;

        m_View.children[parentID].push_back(childID);

        auto& childNode = m_View.nodeCache[childID];
        childNode.parentID = parentID;

        auto& parentNode = m_View.nodeCache[parentID];
        parentNode.childFolderState = EAssetBrowserChildState::Present;
    }

    size_t pendingCount = 0;

    for (auto& [id,node] : m_View.nodeCache)
    {
        if (IsPendingNodeID(id))
            ++pendingCount;
    }

    std::cout
        << "pendingNodes="
        << pendingCount
        << std::endl;
}

AssetBrowserNodeID FolderPickerDialog::MakePendingNodeID(const std::string& path)
{
    std::hash<std::string> hasher;

    return PendingNodeMask | static_cast<AssetBrowserNodeID>(hasher(path));
}

bool FolderPickerDialog::IsPendingNodeID(AssetBrowserNodeID id)
{
    return (id & PendingNodeMask) != 0;
}

AssetBrowserNodeID FolderPickerDialog::GetRootNodeID(EditorHost &host) const
{
    return host.GetService<AssetBrowserService>().GetRootNodeID(m_View.rootPath);
}
