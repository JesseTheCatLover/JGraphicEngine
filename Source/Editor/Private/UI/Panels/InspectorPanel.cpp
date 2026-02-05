//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InspectorPanel.h"

#include "imgui.h"

#include "Core/EditorHost.h"
#include "Subsystems/InspectorSubsystem.h"
#include "Controllers/Outputs/FInspectorOutput.h"
#include "Controllers/Outputs/FInspectorSnapshot.h"
#include "Core/Reflection/REMeta.h"

// ---------------- helpers ----------------
//
// static const char* FindTooltip(const FPropertyMetadata& meta)
// {
//     for (const FMetaEntry& e : meta.entries)
//     {
//         if (e.kind == EMetaKind::Tooltip)
//             return e.value.c_str();
//     }
//     return nullptr;
// }
//
// static bool FindRange(const FPropertyMetadata& meta, double& outMin, double& outMax)
// {
//     for (const FMetaEntry& e : meta.entries)
//     {
//         if (e.kind != EMetaKind::Range)
//             continue;
//
//         // stored as "min,max"
//         const std::string& s = e.value;
//         const size_t comma = s.find(',');
//         if (comma == std::string::npos)
//             return false;
//
//         try
//         {
//             outMin = std::stod(s.substr(0, comma));
//             outMax = std::stod(s.substr(comma + 1));
//             return true;
//         }
//         catch (...)
//         {
//             return false;
//         }
//     }
//     return false;
// }
//
// // ---------------- InspectorPanel ----------------
//
// size_t InspectorPanel::HashCategory(const std::string& s)
// {
//     return std::hash<std::string>{}(s);
// }
//
// void InspectorPanel::OnDestroy(EditorHost& host)
// {
//     host.GetSubsystem<InspectorSubsystem>().Destroy(GetPanelKey());
// }
//
// void InspectorPanel::DrawRow(const FInspectorRow& row)
// {
//     // 2-column layout: name | value
//     ImGui::TableNextRow();
//
//     ImGui::TableSetColumnIndex(0);
//     ImGui::TextUnformatted(row.displayName.c_str());
//
//     // Tooltip (hover on name)
//     if (const char* tip = FindTooltip(row.metadata))
//     {
//         if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
//         {
//             ImGui::BeginTooltip();
//             ImGui::TextUnformatted(tip);
//             ImGui::EndTooltip();
//         }
//     }
//
//     ImGui::TableSetColumnIndex(1);
//     ImGui::TextUnformatted(row.valueText.c_str());
//
//     // Optional: show range info as subtle hint on hover (for now)
//     double rmin = 0.0, rmax = 0.0;
//     if (FindRange(row.metadata, rmin, rmax))
//     {
//         if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
//         {
//             ImGui::BeginTooltip();
//             ImGui::Text("Range: %.3f .. %.3f", rmin, rmax);
//             ImGui::EndTooltip();
//         }
//     }
// }
//
// void InspectorPanel::DrawCategory(size_t objectIndex, const char* name, const FInspectorCategorySnapshot& cat)
// {
//     const size_t key = HashCategory(std::to_string(objectIndex) + ":" + name);
//
//     const bool wantOpen = (m_OpenCategories.count(key) > 0);
//     ImGui::SetNextItemOpen(wantOpen, ImGuiCond_Always);
//
//     const bool opened = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
//     if (ImGui::IsItemToggledOpen())
//     {
//         if (opened) m_OpenCategories.insert(key);
//         else        m_OpenCategories.erase(key);
//     }
//
//     if (!opened)
//         return;
//
//     if (ImGui::BeginTable(name, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
//     {
//         ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.45f);
//         ImGui::TableSetupColumn("Value",    ImGuiTableColumnFlags_WidthStretch, 0.55f);
//
//         for (const auto& row : cat.rows)
//             DrawRow(row);
//
//         ImGui::EndTable();
//     }
// }
// void InspectorPanel::DrawSnapshot(const FInspectorSnapshot& snap)
// {
//     if (snap.objects.empty())
//     {
//         ImGui::TextUnformatted("No inspector data.");
//         return;
//     }
//
//     for (size_t oi = 0; oi < snap.objects.size(); ++oi)
//     {
//         const auto& obj = snap.objects[oi];
//
//         // Header label: "Actor (JActor)" / "Component: Foo (JFancyComponent)"
//         std::string header = obj.displayName.empty()
//             ? (obj.objectTypeName.empty() ? "Object" : obj.objectTypeName)
//             : (obj.displayName + " (" + obj.objectTypeName + ")");
//
//         // Default open for the first block (Actor), closed for others (optional)
//         if (oi == 0) ImGui::SetNextItemOpen(true, ImGuiCond_Once);
//
//         const bool opened = ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_None);
//         if (!opened)
//             continue;
//
//         // Per-object info
//         if (!obj.objectUUID.empty())
//             ImGui::Text("UUID: %s", obj.objectUUID.c_str());
//
//         ImGui::Separator();
//
//         if (obj.categories.empty())
//         {
//             ImGui::TextUnformatted("No reflected properties.");
//             ImGui::Separator();
//             continue;
//         }
//
//         // Draw categories under this object
//         for (const auto& cat : obj.categories)
//         {
//             // IMPORTANT: push unique ID scope so tables/headers don't collide across objects
//             ImGui::PushID((int)oi);
//             DrawCategory(oi, cat.name.c_str(), cat);
//             ImGui::PopID();
//         }
//
//         ImGui::Separator();
//     }
// }
//
// void InspectorPanel::Draw(EditorHost& host)
// {
//     if (!ImGui::Begin(GetName()))
//     {
//         ImGui::End();
//         return;
//     }
//
//     // Build input for this frame
//     FInspectorPanelInput input{};
//     input.panelKey = GetPanelKey();
//
//     input.bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
//     input.bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
//
//     ImGuiIO& io = ImGui::GetIO();
//     input.bCtrl  = io.KeyCtrl;
//     input.bShift = io.KeyShift;
//     input.bAlt   = io.KeyAlt;
//     input.bSuper = io.KeySuper;
//
//     // Get output snapshot
//     const FInspectorOutput* out = host.GetSubsystem<InspectorSubsystem>().GetOutput(GetPanelKey());
//     if (!out || !out->bHasSnapshot || !out->snapshot)
//     {
//         if (out && out->statusText)
//             ImGui::TextUnformatted(out->statusText);
//         else
//             ImGui::TextUnformatted("Inspector: waiting for snapshot...");
//
//         // IMPORTANT: still submit input so controller gets created next tick
//         host.GetSubsystem<InspectorSubsystem>().SubmitInput(input);
//         ImGui::End();
//         return;
//     }
//
//     // Draw snapshot
//     DrawSnapshot(*out->snapshot);
//
//     // Submit input last
//     host.GetSubsystem<InspectorSubsystem>().SubmitInput(input);
//
//     ImGui::End();
// }