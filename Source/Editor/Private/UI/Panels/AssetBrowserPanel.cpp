//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserPanel.h"

#include <vector>
#include <cctype>

#include "imgui.h"
#include "imgui_internal.h"

#include "Controllers/Documents/FAssetBrowserDocument.h"
#include "Controllers/Inputs/FAssetBrowserPanelInput.h"
#include "Controllers/Outputs/FAssetBrowserOutput.h"

#include "Core/EditorHost.h"
#include "Subsystems/AssetBrowserSubsystem.h"
#include "UI/Dialogs/AssetImporterDialog.h"

namespace
{
    static bool DrawToolbarButton(const char* label, const char* tooltip = nullptr)
    {
        const bool pressed = ImGui::Button(label);
        if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("%s", tooltip);
        return pressed;
    }

    static bool Splitter(bool vertical, float thickness,
                         float* size0, float* size1,
                         float min_size0, float min_size1)
    {
        ImVec2 backup_pos = ImGui::GetCursorPos();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::GetStyleColorVec4(ImGuiCol_Separator));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_SeparatorHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::GetStyleColorVec4(ImGuiCol_SeparatorActive));

        ImGui::Button("##splitter", vertical ? ImVec2(thickness, -1.0f) : ImVec2(-1.0f, thickness));

        ImGui::PopStyleColor(3);

        ImGui::SetItemAllowOverlap();

        bool changed = false;
        if (ImGui::IsItemActive())
        {
            float delta = vertical ? ImGui::GetIO().MouseDelta.x : ImGui::GetIO().MouseDelta.y;

            if (delta < 0.0f && *size0 + delta < min_size0) delta = min_size0 - *size0;
            if (delta > 0.0f && *size1 - delta < min_size1) delta = *size1 - min_size1;

            *size0 += delta;
            *size1 -= delta;
            changed = (delta != 0.0f);
        }

        ImGui::SetCursorPos(backup_pos);
        return changed;
    }

    static void NormalizePathSlashes(std::string& s)
    {
        for (char& c : s)
            if (c == '\\') c = '/';
    }

    static std::vector<std::string> SplitPath(const std::string& path)
    {
        std::vector<std::string> parts;
        std::string cur;

        for (char c : path)
        {
            if (c == '/')
            {
                if (!cur.empty()) parts.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }

        if (!cur.empty())
            parts.push_back(cur);

        return parts;
    }

    static std::string JoinPathPrefix(const std::vector<std::string>& parts, int countInclusive)
    {
        std::string out;
        for (int i = 0; i <= countInclusive; ++i)
        {
            out += parts[i];
            if (i != countInclusive) out += "/";
        }
        return out;
    }

    static bool CaseInsensitiveContains(const std::string& text, const char* queryCStr)
    {
        if (!queryCStr || queryCStr[0] == '\0')
            return true;

        std::string q(queryCStr);
        std::string t(text);

        auto lower = [](unsigned char ch) { return (char)std::tolower(ch); };
        for (char& c : q) c = lower((unsigned char)c);
        for (char& c : t) c = lower((unsigned char)c);

        return t.find(q) != std::string::npos;
    }
} // namespace

void AssetBrowserPanel::OnDestroy(EditorHost& /*host*/)
{
    // Clear any cached UI state if desired
    m_SelectedLeftPath.clear();
    m_SelectedAssetID = "";
    m_SearchBuf[0] = '\0';
}

void AssetBrowserPanel::Draw(EditorHost& host)
{
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        return;
    }

    // 1) Get subsystem
    AssetBrowserSubsystem& subsystem = host.GetSubsystem<AssetBrowserSubsystem>();

    // 2) Retrieve last output snapshot for this panel key
    const FAssetBrowserOutput* output = subsystem.GetOutput(GetPanelKey());

    // 3) Build input (panel -> controller commands)
    FAssetBrowserPanelInput input{};
    input.panelKey = GetPanelKey();

    input.bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    input.bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    ImGuiIO& io = ImGui::GetIO();
    input.bCtrl  = io.KeyCtrl;
    input.bShift = io.KeyShift;
    input.bAlt   = io.KeyAlt;
    input.bSuper = io.KeySuper;

    if (!output || !output->bValid)
    {
        ImGui::TextDisabled("Asset Browser: no output yet.");
        ImGui::TextDisabled("PanelKey: %s", GetPanelKey());
        ImGui::End();

        // Still submit input so controller can bootstrap on first frame
        subsystem.SubmitInput(input);
        return;
    }

    const FAssetBrowserDocument& doc = output->document;

    input.bNavigateToPath = false;
    input.bNavigateUp     = false;
    input.bNavigateHome   = false;
    input.bForceRefresh   = false;

    // ---------------- Toolbar ----------------

    if (DrawToolbarButton("Save All", "Save all the edited assets"))
    {

    }

    ImGui::SameLine();
    if (DrawToolbarButton("Import", "Import Assets From System Files"))
    {
       host.GetDialogManager().OpenDialog<AssetImporterDialog>();
    }

    ImGuiStyle& style = ImGui::GetStyle();
    const float exploreButtonsWidth = 154.0f;
    const float spacing = style.ItemSpacing.x;

    float available = ImGui::GetContentRegionAvail().x;
    float searchWidth = available - exploreButtonsWidth - spacing;

    if (searchWidth < 140.0f)
        searchWidth = 140.0f;

    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputTextWithHint("##AssetSearch", "Search assets...", m_SearchBuf, sizeof(m_SearchBuf));

    ImGui::SameLine();
    if (DrawToolbarButton("Home", "Go to assets root"))
        input.bNavigateHome = true;

    ImGui::SameLine();
    if (DrawToolbarButton("Up", "Go to parent directory"))
        input.bNavigateUp = true;

    ImGui::SameLine();
    if (DrawToolbarButton("Refresh", "Refresh current folder"))
        input.bForceRefresh = true;

    // ---------------- Breadcrumbs ----------------
    {
        std::string path = doc.currentPath;
        NormalizePathSlashes(path);
        const auto parts = SplitPath(path);

        ImGui::Separator();

        if (parts.empty())
        {
            ImGui::TextUnformatted("(root)");
        }
        else
        {
            for (int i = 0; i < (int)parts.size(); ++i)
            {
                ImGui::PushID(i);

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
                const bool clicked = ImGui::SmallButton(parts[i].c_str());
                ImGui::PopStyleVar();

                if (clicked)
                {
                    input.bNavigateToPath = true;
                    input.navigateToPath = JoinPathPrefix(parts, i);
                }

                ImGui::PopID();

                if (i != (int)parts.size() - 1)
                {
                    ImGui::SameLine();
                    ImGui::TextUnformatted("/");
                    ImGui::SameLine();
                }
            }
        }

        ImGui::Separator();
    }

    // ---------------- Split layout ----------------
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Clamp widths
    if (m_LeftPaneWidth < m_MinLeftPaneWidth)
        m_LeftPaneWidth = m_MinLeftPaneWidth;

    float splitterThickness = 6.0f;
    float rightWidth = avail.x - m_LeftPaneWidth - splitterThickness;
    if (rightWidth < m_MinRightPaneWidth)
    {
        rightWidth = m_MinRightPaneWidth;
        m_LeftPaneWidth = avail.x - rightWidth - splitterThickness;
        if (m_LeftPaneWidth < m_MinLeftPaneWidth)
            m_LeftPaneWidth = m_MinLeftPaneWidth;
    }

    // Left pane: folders list
    ImGui::BeginChild("##AssetBrowserLeft", ImVec2(m_LeftPaneWidth, 0.0f), true);
    {
        ImGui::TextUnformatted("Folders");
        ImGui::Separator();

        for (const FAssetBrowserDirectory& dir : doc.directories)
        {
            ImGui::PushID(dir.virtualPath.c_str());

            const bool selected = (m_SelectedLeftPath == dir.virtualPath);
            if (ImGui::Selectable(dir.name.c_str(), selected))
                m_SelectedLeftPath = dir.virtualPath;

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                input.bNavigateToPath = true;
                input.navigateToPath = dir.virtualPath;
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Splitter (draggable)
    {
        ImGui::BeginGroup();
        float left = m_LeftPaneWidth;
        float right = avail.x - left - splitterThickness;

        // draw splitter as a child-sized button
        ImGui::InvisibleButton("##splitter_hit", ImVec2(splitterThickness, avail.y));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - splitterThickness); // keep position

        // Use our splitter logic with a real button so it can be dragged.
        // We'll render the button at same place:
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - avail.y));
        // simpler: just call Splitter which creates the button:
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 0.0f);
        Splitter(true, splitterThickness, &left, &right, m_MinLeftPaneWidth, m_MinRightPaneWidth);

        m_LeftPaneWidth = left;
        ImGui::EndGroup();
    }

    ImGui::SameLine();

    // Right pane: grid
    ImGui::BeginChild("##AssetBrowserRight", ImVec2(0.0f, 0.0f), true);
    {
        ImGui::TextUnformatted("Assets");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::SliderFloat("##IconSize", &m_IconSize, 32.0f, 128.0f, "Icon %.0f");

        ImGui::Separator();

        const float cellW = m_IconSize + (m_GridPadding * 2.0f) + 40.0f;
        const float regionW = ImGui::GetContentRegionAvail().x;

        int columns = (int)(regionW / cellW);
        if (columns < 1) columns = 1;

        if (ImGui::BeginTable("##AssetGrid", columns, ImGuiTableFlags_SizingFixedFit))
        {
            // (A) Directories in grid (optional)
            if (m_ShowFoldersInGrid)
            {
                for (const FAssetBrowserDirectory& dir : doc.directories)
                {
                    if (!CaseInsensitiveContains(dir.name, m_SearchBuf))
                        continue;

                    ImGui::TableNextColumn();
                    ImGui::PushID(dir.virtualPath.c_str());

                    const bool selected = (m_SelectedLeftPath == dir.virtualPath);

                    // Tile rect
                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImVec2 tileSize(cellW - m_GridPadding, m_IconSize + 40.0f);
                    ImVec2 p1(p0.x + tileSize.x, p0.y + tileSize.y);

                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    const ImU32 bg     = selected ? IM_COL32(70, 110, 180, 90)  : IM_COL32(255, 255, 255, 20);
                    const ImU32 border = selected ? IM_COL32(120, 170, 255, 180): IM_COL32(255, 255, 255, 40);

                    dl->AddRectFilled(p0, p1, bg, 6.0f);
                    dl->AddRect(p0, p1, border, 6.0f);

                    ImGui::InvisibleButton("##dir_tile", tileSize);

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                        m_SelectedLeftPath = dir.virtualPath;

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        input.bNavigateToPath = true;
                        input.navigateToPath = dir.virtualPath;
                    }

                    // Folder icon
                    ImVec2 iconP0(p0.x + m_GridPadding, p0.y + m_GridPadding);
                    ImVec2 iconP1(iconP0.x + m_IconSize, iconP0.y + m_IconSize);
                    dl->AddRectFilled(iconP0, iconP1, IM_COL32(240, 200, 60, 220), 8.0f);

                    // Label
                    ImVec2 textPos(p0.x + m_GridPadding, iconP1.y + 6.0f);
                    dl->AddText(textPos, IM_COL32(230, 230, 230, 255), dir.name.c_str());

                    ImGui::PopID();
                }
            }

            // (B) Assets in grid
            if (m_ShowAssetsInGrid)
            {
                for (const FAssetBrowserAsset& a : doc.assets)
                {
                    if (!CaseInsensitiveContains(a.name, m_SearchBuf))
                        continue;

                    ImGui::TableNextColumn();
                    ImGui::PushID(a.assetID.c_str()); // stable, unique

                    const bool selected = (m_SelectedAssetID == a.assetID);

                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImVec2 tileSize(cellW - m_GridPadding, m_IconSize + 40.0f);
                    ImVec2 p1(p0.x + tileSize.x, p0.y + tileSize.y);

                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    const ImU32 bg     = selected ? IM_COL32(70, 110, 180, 90)  : IM_COL32(255, 255, 255, 20);
                    const ImU32 border = selected ? IM_COL32(120, 170, 255, 180): IM_COL32(255, 255, 255, 40);

                    dl->AddRectFilled(p0, p1, bg, 6.0f);
                    dl->AddRect(p0, p1, border, 6.0f);

                    ImGui::InvisibleButton("##asset_tile", tileSize);

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        m_SelectedAssetID = a.assetID;
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        // “open asset” is triggered here.
                        // input.bOpenAsset = true;
                        // input.openAssetID = a.assetID;
                    }

                    // Asset icon placeholder (could be colored by type/domain)
                    ImVec2 iconP0(p0.x + m_GridPadding, p0.y + m_GridPadding);
                    ImVec2 iconP1(iconP0.x + m_IconSize, iconP0.y + m_IconSize);
                    dl->AddRectFilled(iconP0, iconP1, IM_COL32(160, 200, 255, 220), 8.0f);

                    // Label
                    ImVec2 textPos(p0.x + m_GridPadding, iconP1.y + 6.0f);
                    dl->AddText(textPos, IM_COL32(230, 230, 230, 255), a.name.c_str());

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    ImGui::End();

    // 5) Submit input back
    subsystem.SubmitInput(input);
}
