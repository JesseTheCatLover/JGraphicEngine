//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "FolderPickerDialog.h"

#include <unordered_set>
#include <algorithm>
#include <functional>

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

    static void AddParentChain(std::unordered_set<std::string>& set, const std::string& leaf)
    {
        std::string p = UPath::Normalize(leaf);
        while (!p.empty() && p != "/" && p != "/Project")
        {
            set.insert(p);

            std::string next = UPath::GetParent(p);
            if (next.empty() || next == p)
                break;

            p = next;
        }

        if (p == "/Project")
            set.insert(p);
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
    m_View.expandedFolderPaths.clear();
    m_View.expandedFolderPaths.insert(m_View.rootPath); // ensure root opens

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
        if (IsSameOrUnderPath("/Project", parent))
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

    DrawDirectoryNodeByID(itRoot->second);

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

        m_Pending.renameTarget = created;
        m_Pending.renameBuffer = UPath::GetFileName(created);
        m_Pending.bStartRenameFocus   = true;

        // Also ensure it’s visible: open its parent chain
        m_Pending.expanded.insert(UPath::GetParent(created));
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
    if (!IsSameOrUnderPath("/Project", m_CurrentPath))
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

    m_bDirty = false;
}

void FolderPickerDialog::SetCurrentPath(const std::string& path)
{
    std::string normalized = UPath::Normalize(path);
    if (normalized.empty())
        normalized = "/Project";

    if (!IsSameOrUnderPath("/Project", normalized))
        normalized = "/Project";

    if (m_CurrentPath == normalized)
    {
        m_PathInputBuffer = m_CurrentPath;
        return;
    }

    // Expand FULL parent chain so selection/pending is visible
    for (std::string p = UPath::GetParent(normalized);
         !p.empty() && p != "/" && IsSameOrUnderPath("/Project", p);
         p = UPath::GetParent(p))
    {
        m_View.expandedFolderPaths.insert(p);
        if (p == "/Project") break;
    }

    m_CurrentPath = normalized;
    m_PathInputBuffer = m_CurrentPath;
    m_bDirty = true;
}

void FolderPickerDialog::DrawDirectoryNodeByID(AssetBrowserNodeID id)
{
    const auto itNode = m_View.nodeCache.find(id);
    if (itNode == m_View.nodeCache.end()) return;

    const FAssetBrowserNode& node = itNode->second;
    if (node.type != EAssetBrowserNodeType::Folder) return;

    const bool bIsSelected = (m_CurrentPath == node.virtualPath);

    const bool bExpandable = FolderHasChildren(node) || HasPendingChildFolder(node.virtualPath);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (bIsSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (bExpandable)
    {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow |
                 ImGuiTreeNodeFlags_OpenOnDoubleClick;
    }
    else
    {
        flags |= ImGuiTreeNodeFlags_Leaf |
                 ImGuiTreeNodeFlags_NoTreePushOnOpen; // important: no TreePop needed
    }

    const bool bExpanded =
    m_View.expandedFolderPaths.contains(node.virtualPath) ||
    IsAncestorPath(node.virtualPath, m_CurrentPath);

    if (bExpanded)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool bOpened =
        ImGui::TreeNodeEx((void*)(intptr_t)id, flags, "%s", node.displayName.c_str());

    if (ImGui::IsItemClicked())
        SetCurrentPath(node.virtualPath);

    // Track expand/collapse only if it can expand
    if (bExpandable && ImGui::IsItemToggledOpen())
    {
        if (bOpened)
            m_View.expandedFolderPaths.insert(node.virtualPath);
        else
            m_View.expandedFolderPaths.erase(node.virtualPath);
    }

    if (bExpandable && bOpened)
    {
        // Draw real children
        if (auto it = m_View.children.find(id); it != m_View.children.end())
        {
            for (AssetBrowserNodeID childID : it->second)
                DrawDirectoryNodeByID(childID);
        }

        // Draw pending children overlay
        if (auto itP = m_Pending.childrenByParent.find(node.virtualPath);
            itP != m_Pending.childrenByParent.end())
        {
            for (const std::string& pendingChildPath : itP->second)
            {
                // If it already exists for real, skip drawing pending duplicate
                if (m_View.pathToID.contains(pendingChildPath))
                    continue;

                DrawPendingChildRow(pendingChildPath);
            }
        }

        ImGui::TreePop();
    }
}

void FolderPickerDialog::ExpandAll()
{
    // Expand all folders currently visible in the projection
    for (const auto& [id, n] : m_View.nodeCache)
    {
        if (n.type == EAssetBrowserNodeType::Folder)
            m_View.expandedFolderPaths.insert(n.virtualPath);
    }
    m_bDirty = true;
}

void FolderPickerDialog::CollapseAll()
{
    m_View.expandedFolderPaths.clear();
    m_View.expandedFolderPaths.insert(m_View.rootPath); // keep root open
    m_bDirty = true;
}

void FolderPickerDialog::SyncPathInputToCurrentPath()
{
    m_PathInputBuffer = m_CurrentPath;
}

bool FolderPickerDialog::ApplyPathInput(EditorHost& host)
{
    std::string normalized = UPath::Normalize(m_PathInputBuffer);
    if (normalized.empty()) normalized = "/";

    if (!IsSameOrUnderPath("/Project", normalized))
    {
        SyncPathInputToCurrentPath();
        return false;
    }

    // If it doesn't exist yet, queue it for creation
    if (!IsFolderPresentInUI(normalized))
        AddPendingFolderPath(normalized);

    const std::string parent = UPath::GetParent(normalized);
    if (!parent.empty())
        m_View.expandedFolderPaths.insert(parent);

    m_CurrentPath = normalized;
    SyncPathInputToCurrentPath();
    m_bDirty = true;
    return true;
}

bool FolderPickerDialog::AddPendingFolderPath(const std::string& rawPath)
{
    std::string path = UPath::Normalize(rawPath);
    if (path.empty()) path = "/";

    if (!IsSameOrUnderPath("/Project", path)) return false;
    if (path == "/Project") return false;

    const std::string parent = UPath::GetParent(path);

    // Ensure all parents are expanded so the new child is visible
    if (!parent.empty())
    {
        m_Pending.expanded.insert(parent);
        AddParentChain(m_Pending.expanded, parent);
    }

    // Add parents too (optional but recommended)
    std::vector<std::string> chain;
    {
        std::string cur = path;
        while (!cur.empty() && cur != "/Project" && cur != "/")
        {
            chain.push_back(cur);
            cur = UPath::GetParent(cur);
        }
        std::reverse(chain.begin(), chain.end()); // create parents first
    }

    bool bAnyAdded = false;
    for (const std::string& p : chain)
    {
        if (m_Pending.createSet.insert(p).second)
        {
            m_Pending.createOrder.push_back(p);
            bAnyAdded = true;
        }
    }

    if (bAnyAdded)
    {
        RebuildPendingChildrenIndex();
        m_bDirty = true; // redraw & possibly adjust arrows
    }

    return true;
}

void FolderPickerDialog::RemovePendingFolderPathAndDescendants(const std::string& normalizedPath)
{
    auto isUnder = [&](const std::string& p)
    {
        return p == normalizedPath || IsAncestorPath(normalizedPath, p);
    };

    // rebuild order vector
    std::vector<std::string> newOrder;
    newOrder.reserve(m_Pending.createOrder.size());

    m_Pending.createSet.clear();

    for (const std::string& p : m_Pending.createOrder)
    {
        if (isUnder(p))
            continue;
        if (m_Pending.createSet.insert(p).second)
            newOrder.push_back(p);
    }

    m_Pending.createOrder = std::move(newOrder);
    RebuildPendingChildrenIndex();

    m_bDirty = true;
}

void FolderPickerDialog::RebuildPendingChildrenIndex()
{
    m_Pending.childrenByParent.clear();

    for (const std::string& childPath : m_Pending.createOrder)
    {
        const std::string child = UPath::Normalize(childPath);
        if (child.empty() || child == "/Project") continue;

        const std::string parent = UPath::GetParent(child);
        if (parent.empty()) continue;

        m_Pending.childrenByParent[parent].push_back(child);
    }

    // Optional: sort children by display name for stable UI
    for (auto& [parent, kids] : m_Pending.childrenByParent)
    {
        std::sort(kids.begin(), kids.end());
        kids.erase(std::unique(kids.begin(), kids.end()), kids.end());
    }
}

bool FolderPickerDialog::CommitPendingFolders(EditorHost& host)
{
    auto& abs = host.GetService<AssetBrowserService>();

    // create in parent-first order:
    std::vector<std::string> paths = m_Pending.createOrder;
    std::sort(paths.begin(), paths.end(), [](auto& a, auto& b)
    {
        return std::count(a.begin(), a.end(), '/') < std::count(b.begin(), b.end(), '/');
    });

    for (const std::string& p : paths)
    {
        // Skip if it already exists now (maybe created externally)
        if (m_View.pathToID.contains(p))
            continue;

        auto r = abs.CreateFolder(m_View, p);
    }

    m_Pending.createOrder.clear();
    m_Pending.createSet.clear();

    // also clear any pending-children mapping
    m_Pending.childrenByParent.clear();

    m_bDirty = true;
    return true;
}

void FolderPickerDialog::DrawPendingChildRow(const std::string& pendingPath)
{
    const std::string path = UPath::Normalize(pendingPath);

    // Unique scope for this row
    ImGui::PushID(path.c_str());

    const bool bIsSelected = (m_CurrentPath == path);
    const bool bExpandable = HasPendingChildFolder(path); // pending-only row

    ImGuiTreeNodeFlags flags = 0;
    if (bIsSelected) flags |= ImGuiTreeNodeFlags_Selected;

    if (bExpandable)
    {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;

        const bool bDefaultOpen = m_Pending.expanded.contains(path);
        ImGui::SetNextItemOpen(bDefaultOpen, ImGuiCond_Once);
    }
    else
    {
        flags |= ImGuiTreeNodeFlags_Leaf |
                 ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // Dim pending rows
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

    const bool bIsRenaming = (m_Pending.renameTarget == path);

    bool bOpened = false;

    if (!bIsRenaming)
    {
        const std::string display = UPath::GetFileName(path);
        bOpened = ImGui::TreeNodeEx("##PendingRow", flags, "%s", std::string(display + " *").c_str());
        bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_None);
        bool dbl = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        if (dbl)
        {
            m_Pending.renameTarget = path;
            m_Pending.renameBuffer = UPath::GetFileName(path);
            m_Pending.bStartRenameFocus = true;
        }
    }
    else
    {
        bOpened = ImGui::TreeNodeEx("##PendingRow", flags, "%s", "");

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
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(
            ImGui::GetStyle().FramePadding.x,
            0.0f)); // less vertical padding
        ImGui::SetNextItemWidth(220.0f);
        const bool bEnter =
            ImGui::InputText("##PendingRenameInput",
                             &m_Pending.renameBuffer,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        const bool enterPressed = bEnter; // from InputText(EnterReturnsTrue)
        const bool deactivatedAfterEdit = ImGui::IsItemDeactivatedAfterEdit();
        const bool deactivated = ImGui::IsItemDeactivated();

        // Optional: "click-away" even if ImGui doesn't count it as deactivated yet (rare cases)
        const bool clickAway =
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsItemHovered() &&
            !ImGui::IsAnyItemActive();

        const bool commit = enterPressed || deactivatedAfterEdit;
        const bool cancel = (!enterPressed) && deactivated && !deactivatedAfterEdit; // left focus without edits

        if (commit) {
            CommitPendingRename(path, m_Pending.renameBuffer);
            EndPendingRename(); // clear target/buffer
        }
        else if (cancel || clickAway)
        {
            CancelPendingRename(); // just clear target/buffer (or revert)
        }
    }

    ImGui::PopStyleColor(); // text dim

    // Persist expand/collapse for pending nodes
    if (bExpandable && ImGui::IsItemToggledOpen())
    {
        if (bOpened)
            m_Pending.expanded.insert(path);
        else
            m_Pending.expanded.erase(path);
    }

    // Selection click must be checked immediately after the TreeNodeEx item
    if (ImGui::IsItemClicked())
        SetCurrentPath(path);

    // Context menu: Rename
    if (!bIsRenaming && ImGui::BeginPopupContextItem("##PendingCtx"))
    {
        if (ImGui::MenuItem("Rename"))
        {
            m_Pending.renameTarget = path;
            m_Pending.renameBuffer = UPath::GetFileName(path);
            m_Pending.bStartRenameFocus = true;
        }
        if (ImGui::MenuItem("Delete"))
        {
            RemovePendingFolderPathAndDescendants(path);
            ImGui::EndPopup();
            ImGui::PopID();
            return;
        }
        ImGui::EndPopup();
    }

    // Right-aligned X button (same row)
    {
        ImGui::SameLine();

        const float xW = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;

        // Window-local right edge of content region
        const float rightLocal = ImGui::GetWindowContentRegionMax().x;

        float x = rightLocal - xW;
        if (x > ImGui::GetCursorPosX())
            ImGui::SetCursorPosX(x);

        // Important: tree items often span the whole row; allow the button to receive clicks
        ImGui::SetItemAllowOverlap();

        if (ImGui::SmallButton("X"))
        {
            RemovePendingFolderPathAndDescendants(path);
            ImGui::PopID();
            return;
        }
    }

    // Draw nested pending children if expanded
    if (bExpandable && bOpened)
    {
        if (auto itP = m_Pending.childrenByParent.find(path); itP != m_Pending.childrenByParent.end())
        {
            for (const std::string& child : itP->second)
                DrawPendingChildRow(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

bool FolderPickerDialog::CommitPendingRename(const std::string& oldPathRaw, const std::string& newNameRaw)
{
    const std::string oldPath = UPath::Normalize(oldPathRaw);
    std::string newName = UPath::SanitizeFileSystemName(newNameRaw);

    if (!m_Pending.createSet.contains(oldPath))
    {
        m_Pending.renameTarget.clear();
        return false;
    }

    if (!IsValidSingleFolderName(newName))
        return false;

    const std::string parent = UPath::GetParent(oldPath);
    if (!IsSameOrUnderPath("/Project", parent))
        return false;

    // compute desired new path
    std::string desiredNewPath = UPath::Join(parent, newName);

    // Rewrite pending-expanded set entries under oldPath -> desiredNewPath
    std::unordered_set<std::string> newExpanded;
    newExpanded.reserve(m_Pending.expanded.size());

    for (const auto& e0 : m_Pending.expanded)
    {
        std::string e = UPath::Normalize(e0);
        if (e == oldPath || IsAncestorPath(oldPath, e))
            e = desiredNewPath + e.substr(oldPath.size());

        newExpanded.insert(e);
    }
    m_Pending.expanded = std::move(newExpanded);

    // If collision (real or pending), choose numbered variant
    if (IsFolderPresentInUI(desiredNewPath) && desiredNewPath != oldPath)
    {
        desiredNewPath = MakeUniqueChildPath(parent, newName);
    }

    desiredNewPath = UPath::Normalize(desiredNewPath);
    if (desiredNewPath == oldPath)
    {
        m_Pending.renameTarget.clear();
        return true;
    }

    // Rewrite ALL pending paths in subtree oldPath/*
    std::vector<std::string> rewrittenOrder;
    rewrittenOrder.reserve(m_Pending.createOrder.size());
    std::unordered_set<std::string> rewrittenSet;
    rewrittenSet.reserve(m_Pending.createSet.size());

    for (const std::string& p0 : m_Pending.createOrder)
    {
        const std::string p = UPath::Normalize(p0);

        std::string out = p;

        if (p == oldPath || IsAncestorPath(oldPath, p))
        {
            // replace prefix oldPath with desiredNewPath
            out = desiredNewPath + p.substr(oldPath.size());
        }

        // keep unique & stable order
        if (rewrittenSet.insert(out).second)
            rewrittenOrder.push_back(out);
    }

    m_Pending.createOrder = std::move(rewrittenOrder);
    m_Pending.createSet = std::move(rewrittenSet);

    // Selection/currentPath rewrite too
    if (m_CurrentPath == oldPath || IsAncestorPath(oldPath, m_CurrentPath))
        SetCurrentPath(desiredNewPath + m_CurrentPath.substr(oldPath.size()));

    m_Pending.renameTarget.clear();
    RebuildPendingChildrenIndex();
    m_bDirty = true;
    return true;
}

void FolderPickerDialog::EndPendingRename()
{
    m_Pending.renameTarget.clear();
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
    if (!IsSameOrUnderPath("/Project", base)) base = "/Project";

    const std::string uniquePath = MakeUniqueChildPath(base, "NewFolder");

    AddPendingFolderPath(uniquePath);
    SetCurrentPath(uniquePath);

    // Ensure both real-tree base and pending-tree base open
    m_View.expandedFolderPaths.insert(base);
    m_Pending.expanded.insert(base);

    return uniquePath;
}

bool FolderPickerDialog::IsRootPath(const std::string& path)
{
    std::string p = UPath::Normalize(path);
    return p == "/" || p.empty();
}

bool FolderPickerDialog::IsSameOrUnderPath(const std::string& normalizedAncestor, const std::string& normalizedPath)
{
    const std::string a = normalizedAncestor;
    const std::string p = normalizedPath;
    if (a.empty() || p.empty()) return false;
    if (a == "/") return true;
    if (p == a) return true;
    if (p.size() <= a.size()) return false;
    if (p.rfind(a, 0) != 0) return false; // prefix
    return p[a.size()] == '/'; // boundary
}

bool FolderPickerDialog::IsFolderPresentInUI(const std::string& normalizedPath) const
{
    return (m_Pending.createSet.contains(normalizedPath)) ||
        m_View.pathToID.contains(normalizedPath); // projection knows it exists
}

bool FolderPickerDialog::HasPendingChildFolder(const std::string& normalizedParent) const
{
    // if you build m_Pending.childrenByParent
    auto it = m_Pending.childrenByParent.find(normalizedParent);
    return it != m_Pending.childrenByParent.end() && !it->second.empty();
}

bool FolderPickerDialog::FolderHasChildren(const FAssetBrowserNode& node) const
{
    // STRICT: only show arrow if we *know* there are child folders
    return node.bChildFoldersKnown && node.bHasChildFolders;
}