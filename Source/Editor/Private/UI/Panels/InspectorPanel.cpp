//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InspectorPanel.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cfloat>
#include <unordered_map>

#include "imgui.h"
#include "imgui_internal.h"

#include "Core/EditorHost.h"
#include "Subsystems/InspectorSubsystem.h"
#include "Controllers/Outputs/FInspectorOutput.h"

// ------------------------------- small UI helpers -------------------------------

static bool BeginBox(const char* id, float height = 0.0f)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 4));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(70, 70, 70, 255));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(34, 34, 34, 140));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(72, 72, 72, 255));
    const bool ok = ImGui::BeginChild(id, ImVec2(0, height), ImGuiChildFlags_Border, ImGuiWindowFlags_None);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
    return ok;
}

static void EndBox()
{
    ImGui::EndChild();
}

static bool DrawPlainSectionToggle(const char* id, const char* label, bool open, bool boldText = false)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window)
        return open;

    ImGui::PushID(id);

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float fullW  = ImGui::GetContentRegionAvail().x;
    const float h      = ImGui::GetTextLineHeight();

    // Clickable row (but visually plain)
    ImGui::InvisibleButton("##SectionToggle", ImVec2(fullW, h));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    if (clicked)
        open = !open;

    // Optional subtle hover feedback (very light)
    if (hovered)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(start, ImVec2(start.x + fullW, start.y + h), IM_COL32(255, 255, 255, 10));
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Arrow
    const float arrowSize = ImGui::GetFontSize() * 0.70f;
    ImVec2 arrowPos(start.x, start.y + (h * 0.2f));
    ImGui::RenderArrow(dl, arrowPos, ImGui::GetColorU32(ImGuiCol_Text), open ? ImGuiDir_Down : ImGuiDir_Right, 0.7f);

    // Text (aligned next to arrow)
    const float textX = start.x + arrowSize + 6.0f;
    const float textY = start.y;
    dl->AddText(ImVec2(textX, textY), ImGui::GetColorU32(ImGuiCol_Text), label);
    if (boldText) dl->AddText(ImVec2(textX + 1.0f, textY),ImGui::GetColorU32(ImGuiCol_Text) , label);

    ImGui::PopID();
    return open;
}

static void DrawVerticalColumnSplitter(float& inOutLabelWidth, float minW, float maxW)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window)
        return;

    // This assumes we are INSIDE the properties child (BeginBox("##InspectorPropsBox"...))
    const ImVec2 winPos  = ImGui::GetWindowPos();   // props child top-left (screen space)
    const ImVec2 winSize = ImGui::GetWindowSize();  // props child size

    // Clamp current width
    inOutLabelWidth = std::clamp(inOutLabelWidth, minW, maxW);

    // Split line X in screen space (relative to child content left)
    // Window->WorkRect.Min.x for a more precise content-left anchor.
    const float contentLeftX = window->WorkRect.Min.x;
    const float splitX       = contentLeftX + inOutLabelWidth;

    // Full-height hit zone
    const float y0 = winPos.y;
    const float y1 = winPos.y + winSize.y;

    // Wider invisible hit target, thin visible line
    const float hitHalfW = 5.0f;

    // Preserve cursor (so no layout is consumed)
    const ImVec2 savedCursor = ImGui::GetCursorScreenPos();

    ImGui::SetCursorScreenPos(ImVec2(splitX - hitHalfW, y0));
    ImGui::InvisibleButton("##PropsColumnSplitterFull", ImVec2(hitHalfW * 2.0f, winSize.y));

    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();

    if (hovered || active)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    if (active)
    {
        inOutLabelWidth += ImGui::GetIO().MouseDelta.x;
        inOutLabelWidth = std::clamp(inOutLabelWidth, minW, maxW);
    }

    // Only visible on drag
    if (/*hover | */active)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 col = active
            ? IM_COL32(150, 150, 150, 255)
            : IM_COL32(110, 110, 110, 220);

        dl->AddLine(ImVec2(splitX, y0), ImVec2(splitX, y1), col, 1.0f);
    }

    // Restore cursor => zero layout impact
    ImGui::SetCursorScreenPos(savedCursor);
}

static bool BeginPropsTable(const char* id, float labelColumnWidth)
{
    ImGuiTableFlags flags =
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_PadOuterX |
        ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable(id, 2, flags))
        return false;

    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    return true;
}

static bool BeginCategoryHeaderUnique(const char* name, uint64_t stableID, bool defaultOpen)
{
    // Important:
    // - category names can repeat across different objects
    // - we still need unique ImGui IDs
    // We give the header a stable PushID scope.
    ImGui::PushID((int)stableID);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    if (defaultOpen)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool open = ImGui::CollapsingHeader(name, flags);
    ImGui::PopID();
    return open;
}

static void PropLabel(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();

    // Cell bounds (screen space)
    const ImVec2 cellMin = ImGui::GetCursorScreenPos();
    const float  cellW   = ImGui::GetContentRegionAvail().x;
    const float  textH   = ImGui::GetTextLineHeight();

    // Render clipped text inside the label cell
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);

    const ImVec2 textMin = cellMin;
    const ImVec2 textMax = ImVec2(cellMin.x + cellW, cellMin.y + textH);

    dl->PushClipRect(textMin, textMax, true);
    dl->AddText(textMin, col, label ? label : "");
    dl->PopClipRect();

    // Invisible item for hover/tooltip on truncated labels
    ImGui::InvisibleButton("##PropLabelHit", ImVec2(cellW, textH));

    if (ImGui::IsItemHovered())
    {
        const ImVec2 fullText = ImGui::CalcTextSize(label ? label : "");
        if (fullText.x > cellW)
            ImGui::SetTooltip("%s", label ? label : "");
    }

    ImGui::TableSetColumnIndex(1);
}

static float CalcComponentWidth(int componentCount)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float avail = ImGui::GetContentRegionAvail().x;
    const float spacing = style.ItemSpacing.x;
    const float totalSpacing = spacing * (componentCount - 1);
    const float w = (avail - totalSpacing) / (float)componentCount;
    return (w > 1.0f) ? w : 1.0f;
}

static bool ContainsCaseInsensitive(const std::string& hay, const char* needle)
{
    if (!needle || needle[0] == 0) return true;

    auto lower = [](unsigned char c) { return (char)std::tolower(c); };

    std::string h = hay;
    std::string n = needle;

    for (char& c : h) c = lower((unsigned char)c);
    for (char& c : n) c = lower((unsigned char)c);

    return h.find(n) != std::string::npos;
}

static void PushEdit(FInspectorPanelInput& input, const FInspectorRow& row, const REVariant& v)
{
    FInspectorEditCommand cmd;
    cmd.handle = row.write;
    cmd.value  = v;
    input.edits.push_back(std::move(cmd));
}

// ------------------------------- InspectorPanel -------------------------------

void InspectorPanel::OnDestroy(EditorHost& /*host*/)
{
    m_CollapsedCategoryKeys.clear();
    m_StringEdits.clear();
    m_SelectedTargetID = 0;
    std::memset(m_SearchBuf, 0, sizeof(m_SearchBuf));
}

bool InspectorPanel::MatchesRowSearch(const FInspectorRow& row, const char* search)
{
    if (!search || search[0] == 0) return true;
    return ContainsCaseInsensitive(row.label, search) || ContainsCaseInsensitive(row.rawName, search);
}

const FInspectorTarget* InspectorPanel::FindTargetByID(const FInspectorDocument& doc, uint64_t targetID)
{
    for (const auto& t : doc.targets)
        if (t.targetID == targetID)
            return &t;
    return nullptr;
}

const FInspectorTarget* InspectorPanel::FindActorTarget(const FInspectorDocument& doc)
{
    for (const auto& t : doc.targets)
        if (t.group == EInspectorTargetGroup::Actor)
            return &t;
    return nullptr;
}

void InspectorPanel::BuildSceneComponentChildrenMap(
    const FInspectorDocument& doc,
    std::unordered_map<uint64_t, std::vector<const FInspectorTarget*>>& outChildren)
{
    outChildren.clear();
    outChildren.reserve(doc.targets.size());

    for (const auto& t : doc.targets)
    {
        if (t.group != EInspectorTargetGroup::SceneComponent)
            continue;

        outChildren[t.parentTargetID].push_back(&t);
    }

    // Keep deterministic-ish ordering for traversal
    for (auto& [pid, vec] : outChildren)
    {
        std::sort(vec.begin(), vec.end(), [](const FInspectorTarget* a, const FInspectorTarget* b)
        {
            if (a->depth != b->depth) return a->depth < b->depth;
            return a->targetID < b->targetID;
        });
    }
}

void InspectorPanel::DrawActorHeader(const FInspectorDocument& doc, FInspectorPanelInput& input)
{
    const FInspectorTarget* actor = FindActorTarget(doc);
    if (!actor)
        return;

    // Build a clean display name from listLabel (strip " (Instance)")
    std::string sourceName = actor->listLabel;
    const std::string suffix = " (Instance)";
    if (sourceName.size() >= suffix.size())
    {
        const size_t pos = sourceName.rfind(suffix);
        if (pos != std::string::npos && pos + suffix.size() == sourceName.size())
            sourceName.erase(pos);
    }

    // Reset local cache when actor changes
    if (m_ActorNameTargetID != actor->targetID)
    {
        m_ActorNameTargetID = actor->targetID;
        m_ActorNameEdit = sourceName;
    }

    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", m_ActorNameEdit.c_str());

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 4));

    // Option A (current behavior): live change
    bool changed = ImGui::InputTextWithHint("##ActorName", "Actor Name", buf, sizeof(buf));

    // Option B (recommended): commit only after edit finished
    // bool changed = ImGui::InputTextWithHint("##ActorName", "Actor Name", buf, sizeof(buf));
    const bool commit = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::PopStyleVar();

    if (changed)
        m_ActorNameEdit = buf;

    // Keep local cache synced if backend changed externally and we're not editing
    if (!ImGui::IsItemActive() && !changed && !commit)
    m_ActorNameEdit = sourceName;

    // Commit only once after user finishes editing (better UX)
    if (!commit)
    {
        ImGui::Spacing();
        return;
    }

    // Trim
    auto trim = [](std::string& s)
    {
        auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };

        while (!s.empty() && is_ws((unsigned char)s.front()))
            s.erase(s.begin());
        while (!s.empty() && is_ws((unsigned char)s.back()))
            s.pop_back();
    };

    std::string newName = m_ActorNameEdit;
    trim(newName);

    if (newName.empty())
    {
        ImGui::Spacing();
        return;
    }

    // If unchanged, don't send command
    if (newName == sourceName)
    {
        ImGui::Spacing();
        return;
    }

    // Reuse any actor row's write handle to get providerID + contextRuntimeID.
    const FInspectorWriteHandle* baseWrite = nullptr;
    for (const auto& cat : actor->categories)
    {
        for (const auto& row : cat.rows)
        {
            baseWrite = &row.write;
            break;
        }
        if (baseWrite) break;
    }

    if (baseWrite)
    {
        FInspectorEditCommand cmd;
        cmd.handle = *baseWrite; // copy provider/context/kind defaults

        // Override routing to our manual actor command
        cmd.handle.kind              = EInspectorTargetKind::ObjectUUID;
        cmd.handle.primaryID         = actor->objectUUID;
        cmd.handle.declaringTypeName = "__ManualActor";
        cmd.handle.propName          = "__ActorName";

        cmd.value = {};
        cmd.value.tag = REValueTag::String;
        cmd.value.s   = newName;

        input.edits.push_back(std::move(cmd));
    }

    ImGui::Spacing();
}

void InspectorPanel::Draw(EditorHost& host)
{
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        return;
    }

    auto& insp = host.GetSubsystem<InspectorSubsystem>();
    const FInspectorOutput* out = insp.GetOutput(GetPanelKey());

    FInspectorPanelInput input;
    input.panelKey = GetPanelKey();

    input.bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    input.bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGuiIO& io = ImGui::GetIO();
    input.bCtrl  = io.KeyCtrl;
    input.bShift = io.KeyShift;
    input.bAlt   = io.KeyAlt;
    input.bSuper = io.KeySuper;

    if (!out)
    {
        ImGui::TextDisabled("Inspector: no output.");
        ImGui::End();
        insp.SubmitInput(input);
        return;
    }

    if (!out->bHasSelection)
    {
        ImGui::TextDisabled("%s", out->statusText ? out->statusText : "Inspector: nothing selected.");
        ImGui::End();
        insp.SubmitInput(input);
        return;
    }

    if (!out->bHasDocument || !out->document)
    {
        ImGui::TextDisabled("%s", out->statusText ? out->statusText : "Inspector: no document.");
        ImGui::End();
        insp.SubmitInput(input);
        return;
    }

    const FInspectorDocument& doc = *out->document;

    // Default selection: Actor
    if (m_SelectedTargetID == 0)
    {
        if (const FInspectorTarget* actorT = FindActorTarget(doc))
            m_SelectedTargetID = actorT->targetID;
        else if (!doc.targets.empty())
            m_SelectedTargetID = doc.targets[0].targetID;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);

    // Actor Header Strip
    DrawActorHeader(doc, input);

    // Components list (selection)
    DrawComponentSection(doc);

    // Properties Search bar
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 1.0f)); // lower Y = shorter input height
    ImGui::SetNextItemWidth(-FLT_MIN); // fill available panel width
    ImGui::InputTextWithHint("##InspectorSearch", "Search", m_SearchBuf, IM_ARRAYSIZE(m_SearchBuf));
    ImGui::PopStyleVar();

    ImGui::Spacing();

    // Properties (header rows + categories)
    DrawPropertiesForSelection(doc, input);

    if (out->statusText && out->statusText[0] != '\0')
    {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", out->statusText);
    }
    ImGui::PopStyleVar();

    ImGui::End();
    insp.SubmitInput(input);
}

void InspectorPanel::DrawComponentSection(const FInspectorDocument& doc)
{
    m_bComponentSectionOpen = DrawPlainSectionToggle("InspectorComponentSection", "Component Section", m_bComponentSectionOpen,
        true);

    ImGui::Spacing();

    if (!m_bComponentSectionOpen)
        return;


    if (!BeginBox("##InspectorComponentsBox", m_ComponentSectionHeight))
    {
        EndBox();
        return;
    }

    auto drawSelectable = [&](const FInspectorTarget& t, float indentPx)
    {
        if (indentPx > 0.0f) ImGui::Indent(indentPx);

        const bool selected = (m_SelectedTargetID == t.targetID);
        if (ImGui::Selectable(t.listLabel.c_str(), selected, ImGuiSelectableFlags_SpanAvailWidth))
            m_SelectedTargetID = t.targetID;

        if (indentPx > 0.0f) ImGui::Unindent(indentPx);
    };

    // Actor first
    for (const auto& t : doc.targets)
        if (t.group == EInspectorTargetGroup::Actor)
            drawSelectable(t, 0.0f);

    // Scene tree
    for (const auto& t : doc.targets)
        if (t.group == EInspectorTargetGroup::SceneComponent)
            drawSelectable(t, (float)t.depth * 16.0f);

    // Actor components
    const bool hasActorComps = std::any_of(doc.targets.begin(), doc.targets.end(),
        [](const FInspectorTarget& t) { return t.group == EInspectorTargetGroup::ActorComponent; });

    if (hasActorComps)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (const auto& t : doc.targets)
            if (t.group == EInspectorTargetGroup::ActorComponent)
                drawSelectable(t, 0.0f);
    }

    EndBox();

    // Resize handle (bottom edge)
    ImGui::PushID("InspectorComponentsResize");

    const float handleHCollision = 6.0f;
    const float handleHDraw = 6.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // full-width invisible handle
    ImGui::InvisibleButton("##ResizeHandle", ImVec2(avail.x, handleHCollision));

    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();

    if (hovered || active)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

    // Optional visual line/strip
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = active ? IM_COL32(140, 140, 140, 255)
                       : hovered ? IM_COL32(110, 110, 110, 255)
                                 : IM_COL32(80, 80, 80, 180);

    dl->AddRectFilled(p, ImVec2(p.x + avail.x, p.y + handleHDraw), col);

    // Drag logic
    if (active)
    {
        m_ComponentSectionHeight += ImGui::GetIO().MouseDelta.y;

        // clamp so it doesn't collapse or eat the whole panel
        const float minH = 80.0f;
        const float maxH = 500.0f; // or compute dynamically
        m_ComponentSectionHeight = std::clamp(m_ComponentSectionHeight, minH, maxH);
    }

    ImGui::PopID();
}

void InspectorPanel::DrawPropertiesForSelection(const FInspectorDocument& doc, FInspectorPanelInput& input)
{
    const FInspectorTarget* selected = FindTargetByID(doc, m_SelectedTargetID);
    if (!selected)
    {
        selected = FindActorTarget(doc);
        if (!selected && !doc.targets.empty())
            selected = &doc.targets[0];

        if (selected)
            m_SelectedTargetID = selected->targetID;
    }

    if (!selected)
        return;

    if (!BeginBox("##InspectorPropsBox", 0.0f))
    {
        EndBox();
        return;
    }

    const float kMinLabelW = 50.0f;
    const float kMaxLabelW = 210.0f;

    DrawVerticalColumnSplitter(m_PropertyLabelColumnWidth, kMinLabelW, kMaxLabelW);

    // Build list of targets we want to merge (in strict draw order)
    std::vector<const FInspectorTarget*> targetsInOrder;
    targetsInOrder.reserve(doc.targets.size());

    if (selected->group == EInspectorTargetGroup::Actor)
    {
        // Actor selection: show everything linearly (NO component headers, NO collapsibles)
        // Order: Actor target, then SceneComponents (doc order), then ActorComponents (doc order)

        // Actor first
        targetsInOrder.push_back(selected);

        for (const auto& t : doc.targets)
            if (t.group == EInspectorTargetGroup::SceneComponent)
                targetsInOrder.push_back(&t);

        for (const auto& t : doc.targets)
            if (t.group == EInspectorTargetGroup::ActorComponent)
                targetsInOrder.push_back(&t);
    }
    else if (selected->group == EInspectorTargetGroup::SceneComponent)
    {
        // SceneComponent selection: include selected + its subtree, but still draw ONLY categories (merged).
        std::unordered_map<uint64_t, std::vector<const FInspectorTarget*>> children;
        BuildSceneComponentChildrenMap(doc, children);

        // DFS preorder
        std::function<void(const FInspectorTarget*)> visit = [&](const FInspectorTarget* node)
        {
            if (!node) return;
            targetsInOrder.push_back(node);

            auto it = children.find(node->targetID);
            if (it == children.end()) return;

            for (const FInspectorTarget* c : it->second)
                visit(c);
        };

        visit(selected);
    }
    else
    {
        // ActorComponent selection: only itself
        targetsInOrder.push_back(selected);
    }

    // Draw top header rows first (e.g. Actor Transform essentials)
    {
        std::vector<const FInspectorRow*> headerRows;
        headerRows.reserve(8);

        for (const FInspectorTarget* t : targetsInOrder)
        {
            if (!t) continue;

            for (const auto& cat : t->categories)
            {
                for (const auto& row : cat.rows)
                {
                    if (row.presentation != EInspectorRowPresentation::Header)
                        continue;

                    if (!MatchesRowSearch(row, m_SearchBuf))
                        continue;

                    headerRows.push_back(&row);
                }
            }
        }

        DrawHeaderRows(headerRows, input);
    }

    // Then draw normal reflected categories
    DrawFilteredMergedCategories(targetsInOrder, input);

    EndBox();
}

void InspectorPanel::DrawHeaderRows(const std::vector<const FInspectorRow*>& rows, FInspectorPanelInput& input)
{
    if (rows.empty())
        return;

    // Dedicated "Essentials" strip
    if (BeginPropsTable("##InspectorHeaderRowsTable", m_PropertyLabelColumnWidth))
    {
        for (const FInspectorRow* pr : rows)
        {
            if (!pr) continue;
            DrawRow(*pr, input);
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
}

void InspectorPanel::DrawFilteredMergedCategories(
    const std::vector<const FInspectorTarget*>& targetsInOrder,
    FInspectorPanelInput& input)
{
    // Merge rule:
    // - categories with the same name become ONE category in UI
    // - rows keep their own stable IDs (row.rowID) so ImGui won’t conflict
    // - search can match either row names OR category names
    struct FMergedCategory
    {
        std::string name;
        uint64_t id = 0; // use categoryID from first encounter (already hashed in provider)
        std::vector<const FInspectorRow*> rows;
    };

    std::vector<FMergedCategory> merged;
    merged.reserve(32);

    std::unordered_map<std::string, size_t> idxOf;
    idxOf.reserve(32);

    for (const FInspectorTarget* t : targetsInOrder)
    {
        if (!t) continue;

        for (const auto& cat : t->categories)
        {
            // Skip synthetic category entirely (headers are drawn separately)
            if (cat.name == "__Essentials")
                continue;

            size_t idx = (size_t)-1;

            auto it = idxOf.find(cat.name);
            if (it == idxOf.end())
            {
                idx = merged.size();
                idxOf[cat.name] = idx;

                FMergedCategory mc;
                mc.name = cat.name;
                mc.id   = cat.categoryID; // stable enough; consistent across frames for same category name
                merged.push_back(std::move(mc));
            }
            else
            {
                idx = it->second;
            }

            // Category search behavior:
            // - If category name matches search => include all normal rows
            // - Else include only rows that match search
            const bool categoryMatched = ContainsCaseInsensitive(cat.name, m_SearchBuf);

            auto& dst = merged[idx].rows;
            for (const auto& row : cat.rows)
            {
                if (row.presentation == EInspectorRowPresentation::Header)
                    continue; // headers are drawn separately in top block

                if (!categoryMatched && !MatchesRowSearch(row, m_SearchBuf))
                    continue;

                dst.push_back(&row);
            }
        }
    }

    // Draw only categories that actually have visible rows after search
    for (const auto& mc : merged)
    {
        if (mc.rows.empty())
            continue;

        DrawCategorySection(mc.id, mc.name, mc.rows, input);
    }
}

void InspectorPanel::DrawCategorySection(uint64_t categoryID, const std::string& categoryName,
                                       const std::vector<const FInspectorRow*>& rows,
                                       FInspectorPanelInput& input)
{
    // Do not show the synthetic category used to transport header rows
    if (categoryName == "__Essentials")
        return;

    // collapse state by categoryID (since categories are merged globally on the page)
    // Default behavior is OPEN unless explicitly collapsed by the user.
    const uint64_t key = categoryID;
    const bool isCollapsed = (m_CollapsedCategoryKeys.find(key) != m_CollapsedCategoryKeys.end());

    // On first appearance, categories open by default.
    // Afterwards, ImGui keeps state and we update our collapsed set below.
    ImGui::SetNextItemOpen(!isCollapsed, ImGuiCond_Once);

    const bool open = BeginCategoryHeaderUnique(categoryName.c_str(), categoryID, /*defaultOpen*/true);

    if (open) m_CollapsedCategoryKeys.erase(key);
    else      m_CollapsedCategoryKeys.insert(key);

    if (!open)
        return;

    ImGui::PushID((int)categoryID);

    if (BeginPropsTable("##PropsTable", m_PropertyLabelColumnWidth))
    {
        for (const FInspectorRow* pr : rows)
        {
            if (!pr) continue;
            DrawRow(*pr, input);
        }

        ImGui::EndTable();
    }

    ImGui::PopID();
}

void InspectorPanel::DrawRow(const FInspectorRow& row, FInspectorPanelInput& input)
{
    ImGui::PushID((int)row.rowID);

    PropLabel(row.label.c_str());

    if (row.bReadOnly)
        ImGui::BeginDisabled(true);

    switch (row.widget)
    {
        case EInspectorWidget::Bool:
        {
            bool v = (row.value.tag == REValueTag::Bool) ? row.value.b : false;
            if (ImGui::Checkbox("##v", &v))
            {
                REVariant nv{};
                nv.tag = REValueTag::Bool;
                nv.b = v;
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Int:
        {
            int v = 0;
            if (row.value.tag == REValueTag::Int)   v = row.value.i32;
            if (row.value.tag == REValueTag::Int64) v = (int)row.value.i64;

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##v", &v, 1.0f))
            {
                REVariant nv{};
                if (row.value.tag == REValueTag::Int64)
                {
                    nv.tag = REValueTag::Int64;
                    nv.i64 = (int64_t)v;
                }
                else
                {
                    nv.tag = REValueTag::Int;
                    nv.i32 = v;
                }
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Float:
        {
            float v = (row.value.tag == REValueTag::Float) ? row.value.f32 : 0.0f;

            ImGui::SetNextItemWidth(-FLT_MIN);
            bool changed = false;

            if (row.meta.bHasClamp)
                changed = ImGui::DragFloat("##v", &v, 0.05f, row.meta.clampMin, row.meta.clampMax, "%.3f");
            else
                changed = ImGui::DragFloat("##v", &v, 0.05f, 0.0f, 0.0f, "%.3f");

            if (changed)
            {
                REVariant nv{};
                nv.tag = REValueTag::Float;
                nv.f32 = v;
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Double:
        {
            double v = (row.value.tag == REValueTag::Double) ? row.value.f64 : 0.0;

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragScalar("##v", ImGuiDataType_Double, &v, 0.05))
            {
                REVariant nv{};
                nv.tag = REValueTag::Double;
                nv.f64 = v;
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::String:
        {
            std::string& st = m_StringEdits[row.rowID];
            if (st.empty() && row.value.tag == REValueTag::String)
                st = row.value.s;

            char buf[512];
            std::snprintf(buf, sizeof(buf), "%s", st.c_str());

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##v", buf, sizeof(buf)))
            {
                st = buf;
                REVariant nv{};
                nv.tag = REValueTag::String;
                nv.s = st;
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Vec2:
        {
            FVector2 v = (row.value.tag == REValueTag::Vec2) ? row.value.v2 : FVector2{};
            float x = v.x, y = v.y;

            const float w = CalcComponentWidth(2);

            ImGui::SetNextItemWidth(w);
            bool cx = ImGui::DragFloat("##x", &x, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(w);
            bool cy = ImGui::DragFloat("##y", &y, 0.05f, 0, 0, "%.2f");

            if (cx || cy)
            {
                REVariant nv{};
                nv.tag = REValueTag::Vec2;
                nv.v2 = FVector2{ x, y };
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Vec3:
        {
            FVector3 v = (row.value.tag == REValueTag::Vec3) ? row.value.v3 : FVector3{};
            float x = v.x, y = v.y, z = v.z;

            const float w = CalcComponentWidth(3);

            ImGui::SetNextItemWidth(w);
            bool cx = ImGui::DragFloat("##x", &x, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(w);
            bool cy = ImGui::DragFloat("##y", &y, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(w);
            bool cz = ImGui::DragFloat("##z", &z, 0.05f, 0, 0, "%.2f");

            if (cx || cy || cz)
            {
                REVariant nv{};
                nv.tag = REValueTag::Vec3;
                nv.v3 = FVector3{ x, y, z };
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Vec4:
        {
            FVector4 v = (row.value.tag == REValueTag::Vec4) ? row.value.v4 : FVector4{};
            float x = v.x, y = v.y, z = v.z, w = v.w;

            const float wItem = CalcComponentWidth(4);


            ImGui::SetNextItemWidth(wItem);
            bool cx = ImGui::DragFloat("##x", &x, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(wItem);
            bool cy = ImGui::DragFloat("##y", &y, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(wItem);
            bool cz = ImGui::DragFloat("##z", &z, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(wItem);
            bool cw = ImGui::DragFloat("##w", &w, 0.05f, 0, 0, "%.2f");

            if (cx || cy || cz || cw)
            {
                REVariant nv{};
                nv.tag = REValueTag::Vec4;
                nv.v4 = FVector4{ x, y, z, w };
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Transform:
        {
            FTransform xf = (row.value.tag == REValueTag::Transform) ? row.value.t : FTransform{};

            FVector3 loc = xf.GetPosition();
            FRotator rot = xf.GetRotationAsRotator();
            FVector3 scl = xf.GetScale();

            bool changed = false;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Location");
            ImGui::SameLine(0.0f, 10.0f);
            float l[3] = { loc.x, loc.y, loc.z };
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::DragFloat3("##loc", l, 0.05f, 0.0f, 0.0f, "%.2f"))
            {
                loc = FVector3(l[0], l[1], l[2]);
                changed = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rotation");
            ImGui::SameLine(0.0f, 10.0f);
            float r[3] = { rot.pitch, rot.yaw, rot.roll };
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::DragFloat3("##rot", r, 0.25f, 0.0f, 0.0f, "%.2f"))
            {
                rot = FRotator(r[0], r[1], r[2]);
                changed = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Scale");
            ImGui::SameLine(0.0f, 10.0f);
            float s[3] = { scl.x, scl.y, scl.z };
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::DragFloat3("##scl", s, 0.01f, 0.0f, 0.0f, "%.3f"))
            {
                scl = FVector3(s[0], s[1], s[2]);
                changed = true;
            }

            ImGui::PopStyleVar();

            if (changed)
            {
                xf.SetPosition(loc);
                xf.SetRotation(rot.ToQuat());
                xf.SetScale(scl);

                REVariant nv{};
                nv.tag = REValueTag::Transform;
                nv.t = xf;
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::Enum:
        {
            int64_t raw = (row.value.tag == REValueTag::EnumInt64) ? row.value.i64 : 0;
            int v = (int)raw;

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##v", &v, 1.0f))
            {
                REVariant nv{};
                nv.tag = REValueTag::EnumInt64;
                nv.i64 = (int64_t)v;
                PushEdit(input, row, nv);
            }
        } break;

        case EInspectorWidget::ObjectRef:
        {
            const char* txt =
                (row.value.tag == REValueTag::ObjectUUID)
                    ? (row.value.s.empty() ? "<null>" : row.value.s.c_str())
                    : "<null>";

            ImGui::TextUnformatted(txt);
        } break;

        default:
        {
            ImGui::TextDisabled("%s", row.rawName.c_str());
        } break;
    }

    if (row.bReadOnly)
        ImGui::EndDisabled();

    ImGui::PopID();
}