//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Controllers/InspectorController.h"

#include <unordered_map>
#include <vector>

#include "Core/EditorHost.h"
#include "Core/Services/SelectionService.h"
#include "Core/Services/SceneQueryService.h"

#include "Controllers/Inputs/FInspectorPanelInput.h"
#include "Controllers/Outputs/FInspectorOutput.h"

#include "Core/JCoreObject.h"
#include "Scene/JActor.h"
#include "Core/Reflection/RETypeRegistry.h"

// ------------------------- helpers -------------------------
//
// static std::string GetCategoryOrDefault(const FPropertyMetadata& meta, const char* fallback)
// {
//     for (const FMetaEntry& e : meta.entries)
//     {
//         if (e.kind == EMetaKind::Category)
//             return e.value;
//     }
//     return (fallback && fallback[0]) ? fallback : "Default";
// }
//
// static std::string ToValueText(const void* fieldPtr, const std::type_index& ti)
// {
//     if (!fieldPtr) return "<null>";
//
//     if (ti == typeid(int))
//         return std::to_string(*reinterpret_cast<const int*>(fieldPtr));
//     if (ti == typeid(size_t))
//         return std::to_string(*reinterpret_cast<const size_t*>(fieldPtr));
//     if (ti == typeid(float))
//         return std::to_string(*reinterpret_cast<const float*>(fieldPtr));
//     if (ti == typeid(double))
//         return std::to_string(*reinterpret_cast<const double*>(fieldPtr));
//     if (ti == typeid(bool))
//         return (*reinterpret_cast<const bool*>(fieldPtr)) ? "true" : "false";
//     if (ti == typeid(std::string))
//         return *reinterpret_cast<const std::string*>(fieldPtr);
//
//     if (ti == typeid(FVector2))
//     {
//         const auto& v = *reinterpret_cast<const FVector2*>(fieldPtr);
//         return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
//     }
//     if (ti == typeid(FVector3))
//     {
//         const auto& v = *reinterpret_cast<const FVector3*>(fieldPtr);
//         return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
//     }
//     if (ti == typeid(FVector4))
//     {
//         const auto& v = *reinterpret_cast<const FVector4*>(fieldPtr);
//         return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
//                      std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
//     }
//     if (ti == typeid(FRotator))
//     {
//         const auto& r = *reinterpret_cast<const FRotator*>(fieldPtr);
//         return "(Pitch=" + std::to_string(r.Pitch) +
//                ", Yaw=" + std::to_string(r.Yaw) +
//                ", Roll=" + std::to_string(r.Roll) + ")";
//     }
//     if (ti == typeid(FQuat))
//     {
//         const auto& q = *reinterpret_cast<const FQuat*>(fieldPtr);
//         return "(" + std::to_string(q.x()) + ", " + std::to_string(q.y()) + ", " +
//                      std::to_string(q.z()) + ", " + std::to_string(q.w()) + ")";
//     }
//     if (ti == typeid(FTransform))
//     {
//         const auto& t = *reinterpret_cast<const FTransform*>(fieldPtr);
//         return t.GetPosition().ToString() + "\n" + t.GetRotationAsRotator().ToString() + "\n" + t.GetScale().ToString();
//     }
//
//     return "<unsupported>";
// }
//
// static void BuildObjectSnapshotFromObject(const JCoreObject& obj, const char* displayName, FInspectorObjectSnapshot& outObj)
// {
//     outObj.categories.clear();
//     outObj.displayName   = displayName ? displayName : "Object";
//     outObj.objectTypeName = obj.GetClassTypeName();
//     outObj.objectUUID     = obj.GetUUID();
//
//     const REType* mostDerived = RETypeRegistry::FindType(typeid(obj));
//     if (!mostDerived)
//         return;
//
//     std::unordered_map<std::string, size_t> catIndex;
//     catIndex.reserve(16);
//
//     auto GetOrCreateCategory = [&](const std::string& name) -> FInspectorCategorySnapshot&
//     {
//         auto it = catIndex.find(name);
//         if (it != catIndex.end())
//             return outObj.categories[it->second];
//
//         size_t idx = outObj.categories.size();
//         outObj.categories.push_back(FInspectorCategorySnapshot{ name, {} });
//         catIndex[name] = idx;
//         return outObj.categories.back();
//     };
//
//     const char* basePtr = reinterpret_cast<const char*>(&obj);
//
//     for (const REType* type = mostDerived; type != nullptr; type = RETypeRegistry::GetBaseType(type))
//     {
//         for (const REProperty& prop : type->properties)
//         {
//             const void* fieldPtr = basePtr + prop.offset;
//
//             const std::string category = GetCategoryOrDefault(prop.metadata, type->name.c_str());
//             auto& cat = GetOrCreateCategory(category);
//
//             FInspectorRow row;
//             row.displayName       = prop.name;
//             row.declaringTypeName = type->name;
//             row.typeName          = prop.type.name();
//             row.valueText         = ToValueText(fieldPtr, prop.type);
//             row.metadata          = prop.metadata;
//
//             cat.rows.push_back(std::move(row));
//         }
//     }
// }
//
// ------------------------- controller -------------------------

InspectorController::InspectorController(PanelID id, EditorHost& host)
    : m_PanelID(id)
    , m_Host(host)
{}

void InspectorController::Update(float /*deltaTime*/, const FInspectorPanelInput& /*input*/, FInspectorOutput& out)
{
    // auto& selection = m_Host.GetService<SelectionService>();
    // auto& queries   = m_Host.GetService<SceneQueryService>();
    //
    // out.bHasSelection = false;
    // out.bHasSnapshot  = false;
    // out.selectedActor = 0;
    // out.snapshot      = nullptr;
    // out.statusText    = nullptr;
    //
    // const auto& selected = selection.GetSelection();
    // if (selected.empty())
    // {
    //     out.statusText = "Inspector: nothing selected.";
    //     return;
    // }
    //
    // out.bHasSelection = true;
    //
    // if (selected.size() > 1)
    // {
    //     out.statusText = "Inspector: multi-select not supported yet.";
    //     return;
    // }
    //
    // const uint64_t actorId = selected[0];
    // out.selectedActor = actorId;
    //
    // JActor* actor = queries.TryGetActor(actorId);
    // if (!actor)
    // {
    //     out.statusText = "Inspector: selected actor not found.";
    //     return;
    // }
    //
    // // Build snapshot: Actor + Components
    // m_Snapshot.objects.clear();
    // m_Snapshot.objects.reserve(8);
    //
    // // Actor block
    // {
    //     FInspectorObjectSnapshot objSnap;
    //     BuildObjectSnapshotFromObject(*actor, "Actor", objSnap);
    //     m_Snapshot.objects.push_back(std::move(objSnap));
    // }
    //
    // // Component blocks
    // std::vector<JCoreObject*> comps;
    // if (queries.TryGetActorComponents(actorId, comps))
    // {
    //     for (JCoreObject* c : comps)
    //     {
    //         if (!c) continue;
    //
    //         // Basic label
    //         std::string label = std::string("Component: ") + c->GetClassTypeName();
    //
    //         FInspectorObjectSnapshot objSnap;
    //         BuildObjectSnapshotFromObject(*c, label.c_str(), objSnap);
    //         m_Snapshot.objects.push_back(std::move(objSnap));
    //     }
    // }
    //
    // out.snapshot = &m_Snapshot;
    // out.bHasSnapshot = true;
    //
    // // Status if nothing reflected anywhere
    // bool bAnyProps = false;
    // for (const auto& o : m_Snapshot.objects)
    // {
    //     for (const auto& cat : o.categories)
    //     {
    //         if (!cat.rows.empty()) { bAnyProps = true; break; }
    //     }
    //     if (bAnyProps) break;
    // }
    //
    // if (!bAnyProps)
    // {
    //     out.statusText = "Inspector: no reflected properties registered.";
    // }
}