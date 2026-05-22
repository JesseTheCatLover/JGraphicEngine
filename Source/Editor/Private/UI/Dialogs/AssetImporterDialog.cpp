//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetImporterDialog.h"
#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"

#include <imgui.h>

#include "FolderPickerDialog.h"
#include "imgui_internal.h"

void AssetImporterDialog::OnCreate(EditorHost& host, EditorRuntime& runtime)
{
    // No heavy engine side setup needed here.
    // Dialog is persistent; UI state will be reset in OnOpen().
}

void AssetImporterDialog::OnDestroy(EditorHost& host, EditorRuntime& runtime)
{
    (void)host;
    (void)runtime;

    m_Items.clear();
    m_SelectedIndices.clear();
    m_bIsOpen = false;
}

void AssetImporterDialog::OnOpen(EditorHost& host, EditorRuntime& runtime)
{
    IEditorDialog::OnOpen(host, runtime);

    m_bIsOpen = true;

    // Reset UI state each time user explicitly opens importer
    m_Items.clear();
    m_SelectedIndices.clear();
}

void AssetImporterDialog::Draw(EditorHost& host, EditorRuntime& runtime)
{
    if (!m_bIsOpen)
        return;

    // Request focus
    if (m_bRequestFocus)
    {
        ImGui::SetNextWindowFocus();
        m_bRequestFocus = false;
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
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    // Window begin
    bool open = m_bIsOpen;
    if (!ImGui::Begin("Asset Import Batcher", &open, windowFlags))
    {
        m_bIsOpen = open;
        ImGui::End();
        return;
    }
    m_bIsOpen = open;

    // Compute width layout:
    // Total width available inside this window
    float totalWidth = ImGui::GetContentRegionAvail().x;
    if (totalWidth < m_MinLeftPaneWidth + m_MinRightPaneWidth)
        totalWidth = m_MinLeftPaneWidth + m_MinRightPaneWidth;

    // On first active frame, initialize splitter in the center with clamp
    if (!m_bInitializedSplitter || m_LeftPaneWidth <= 0.0f)
    {
        float defaultLeft = totalWidth * 0.5f; // center
        float maxLeft = totalWidth - m_MinRightPaneWidth;
        if (maxLeft < m_MinLeftPaneWidth)
            maxLeft = m_MinLeftPaneWidth;
        m_LeftPaneWidth  = ImClamp(defaultLeft, m_MinLeftPaneWidth, maxLeft);
        m_bInitializedSplitter = true;
    }

    // Now clamp every frame
    float maxLeft = totalWidth - m_MinRightPaneWidth;
    if (maxLeft < m_MinLeftPaneWidth)
        maxLeft = m_MinLeftPaneWidth;
    m_LeftPaneWidth = ImClamp(m_LeftPaneWidth, m_MinLeftPaneWidth, maxLeft);

    // Draw Left pane
    ImGui::BeginChild("AssetImporter_LeftPane", ImVec2(m_LeftPaneWidth, 0.0f), // fixed width, full height
                      false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawLeftPane(host, runtime);
    ImGui::EndChild();

    // Draw Splitter
    ImGui::SameLine();

    const float splitterWidth = 4.0f;  // clickable area
    ImGui::PushStyleColor(ImGuiCol_Button, 0); // invisible
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
    ImGui::Button("##HSplitter", ImVec2(splitterWidth, -1.0f));
    ImGui::PopStyleColor(3);

    // Make it draggable
    if (ImGui::IsItemActive())
    {
        float delta = ImGui::GetIO().MouseDelta.x;
        m_LeftPaneWidth = ImClamp(m_LeftPaneWidth + delta, m_MinLeftPaneWidth, maxLeft);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    // Optionally draw a 1px visible line in the middle of the splitter:
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetItemRectMin();
        ImVec2 q = ImGui::GetItemRectMax();
        float x = (p.x + q.x) * 0.5f;
        dl->AddLine(ImVec2(x, p.y), ImVec2(x, q.y),
                    ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
    }

    // Draw Right pane
    ImGui::SameLine();

    ImGui::BeginChild("AssetImporter_RightPane",ImVec2(0.0f, 0.0f), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawRightPane(host, runtime);
    ImGui::EndChild();

    // Tick the update
    UpdateFolderPickerResult(host, runtime);

    ImGui::End(); // Window end
}

void AssetImporterDialog::DrawLeftPane(EditorHost& host, EditorRuntime& runtime)
{
    // Top bar
    DrawLeftTopBar(host, runtime);

    ImGui::Separator();

    // Middle file list (scrollable)
    DrawLeftItemList(host, runtime);

    if (m_Items.empty())
    {
        ImGui::Separator();
    }

    // Bottom bar (stays at the bottom of the left column)
    DrawLeftBottomBar(host, runtime);
}

void AssetImporterDialog::DrawLeftTopBar(EditorHost& host, EditorRuntime& runtime)
{
    if (ImGui::Button("Add Files..."))
    {
        OnBrowseSourceFiles(host, runtime);
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("Sort by:");
}

void AssetImporterDialog::DrawLeftItemList(EditorHost& host, EditorRuntime& runtime)
{
    (void)host;
    (void)runtime;

    EnsureSelectionIsValid();

    ImGui::TextUnformatted("Pending Source Files List");
    ImGui::Spacing();

    const float bottomBarHeight = m_Items.empty() ? ImGui::GetFrameHeightWithSpacing() + 4.f :
    ImGui::GetFrameHeightWithSpacing();
    ImVec2 childSize(0.0f, -bottomBarHeight);

    if (!ImGui::BeginChild("PendingFileListRegion", childSize, !m_Items.empty(),
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::EndChild();
        return;
    }

    if (m_Items.empty())
    {
        ImGui::TextDisabled("No files selected.");
        ImGui::EndChild();
        return;
    }

        for (int i = 0; i < static_cast<int>(m_Items.size()); ++i)
        {
            const FPendingItem& item = m_Items[i];

            const bool bIsSelected =
                std::find(m_SelectedIndices.begin(), m_SelectedIndices.end(), i) != m_SelectedIndices.end();

            const bool bCtrlHeld = ImGui::GetIO().KeyCtrl;

            std::string label = item.sourceFilePath;

            if (ImGui::Selectable(label.c_str(), bIsSelected))
            {
                if (bCtrlHeld)
                {
                    auto it = std::find(m_SelectedIndices.begin(), m_SelectedIndices.end(), i);
                    if (it != m_SelectedIndices.end())
                        m_SelectedIndices.erase(it);
                    else
                        m_SelectedIndices.push_back(i);
                }
                else
                {
                    m_SelectedIndices.clear();
                    m_SelectedIndices.push_back(i);
                }
            }
        }

    ImGui::EndChild();
}

void AssetImporterDialog::DrawLeftBottomBar(EditorHost& host, EditorRuntime& runtime)
{
    (void)host;
    (void)runtime;

    const bool bHasSelection = !m_SelectedIndices.empty();
    const bool bHasItems = !m_Items.empty();

    // Delete (left side)
    if (!bHasSelection)
        ImGui::BeginDisabled();

    if (ImGui::Button("Delete"))
    {
        OnDeleteSelected();
    }

    if (!bHasSelection)
        ImGui::EndDisabled();

    // (Right-aligned Cancel / Done)

    // Reserve space for "Cancel" + spacing + "Done"
    const float buttonWidth = ImGui::CalcTextSize("Done").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float buttonWidth2 = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
    const float totalButtonsW = buttonWidth + buttonWidth2 + itemSpacing;

    // Move cursor to right side of the region
    float avail = ImGui::GetContentRegionAvail().x;
    if (avail > totalButtonsW)
        ImGui::SameLine(avail - totalButtonsW);

    // Cancel
    if (ImGui::Button("Cancel"))
    {
        OnCancel();
    }

    // Done
    ImGui::SameLine();

    if (!bHasItems)
        ImGui::BeginDisabled();

    if (ImGui::Button("Done"))
    {
        OnDone(host, runtime);
    }

    if (!bHasItems)
        ImGui::EndDisabled();
}

void AssetImporterDialog::DrawRightPane(EditorHost& host, EditorRuntime& runtime)
{
    (void)host;
    (void)runtime;

    EnsureSelectionIsValid();

    // Right pane scroll area – no border, horizontal scrollbar allowed if needed
    if (!ImGui::BeginChild("RightPaneRegion", ImVec2(0.0f, 0.0f), false,
                           ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::EndChild();
        return;
    }

    ImGui::TextUnformatted("Import Settings");
    ImGui::Spacing();

    if (m_SelectedIndices.empty())
    {
        ImGui::TextDisabled("Select one or more files to edit import settings.");
        ImGui::EndChild();
        return;
    }

    if (m_SelectedIndices.size() == 1)
    {
        FPendingItem& item = m_Items[m_SelectedIndices[0]];

        ImGui::Text("Source File:");
        ImGui::TextWrapped("%s", item.sourceFilePath.c_str());

        ImGui::Spacing();

        char buffer[512];
        std::memset(buffer, 0, sizeof(buffer));
        std::snprintf(buffer, sizeof(buffer), "%s", item.destinationVirtualFolder.c_str());

        ImGui::Text("Destination Folder: ");
        if (ImGui::InputText("##DestinationFolderInput", buffer, sizeof(buffer)))
        {
            item.destinationVirtualFolder = buffer;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            OnChooseDestinationForSelected(host, runtime);
        }

        ImGui::Checkbox("Overwrite Existing", &item.bOverwrite);
    }
    else
    {
        ImGui::Text("%zu files selected", m_SelectedIndices.size());
        ImGui::Spacing();

        // Multi-edit destination
        static char multiDestBuffer[512] = "";
        ImGui::InputText("Destination Folder", multiDestBuffer, sizeof(multiDestBuffer));

        if (ImGui::Button("Apply Destination To Selected"))
        {
            const std::string newDest = multiDestBuffer;
            for (int idx : m_SelectedIndices)
            {
                if (idx >= 0 && idx < static_cast<int>(m_Items.size()))
                    m_Items[idx].destinationVirtualFolder = newDest;
            }
        }

        static bool multiOverwriteValue = false;
        ImGui::Checkbox("Overwrite Existing", &multiOverwriteValue);

        if (ImGui::Button("Apply Overwrite To Selected"))
        {
            for (int idx : m_SelectedIndices)
            {
                if (idx >= 0 && idx < static_cast<int>(m_Items.size()))
                    m_Items[idx].bOverwrite = multiOverwriteValue;
            }
        }
    }

    ImGui::EndChild();
}

void AssetImporterDialog::OnBrowseSourceFiles(EditorHost& host, EditorRuntime& runtime)
{
    (void)host;

    auto& surfaceAPI = runtime.GetSurface();

    std::vector<std::string> files = surfaceAPI.OpenFileDialogMultiple("", "");

    for (const std::string& filePath : files)
    {
        FPendingItem item;
        item.sourceFilePath = filePath;
        item.destinationVirtualFolder = "/Project"; /* TODO: Must set it to a default later, and the default will be from
        the folder where the importer was asked to be opened by the AssetBrowser. */
        item.bOverwrite = false;

        m_Items.push_back(std::move(item));
    }

    if (!files.empty())
    {
        m_SelectedIndices.clear();
        const int lastIndex = static_cast<int>(m_Items.size()) - 1;
        if (lastIndex >= 0)
            m_SelectedIndices.push_back(lastIndex);
    }
}

void AssetImporterDialog::OnChooseDestinationForSelected(EditorHost& host, EditorRuntime& runtime)
{
    (void)runtime;

    EnsureSelectionIsValid();
    if (m_SelectedIndices.empty())
        return;

    // 1) Open the folder picker dialog
    auto* picker = host.GetDialogManager().RequestOpenDialog<FolderPickerDialog>();
    if (!picker)
        return; // safety

    // Configure for this use
    picker->SetTitle("Choose Destination Folder");

    std::string initialPath = "/Project";

    // We want to base it on the first selected item:
    int firstIdx = m_SelectedIndices.front();
    if (firstIdx >= 0 && firstIdx < static_cast<int>(m_Items.size()))
    {
        const std::string& dest = m_Items[firstIdx].destinationVirtualFolder;
        if (!dest.empty())
            initialPath = dest;
    }

    picker->SetInitialPath(initialPath);

    // 2) We do NOT block/wait here. The user will interact with the dialog.
    // The picker will set its result and close itself; we read it later.
}

void AssetImporterDialog::OnDeleteSelected()
{
    EnsureSelectionIsValid();
    if (m_SelectedIndices.empty())
        return;

    std::sort(m_SelectedIndices.begin(), m_SelectedIndices.end());
    m_SelectedIndices.erase(std::unique(m_SelectedIndices.begin(), m_SelectedIndices.end()), m_SelectedIndices.end());

    for (auto it = m_SelectedIndices.rbegin(); it != m_SelectedIndices.rend(); ++it)
    {
        const int idx = *it;
        if (idx >= 0 && idx < static_cast<int>(m_Items.size()))
            m_Items.erase(m_Items.begin() + idx);
    }

    m_SelectedIndices.clear();
}

void AssetImporterDialog::OnCancel()
{
    m_bIsOpen = false;
}

void AssetImporterDialog::OnDone(EditorHost &host, EditorRuntime &runtime)
{
    if (m_Items.empty())
    {
        m_bIsOpen = false;
        return;
    }

    BuildImportRequestsAndSubmit(host, runtime);
    m_bIsOpen = false;
}

void AssetImporterDialog::BuildImportRequestsAndSubmit(EditorHost& host, EditorRuntime& runtime)
{
    (void)host;

    std::vector<FAssetImportRequest> requests;
    requests.reserve(m_Items.size());

    for (const FPendingItem& item : m_Items)
    {
        FAssetImportRequest request;

        request.sourceFilePath = item.sourceFilePath;
        request.destinationVirtualFolder = item.destinationVirtualFolder;
        request.bOverwrite = item.bOverwrite;

        requests.push_back(std::move(request));
    }

    auto& fileAPI = runtime.GetFile();

    [[maybe_unused]] std::vector<FAssetImportResult> results = fileAPI.ImportAssetsBatch(requests);

    // Optional later:
    // - report failures
    // - show summary
    // - log imported assets
}

void AssetImporterDialog::EnsureSelectionIsValid()
{
    const int itemCount = static_cast<int>(m_Items.size());

    m_SelectedIndices.erase(
        std::remove_if(
            m_SelectedIndices.begin(),
            m_SelectedIndices.end(),
            [itemCount](int idx)
            {
                return idx < 0 || idx >= itemCount;
            }),
        m_SelectedIndices.end());

    std::sort(m_SelectedIndices.begin(), m_SelectedIndices.end());
    m_SelectedIndices.erase(
        std::unique(m_SelectedIndices.begin(), m_SelectedIndices.end()),
        m_SelectedIndices.end());
}

void AssetImporterDialog::UpdateFolderPickerResult(EditorHost& host, EditorRuntime& runtime)
{
    (void)runtime;

    auto* picker = host.GetDialogManager().FindDialogInstance<FolderPickerDialog>();
    if (!picker)
        return; // dialog not open / already destroyed

    // We only care after the user has made a choice and the dialog closed
    // or at least after they hit "Select".
    const auto& result = picker->GetResult();
    if (!result.bAccepted)
        return;

    const std::string& selectedPath = result.selectedPath;
    if (selectedPath.empty())
        return;

    // Apply to currently selected items
    EnsureSelectionIsValid();
    if (m_SelectedIndices.empty())
        return;

    for (int idx : m_SelectedIndices)
    {
        if (idx >= 0 && idx < static_cast<int>(m_Items.size()))
            m_Items[idx].destinationVirtualFolder = selectedPath;
    }
}