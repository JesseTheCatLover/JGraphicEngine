//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InspectorPanel.h"

#include <algorithm>
#include <cctype>
#include <cstring>
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

static bool BeginPropsTable(const char* id)
{
    ImGuiTableFlags flags =
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_PadOuterX |
        ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable(id, 2, flags))
        return false;

    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    return true;
}

static void PropLabel(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
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
    m_OpenCategoryKeys.clear();
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

    // Components list (selection)
    DrawComponentSection(doc);

    ImGui::InputTextWithHint("##InspectorSearch", "Search", m_SearchBuf, IM_ARRAYSIZE(m_SearchBuf));
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
    ImGui::SeparatorText("Component Section");

    if (!BeginBox("##InspectorComponentsBox", 150.0f))
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
    if (BeginPropsTable("##InspectorHeaderRowsTable"))
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

    // open state by categoryID (since categories are merged globally on the page)
    const uint64_t key = categoryID;
    const bool wasOpen = (m_OpenCategoryKeys.find(key) != m_OpenCategoryKeys.end());
    ImGui::SetNextItemOpen(wasOpen, ImGuiCond_Once);

    const bool open = BeginCategoryHeaderUnique(categoryName.c_str(), categoryID, /*defaultOpen*/true);

    if (open) m_OpenCategoryKeys.insert(key);
    else      m_OpenCategoryKeys.erase(key);

    if (!open)
        return;

    ImGui::PushID((int)categoryID);

    if (BeginPropsTable("##PropsTable"))
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
    PropLabel(row.label.c_str());

    // Critical:
    // row.rowID must be unique across ALL merged targets, which your provider guarantees.
    ImGui::PushID((int)row.rowID);

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

            ImGui::SetNextItemWidth(180.0f);
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

            ImGui::SetNextItemWidth(180.0f);
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

            ImGui::SetNextItemWidth(180.0f);
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

            ImGui::SetNextItemWidth(-1.0f);
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

            ImGui::SetNextItemWidth(90.0f);
            bool cx = ImGui::DragFloat("##x", &x, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
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

            ImGui::SetNextItemWidth(80.0f);
            bool cx = ImGui::DragFloat("##x", &x, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            bool cy = ImGui::DragFloat("##y", &y, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
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

            ImGui::SetNextItemWidth(60.0f);
            bool cx = ImGui::DragFloat("##x", &x, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            bool cy = ImGui::DragFloat("##y", &y, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            bool cz = ImGui::DragFloat("##z", &z, 0.05f, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
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

            ImGui::SetNextItemWidth(180.0f);
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