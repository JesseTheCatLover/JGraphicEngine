//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Controllers/InspectorController.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

#include "Core/EditorHost.h"
#include "Core/Services/SelectionService.h"
#include "Core/Services/SceneQueryService.h"

#include "Controllers/Inputs/FInspectorPanelInput.h"
#include "Controllers/Outputs/FInspectorOutput.h"

#include "Core/JCoreObject.h"
#include "Scene/JActor.h"
#include "Core/Reflection/REMeta.h"
#include "Core/Reflection/RETypeRegistry.h"


// ------------------------- helpers -------------------------

struct FInspectorContext
{
    bool bSchemaView = false; // true when inspecting a CDO/archetype
};

static std::string GetCategoryOrDefault(const REProperty& prop, const char* fallback)
{
    std::string cat;
    if (REMetaSchema::Get().GetString(prop.meta, REMetaID::Category, cat) && !cat.empty())
        return cat;
    return (fallback && *fallback) ? fallback : "Default";
}

static std::string GetDisplayNameOrDefault(const REProperty& prop)
{
    std::string dn;
    if (REMetaSchema::Get().GetString(prop.meta, REMetaID::DisplayName, dn) && !dn.empty())
        return dn;
    return prop.name;
}

static bool ShouldShowInInspector(const REProperty& prop, const FInspectorContext& ctx, bool& outReadOnly)
{
    const REPropertyMetaResolved& m = prop.GetResolvedMeta();

    // Hide completely (but still serialized)
    if (m.bHiddenInInspector)
        return false;

    // Scope gating
    if (ctx.bSchemaView)
    {
        if (m.editorScope == REEditorScope::InstanceOnly)
            return false;
    }
    else
    {
        if (m.editorScope == REEditorScope::SchemaOnly)
            return false;
    }

    // Readonly vs editable
    outReadOnly = (m.editorVis == REEditorVis::Visible);
    return true;
}

// MVP: format a JCoreObject* nicely if the property is actually a JCoreObject pointer.
// If not, we still show "<object>".
static std::string ObjectPtrToText(const void* fieldPtr)
{
    if (!fieldPtr) return "<null>";
    auto* obj = *reinterpret_cast<JCoreObject* const*>(fieldPtr);
    if (!obj) return "<null>";
    return obj->GetUUID();
}

static std::string EnumToText_Int64(const void* fieldPtr, const REEnum* /*enumType*/)
{
    if (!fieldPtr) return "<null>";
    // MVP: treat enum storage as int64-ish (works for most underlying types if small)
    // If you want perfect later, use enumType->underlyingType or store resolved size/tag.
    const int64_t v = *reinterpret_cast<const int64_t*>(fieldPtr);
    return std::to_string(v);
}

static std::string ToValueText(const REProperty& prop, const void* basePtr)
{
    const void* fieldPtr = prop.getConstPtr ? prop.getConstPtr(basePtr) : nullptr;
    if (!fieldPtr) return "<no-accessor>";

    // Use kind first when possible
    switch (prop.kind)
    {
    case REPropKind::ObjectPtr:
        return ObjectPtrToText(fieldPtr);

    case REPropKind::Enum:
        return EnumToText_Int64(fieldPtr, prop.enumType);

    case REPropKind::ReflectedStruct:
        // MVP: show summary (you can expand recursively later)
        return std::string("<struct ") + (prop.reflectedType ? prop.reflectedType->name : "?") + ">";

    default:
        break;
    }

    // Value types by typeName (generator tokens)
    const std::string& tn = prop.typeName;

    if (tn == "int")       return std::to_string(*reinterpret_cast<const int*>(fieldPtr));
    if (tn == "int32")     return std::to_string(*reinterpret_cast<const int32_t*>(fieldPtr));
    if (tn == "int64")     return std::to_string(*reinterpret_cast<const int64_t*>(fieldPtr));
    if (tn == "size_t")    return std::to_string(*reinterpret_cast<const size_t*>(fieldPtr));
    if (tn == "float")     return std::to_string(*reinterpret_cast<const float*>(fieldPtr));
    if (tn == "double")    return std::to_string(*reinterpret_cast<const double*>(fieldPtr));
    if (tn == "bool")      return (*reinterpret_cast<const bool*>(fieldPtr)) ? "true" : "false";
    if (tn == "std::string") return *reinterpret_cast<const std::string*>(fieldPtr);

    // Math types you included in RETypeRegistry.h
    if (tn == "FVector2")
    {
        const auto& v = *reinterpret_cast<const FVector2*>(fieldPtr);
        return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
    }
    if (tn == "FVector3")
    {
        const auto& v = *reinterpret_cast<const FVector3*>(fieldPtr);
        return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
    }
    if (tn == "FVector4")
    {
        const auto& v = *reinterpret_cast<const FVector4*>(fieldPtr);
        return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
    }
    if (tn == "FQuat")
    {
        const auto& q = *reinterpret_cast<const FQuat*>(fieldPtr);
        return "(" + std::to_string(q.x()) + ", " + std::to_string(q.y()) + ", " + std::to_string(q.z()) + ", " + std::to_string(q.w()) + ")";
    }
    if (tn == "FTransform")
    {
        const auto& t = *reinterpret_cast<const FTransform*>(fieldPtr);
        return t.ToString(); // if you have it; else replace with your own formatting
    }

    return "<unsupported>";
}

static void BuildObjectSnapshotFromObject(const JCoreObject& obj, const char* displayName, FInspectorObjectSnapshot& outObj)
{
    outObj.categories.clear();
    outObj.displayName    = displayName ? displayName : "Object";
    outObj.objectTypeName = obj.GetREType() ? obj.GetREType()->name : "<unknown>";
    outObj.objectUUID     = obj.GetUUID();

    const REType* mostDerived = obj.GetREType();
    if (!mostDerived)
        return;

    FInspectorContext ctx;
    ctx.bSchemaView = obj.IsCDO();

    std::unordered_map<std::string, size_t> catIndex;
    catIndex.reserve(16);

    auto GetOrCreateCategory = [&](const std::string& name) -> FInspectorCategorySnapshot&
    {
        auto it = catIndex.find(name);
        if (it != catIndex.end())
            return outObj.categories[it->second];

        const size_t idx = outObj.categories.size();
        outObj.categories.push_back(FInspectorCategorySnapshot{ name, {} });
        catIndex[name] = idx;
        return outObj.categories.back();
    };

    RETypeRegistry::Get().ForEachProperty_BaseToDerived(mostDerived, [&](const REType& declaring, const REProperty& prop)
    {
        // Adjust base pointer for properties declared in a base type
        const void* instBase = &obj;
        if (&declaring != mostDerived && declaring.upcastFromMostDerived)
            instBase = declaring.upcastFromMostDerived(&obj);

        bool bReadOnly = false;
        if (!ShouldShowInInspector(prop, ctx, bReadOnly))
            return;

        const std::string category = GetCategoryOrDefault(prop, declaring.name.c_str());
        auto& cat = GetOrCreateCategory(category);

        FInspectorRow row;
        row.objectUUID        = outObj.objectUUID;
        row.propName          = prop.name;
        row.displayName       = GetDisplayNameOrDefault(prop);
        row.declaringTypeName = declaring.name;
        row.typeName          = prop.typeName;
        row.valueText         = ToValueText(prop, instBase);
        row.bReadOnly         = bReadOnly;
        row.meta              = prop.GetResolvedMeta();

        cat.rows.push_back(std::move(row));
    });
}

static JCoreObject* FindTargetObjectByUUID(JActor* actor, const std::vector<JCoreObject*>& comps, const std::string& uuid)
{
    if (!actor) return nullptr;
    if (actor->GetUUID() == uuid) return actor;

    for (JCoreObject* c : comps)
        if (c && c->GetUUID() == uuid)
            return c;

    return nullptr;
}

static REProperty* FindPropertyMutable(RETypeRegistry& reg, JCoreObject& obj, const std::string& declaringTypeName, const std::string& propName)
{
    const REType* most = obj.GetREType();
    if (!most) return nullptr;

    // Walk base->derived but we need mutable access; so find the REType* by name each time.
    // Registry stores stable REType; properties vector is mutable in that entry.
    for (const REType* t = most; t != nullptr; t = reg.GetBaseType(t))
    {
        if (t->name != declaringTypeName) continue;

        // We need mutable REType to return mutable REProperty.
        // If you don’t expose FindTypeMutable by name, do this:
        REType* tm = const_cast<REType*>(t);
        for (auto& p : tm->properties)
            if (p.name == propName)
                return &p;
    }
    return nullptr;
}

static bool ApplyVariantToProperty(JCoreObject& obj, REProperty& prop, const REVariant& v)
{
    void* fieldPtr = prop.getPtr ? prop.getPtr(&obj) : nullptr;
    if (!fieldPtr) return false;

    const std::string& tn = prop.typeName;

    // Respect SkipSerialization etc? That’s for serialization, not editing.
    // Respect editor hidden/read-only in panel (already).

    // ---- bool ----
    if (tn == "bool" && v.tag == REValueTag::Bool)
    {
        *reinterpret_cast<bool*>(fieldPtr) = v.b;
        return true;
    }

    // ---- int / int32 ----
    if ((tn == "int" || tn == "int32") && (v.tag == REValueTag::Int))
    {
        *reinterpret_cast<int*>(fieldPtr) = v.i32;
        return true;
    }

    // ---- int64 / size_t ----
    if ((tn == "int64") && (v.tag == REValueTag::Int64))
    {
        *reinterpret_cast<int64_t*>(fieldPtr) = v.i64;
        return true;
    }
    if ((tn == "size_t") && (v.tag == REValueTag::Int64))
    {
        *reinterpret_cast<size_t*>(fieldPtr) = (size_t)v.i64;
        return true;
    }

    // ---- float/double ----
    if (tn == "float" && v.tag == REValueTag::Float)
    {
        float x = v.f32;

        // Apply clamp meta if present
        if (prop.GetResolvedMeta().bHasClamp)
        {
            x = std::max(x, prop.GetResolvedMeta().clampMin);
            x = std::min(x, prop.GetResolvedMeta().clampMax);
        }

        *reinterpret_cast<float*>(fieldPtr) = x;
        return true;
    }

    if (tn == "double" && v.tag == REValueTag::Double)
    {
        double x = v.f64;
        *reinterpret_cast<double*>(fieldPtr) = x;
        return true;
    }

    // ---- string ----
    if (tn == "std::string" && v.tag == REValueTag::String)
    {
        *reinterpret_cast<std::string*>(fieldPtr) = v.s;
        return true;
    }

    // ---- vectors/quats/transforms (if you want editable now) ----
    if (tn == "FVector3" && v.tag == REValueTag::Vec3)
    {
        *reinterpret_cast<FVector3*>(fieldPtr) = v.v3;
        return true;
    }

    // enums + objectptr later (needs safe underlying size / object picker)
    return false;
}

// ------------------------- controller -------------------------

InspectorController::InspectorController(PanelID id, EditorHost& host)
    : m_PanelID(id)
    , m_Host(host)
{}

void InspectorController::Update(float /*deltaTime*/, const FInspectorPanelInput& input, FInspectorOutput& out)
{
    auto& selection = m_Host.GetService<SelectionService>();
    auto& queries   = m_Host.GetService<SceneQueryService>();

    out.bHasSelection = false;
    out.bHasSnapshot  = false;
    out.selectedActor = 0;
    out.snapshot      = nullptr;
    out.statusText    = nullptr;

    const auto& selected = selection.GetSelection();
    if (selected.empty())
    {
        out.statusText = "Inspector: nothing selected.";
        return;
    }

    out.bHasSelection = true;

    if (selected.size() > 1)
    {
        out.statusText = "Inspector: multi-select not supported yet.";
        return;
    }

    const uint64_t actorId = selected[0];
    out.selectedActor = actorId;

    JActor* actor = queries.TryGetActor(actorId);
    if (!actor)
    {
        out.statusText = "Inspector: selected actor not found.";
        return;
    }

    // Build snapshot: Actor + Components
    m_Snapshot.objects.clear();
    m_Snapshot.objects.reserve(8);

    // Actor block
    {
        FInspectorObjectSnapshot objSnap;
        BuildObjectSnapshotFromObject(*actor, "Actor", objSnap);
        m_Snapshot.objects.push_back(std::move(objSnap));
    }

    // Component blocks (you said SceneQueryService can provide components)
    std::vector<JCoreObject*> comps;
    if (queries.TryGetActorComponents(actorId, comps))
    {
        for (JCoreObject* c : comps)
        {
            if (!c) continue;

            const char* typeName = (c->GetREType() ? c->GetREType()->name.c_str() : "<unknown>");
            std::string label = std::string("Component: ") + typeName;

            FInspectorObjectSnapshot objSnap;
            BuildObjectSnapshotFromObject(*c, label.c_str(), objSnap);
            m_Snapshot.objects.push_back(std::move(objSnap));
        }
    }

    for (const FInspectorEditCommand& cmd : input.edits)
    {
        JCoreObject* target = FindTargetObjectByUUID(actor, comps, cmd.objectUUID);
        if (!target) continue;

        REProperty* prop = FindPropertyMutable(RETypeRegistry::Get(), *target, cmd.declaringTypeName, cmd.propName);
        if (!prop) continue;

        // If read-only in meta, ignore (extra safety)
        const auto& rm = prop->GetResolvedMeta();
        if (rm.bHiddenInInspector) continue;
        if (rm.editorVis == REEditorVis::Visible) continue;

        ApplyVariantToProperty(*target, *prop, cmd.value);

        // Optional: mark dirty on scene / transaction system
        // m_Host.GetService<...>().MarkDirty(target);
    }

    out.snapshot = &m_Snapshot;
    out.bHasSnapshot = true;

    // Status message if everything ended up empty
    bool bAnyProps = false;
    for (const auto& o : m_Snapshot.objects)
    {
        for (const auto& cat : o.categories)
        {
            if (!cat.rows.empty()) { bAnyProps = true; break; }
        }
        if (bAnyProps) break;
    }

    if (!bAnyProps)
        out.statusText = "Inspector: no reflected properties visible.";
}