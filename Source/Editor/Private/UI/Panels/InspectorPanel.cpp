//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InspectorPanel.h"

#include "imgui.h"
#include "Core/EditorHost.h"
#include "Subsystems/InspectorSubsystem.h"
#include "Controllers/Outputs/FInspectorOutput.h"
#include "Controllers/Outputs/FInspectorSnapshot.h"

static size_t HashCombine(size_t a, size_t b)
{
    // simple hash combine
    return a ^ (b + 0x9e3779b97f4a7c15ull + (a<<6) + (a>>2));
}

size_t InspectorPanel::HashCategory(const std::string& s)
{
    return std::hash<std::string>{}(s);
}

size_t InspectorPanel::HashRowKey(const FInspectorRow& row)
{
    size_t h = std::hash<std::string>{}(row.objectUUID);
    h = HashCombine(h, std::hash<std::string>{}(row.declaringTypeName));
    h = HashCombine(h, std::hash<std::string>{}(row.propName));
    return h;
}

void InspectorPanel::OnDestroy(EditorHost& host)
{
    host.GetSubsystem<InspectorSubsystem>().Destroy(GetPanelKey());
}

static void DrawTooltipIfAny(const std::string& tip)
{
    if (tip.empty()) return;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tip.c_str());
        ImGui::EndTooltip();
    }
}

// This draws ONE row and may push an edit to input.edits
void InspectorPanel::DrawRow(const FInspectorRow& row, FInspectorPanelInput& input)
{
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(row.displayName.c_str());
    DrawTooltipIfAny(row.meta.tooltip);

    ImGui::TableSetColumnIndex(1);

    ImGui::PushID(row.objectUUID.c_str());
    ImGui::PushID(row.declaringTypeName.c_str());
    ImGui::PushID(row.propName.c_str());

    const bool bEditable = !row.bReadOnly;

    // If not editable: just show text
    if (!bEditable)
    {
        ImGui::TextUnformatted(row.valueText.c_str());
        ImGui::PopID();
        return;
    }

    // Editable widgets by typeName
    const std::string& tn = row.typeName;

    // ---- bool ----
    if (tn == "bool")
    {
        bool v = (row.valueText == "true"); // MVP (better: row should carry typed value later)
        if (ImGui::Checkbox("##value", &v))
        {
            FInspectorEditCommand cmd;
            cmd.objectUUID = row.objectUUID;
            cmd.declaringTypeName = row.declaringTypeName;
            cmd.propName = row.propName;
            cmd.value.tag = REValueTag::Bool;
            cmd.value.b = v;
            input.edits.push_back(std::move(cmd));
        }
        ImGui::PopID();
        ImGui::PopID();
        ImGui::PopID();
        return;
    }

    // ---- int ----
    if (tn == "int" || tn == "int32")
    {
        int v = 0;
        // MVP parse from string
        try { v = std::stoi(row.valueText); } catch (...) {}

        if (ImGui::DragInt("##value", &v, 1.0f))
        {
            FInspectorEditCommand cmd;
            cmd.objectUUID = row.objectUUID;
            cmd.declaringTypeName = row.declaringTypeName;
            cmd.propName = row.propName;
            cmd.value.tag = REValueTag::Int;
            cmd.value.i32 = v;
            input.edits.push_back(std::move(cmd));
        }
        ImGui::PopID();
        return;
    }

    // ---- float ----
    if (tn == "float")
    {
        float v = 0.f;
        try { v = std::stof(row.valueText); } catch (...) {}

        const float speed = row.meta.bHasStep ? row.meta.step : 0.1f;

        // If you have range meta, use slider, else drag
        bool changed = false;
        if (row.meta.bHasRange)
            changed = ImGui::SliderFloat("##value", &v, row.meta.rangeMin, row.meta.rangeMax);
        else
            changed = ImGui::DragFloat("##value", &v, speed);

        if (changed)
        {
            // clamp meta (hard clamp)
            if (row.meta.bHasClamp)
            {
                if (v < row.meta.clampMin) v = row.meta.clampMin;
                if (v > row.meta.clampMax) v = row.meta.clampMax;
            }

            FInspectorEditCommand cmd;
            cmd.objectUUID = row.objectUUID;
            cmd.declaringTypeName = row.declaringTypeName;
            cmd.propName = row.propName;
            cmd.value.tag = REValueTag::Float;
            cmd.value.f32 = v;
            input.edits.push_back(std::move(cmd));
        }

        ImGui::PopID();
        return;
    }

    // ---- std::string ----
    if (tn == "std::string")
    {
        const size_t key = HashRowKey(row);
        auto& st = m_StringEdits[key];

        // Ensure buffer has something (first time)
        if (st.buf.empty() && !row.valueText.empty())
            st.buf = row.valueText;

        // Build temp char buffer from st.buf
        std::vector<char> tmp(st.buf.begin(), st.buf.end());
        tmp.push_back('\0');
        tmp.resize(512, '\0');

        const bool changed = ImGui::InputText("##value", tmp.data(), tmp.size(),
            ImGuiInputTextFlags_EnterReturnsTrue);

        const bool active = ImGui::IsItemActive();

        // If NOT actively editing, keep UI buffer synced to snapshot
        if (!active && st.buf != row.valueText)
        {
            st.buf = row.valueText;
        }

        // Commit change when Enter
        if (changed)
        {
            st.buf = std::string(tmp.data());

            FInspectorEditCommand cmd;
            cmd.objectUUID = row.objectUUID;
            cmd.declaringTypeName = row.declaringTypeName;
            cmd.propName = row.propName;
            cmd.value.tag = REValueTag::String;
            cmd.value.s = st.buf;
            input.edits.push_back(std::move(cmd));
        }

        ImGui::PopID();
        return;
    }

    // ---- FVector3 (optional) ----
    if (tn == "FVector3")
    {
        // MVP parse "(x, y, z)" is annoying; for now show 3 floats using cached state.
        // Quick win: use a local buffer initialized to 0 and let user edit; it’ll “jump” until
        // you upgrade snapshot to carry typed values. Still workable for now.
        float v[3] = {0,0,0};
        if (ImGui::DragFloat3("##value", v, 0.1f))
        {
            FInspectorEditCommand cmd;
            cmd.objectUUID = row.objectUUID;
            cmd.declaringTypeName = row.declaringTypeName;
            cmd.propName = row.propName;
            cmd.value.tag = REValueTag::Vec3;
            cmd.value.v3 = FVector3(v[0], v[1], v[2]);
            input.edits.push_back(std::move(cmd));
        }
        ImGui::PopID();
        return;
    }

    // Fallback
    ImGui::TextUnformatted(row.valueText.c_str());
    ImGui::PopID();
}

void InspectorPanel::DrawCategory(size_t objectIndex, const char* name, const FInspectorCategorySnapshot& cat, FInspectorPanelInput& input)
{
    const size_t key = HashCategory(std::to_string(objectIndex) + ":" + name);

    const bool wantOpen = (m_OpenCategories.count(key) > 0);
    ImGui::SetNextItemOpen(wantOpen, ImGuiCond_Always);

    const bool opened = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
    if (ImGui::IsItemToggledOpen())
    {
        if (opened) m_OpenCategories.insert(key);
        else        m_OpenCategories.erase(key);
    }

    if (!opened)
        return;

    if (ImGui::BeginTable(name, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableSetupColumn("Value",    ImGuiTableColumnFlags_WidthStretch, 0.55f);

        for (const auto& row : cat.rows)
            DrawRow(row, input);

        ImGui::EndTable();
    }
}

void InspectorPanel::DrawSnapshot(const FInspectorSnapshot& snap, FInspectorPanelInput& input)
{
    if (snap.objects.empty())
    {
        ImGui::TextUnformatted("No inspector data.");
        return;
    }

    for (size_t oi = 0; oi < snap.objects.size(); ++oi)
    {
        const auto& obj = snap.objects[oi];

        std::string header = obj.displayName.empty()
            ? (obj.objectTypeName.empty() ? "Object" : obj.objectTypeName)
            : (obj.displayName + " (" + obj.objectTypeName + ")");

        if (oi == 0) ImGui::SetNextItemOpen(true, ImGuiCond_Once);

        const bool opened = ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_None);
        if (!opened)
            continue;

        if (!obj.objectUUID.empty())
            ImGui::Text("UUID: %s", obj.objectUUID.c_str());

        ImGui::Separator();

        if (obj.categories.empty())
        {
            ImGui::TextUnformatted("No reflected properties.");
            ImGui::Separator();
            continue;
        }

        for (const auto& cat : obj.categories)
        {
            ImGui::PushID((int)oi);
            DrawCategory(oi, cat.name.c_str(), cat, input);
            ImGui::PopID();
        }

        ImGui::Separator();
    }
}

void InspectorPanel::Draw(EditorHost& host)
{
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        return;
    }

    FInspectorPanelInput input{};
    input.panelKey = GetPanelKey();

    input.bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    input.bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGuiIO& io = ImGui::GetIO();
    input.bCtrl  = io.KeyCtrl;
    input.bShift = io.KeyShift;
    input.bAlt   = io.KeyAlt;
    input.bSuper = io.KeySuper;

    const FInspectorOutput* out = host.GetSubsystem<InspectorSubsystem>().GetOutput(GetPanelKey());
    if (!out || !out->bHasSnapshot || !out->snapshot)
    {
        if (out && out->statusText) ImGui::TextUnformatted(out->statusText);
        else                        ImGui::TextUnformatted("Inspector: waiting for snapshot...");

        host.GetSubsystem<InspectorSubsystem>().SubmitInput(input);
        ImGui::End();
        return;
    }

    // Draw + record edits
    DrawSnapshot(*out->snapshot, input);

    // Submit input (with edits)
    host.GetSubsystem<InspectorSubsystem>().SubmitInput(input);

    ImGui::End();
}