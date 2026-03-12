//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneAPI.h"

#include "Core/EngineContext.h"
#include "Framework/SceneManager.h"
#include "Scene/JScene.h"
#include "Core/JCoreObject.h"
#include "Scene/JActor.h"
#include "Scene/SceneComponents/JSceneComponent.h"
#include "Scene/ActorComponents/JActorComponent.h"

#include "Core/Reflection/RETypeRegistry.h"

namespace
{
    static REProperty* FindPropertyMutable(RETypeRegistry& reg,
                                           JCoreObject& obj,
                                           const std::string& declaringTypeName,
                                           const std::string& propName)
    {
        const REType* most = obj.GetREType();
        if (!most) return nullptr;

        for (const REType* t = most; t != nullptr; t = reg.GetBaseType(t))
        {
            if (t->name != declaringTypeName) continue;

            auto* tm = const_cast<REType*>(t);
            for (auto& p : tm->properties)
                if (p.name == propName)
                    return &p;
        }
        return nullptr;
    }

    static const void* ResolveDeclaringBasePtr_Const(const JCoreObject& mostDerived,
                                                     const std::string& declaringTypeName)
    {
        const REType* most = mostDerived.GetREType();
        if (!most) return &mostDerived;

        auto& reg = RETypeRegistry::Get();

        for (const REType* t = most; t != nullptr; t = reg.GetBaseType(t))
        {
            if (t->name != declaringTypeName)
                continue;

            if (t == most)
                return &mostDerived;

            if (t->upcastFromMostDerived)
                return t->upcastFromMostDerived(&mostDerived);

            // Fallback: OK for most single inheritance cases
            return &mostDerived;
        }

        return &mostDerived;
    }

    static void* ResolveDeclaringBasePtr(JCoreObject& mostDerived,
                                         const std::string& declaringTypeName)
    {
        return const_cast<void*>(ResolveDeclaringBasePtr_Const(mostDerived, declaringTypeName));
    }

    // ---- Variant IO ----

    static bool ReadVariantFromProperty(const REProperty& prop, const void* basePtr, REVariant& out)
    {
        out = {};

        const void* fieldPtr = prop.getConstPtr ? prop.getConstPtr(basePtr) : nullptr;
        if (!fieldPtr)
            return false;

        if (prop.kind == REPropKind::ObjectPtr)
        {
            out.tag = REValueTag::ObjectUUID;
            auto* obj = *reinterpret_cast<JCoreObject* const*>(fieldPtr);
            out.s = obj ? obj->GetUUID() : "";
            return true;
        }

        if (prop.kind == REPropKind::Enum)
        {
            out.tag = REValueTag::EnumInt64;
            out.i64 = *reinterpret_cast<const int64_t*>(fieldPtr);
            return true;
        }

        const std::string& tn = prop.typeName;

        if (tn == "bool")        { out.tag = REValueTag::Bool;   out.b = *reinterpret_cast<const bool*>(fieldPtr); return true; }
        if (tn == "int" || tn == "int32") { out.tag = REValueTag::Int; out.i32 = *reinterpret_cast<const int32_t*>(fieldPtr); return true; }
        if (tn == "int64")       { out.tag = REValueTag::Int64;  out.i64 = *reinterpret_cast<const int64_t*>(fieldPtr); return true; }
        if (tn == "size_t")      { out.tag = REValueTag::Int64;  out.i64 = (int64_t)*reinterpret_cast<const size_t*>(fieldPtr); return true; }
        if (tn == "float")       { out.tag = REValueTag::Float;  out.f32 = *reinterpret_cast<const float*>(fieldPtr); return true; }
        if (tn == "double")      { out.tag = REValueTag::Double; out.f64 = *reinterpret_cast<const double*>(fieldPtr); return true; }
        if (tn == "std::string") { out.tag = REValueTag::String; out.s   = *reinterpret_cast<const std::string*>(fieldPtr); return true; }

        if (tn == "FVector2")    { out.tag = REValueTag::Vec2; out.v2 = *reinterpret_cast<const FVector2*>(fieldPtr); return true; }
        if (tn == "FVector3")    { out.tag = REValueTag::Vec3; out.v3 = *reinterpret_cast<const FVector3*>(fieldPtr); return true; }
        if (tn == "FVector4")    { out.tag = REValueTag::Vec4; out.v4 = *reinterpret_cast<const FVector4*>(fieldPtr); return true; }
        if (tn == "FQuat")       { out.tag = REValueTag::Quat; out.q  = *reinterpret_cast<const FQuat*>(fieldPtr); return true; }
        if (tn == "FTransform")  { out.tag = REValueTag::Transform; out.t = *reinterpret_cast<const FTransform*>(fieldPtr); return true; }

        return false;
    }

    static bool ApplyVariantToProperty(const REProperty& prop, void* basePtr, const REVariant& v)
    {
        if (prop.setFromValue)
            return prop.setFromValue(basePtr, v);

        void* fieldPtr = prop.getPtr ? prop.getPtr(basePtr) : nullptr;
        if (!fieldPtr) return false;

        const std::string& tn = prop.typeName;

        if (tn == "bool" && v.tag == REValueTag::Bool) { *reinterpret_cast<bool*>(fieldPtr) = v.b; return true; }

        if ((tn == "int" || tn == "int32") && v.tag == REValueTag::Int) { *reinterpret_cast<int32_t*>(fieldPtr) = v.i32; return true; }
        if (tn == "int64" && v.tag == REValueTag::Int64) { *reinterpret_cast<int64_t*>(fieldPtr) = v.i64; return true; }
        if (tn == "size_t" && v.tag == REValueTag::Int64) { *reinterpret_cast<size_t*>(fieldPtr) = (size_t)v.i64; return true; }

        if (tn == "float" && v.tag == REValueTag::Float)
        {
            float x = v.f32;
            const auto& rm = prop.GetResolvedMeta();
            if (rm.bHasClamp)
            {
                x = std::max(x, rm.clampMin);
                x = std::min(x, rm.clampMax);
            }
            *reinterpret_cast<float*>(fieldPtr) = x;
            return true;
        }

        if (tn == "double" && v.tag == REValueTag::Double) { *reinterpret_cast<double*>(fieldPtr) = v.f64; return true; }
        if (tn == "std::string" && v.tag == REValueTag::String) { *reinterpret_cast<std::string*>(fieldPtr) = v.s; return true; }

        if (tn == "FVector2" && v.tag == REValueTag::Vec2) { *reinterpret_cast<FVector2*>(fieldPtr) = v.v2; return true; }
        if (tn == "FVector3" && v.tag == REValueTag::Vec3) { *reinterpret_cast<FVector3*>(fieldPtr) = v.v3; return true; }
        if (tn == "FVector4" && v.tag == REValueTag::Vec4) { *reinterpret_cast<FVector4*>(fieldPtr) = v.v4; return true; }
        if (tn == "FQuat" && v.tag == REValueTag::Quat) { *reinterpret_cast<FQuat*>(fieldPtr) = v.q; return true; }
        if (tn == "FTransform" && v.tag == REValueTag::Transform) { *reinterpret_cast<FTransform*>(fieldPtr) = v.t; return true; }

        if (prop.kind == REPropKind::Enum && v.tag == REValueTag::EnumInt64)
        {
            *reinterpret_cast<int64_t*>(fieldPtr) = v.i64;
            return true;
        }

        return false;
    }
}

EditorSceneAPI::EditorSceneAPI(EngineContext &ctx, SceneManager &scene, DebugDraw &debugDraw):
m_Context(ctx),
m_SceneManager(scene),
m_DebugDraw(debugDraw)
{
}

JScene* EditorSceneAPI::GetActiveScene()
{
    return m_SceneManager.GetActiveScene();
}

DebugDraw& EditorSceneAPI::GetDebugDraw()
{
    return m_DebugDraw;
}

JActor* EditorSceneAPI::TryGetActor(ActorID id) const
{
    return m_SceneManager.FindActorByID(id);
}

bool EditorSceneAPI::TryGetActorWorldTransform(ActorID id, FTransform& outXf) const
{
    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a) return false;

    outXf = a->GetActorTransform();
    return true;
}

bool EditorSceneAPI::TrySetActorWorldTransform(ActorID id, const FTransform &xf)
{
    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a) return false;

    a->SetActorTransform(xf);

    return true;
}

bool EditorSceneAPI::TrySetActorWorldLocation(ActorID id, const FVector3& p)
{
    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a) return false;

    a->SetActorLocation(p);
    return true;
}

bool EditorSceneAPI::TrySetActorWorldRotation(ActorID id, const FQuat& q)
{
    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a) return false;

    a->SetActorRotation(q);
    return true;
}

bool EditorSceneAPI::TrySetActorWorldScale(ActorID id, const FVector3& s)
{
    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a) return false;

    a->SetActorScale(s);
    return true;
}

bool EditorSceneAPI::TryGetActorComponents(ActorID id, std::vector<JCoreObject*>& outObjects) const
{
    outObjects.clear();

    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a)
        return false;

    // Reserve to reduce allocations
    const size_t reserveCount =
    a->GetActorComponents().size() + a->GetSceneComponents().size();
    outObjects.reserve(reserveCount);


    // Actor (logic) components
    for (const auto& comp : a->GetActorComponents())
    {
        if (!comp) continue;
        outObjects.push_back(comp); // JActorComponent -> JCoreObject*
    }


    // Scene components (including root + children)
    for (const auto& comp : a->GetSceneComponents())
    {
        if (!comp) continue;
        outObjects.push_back(comp); // JSceneComponent -> JCoreObject*
    }


    return true;
}

std::string EditorSceneAPI::GetActorName(ActorID id) const
{
    if (auto actor = TryGetActor(id))
        return actor->GetActorName();

    return "No actor found.";
}

void EditorSceneAPI::SetActorName(ActorID id, const std::string &newName)
{
    if (auto actor = TryGetActor(id))
        actor->SetActorName(newName);
}

std::vector<FHierarchySnapshot> EditorSceneAPI::BuildHierarchySnapshot() const
{
    std::vector<FHierarchySnapshot> result;

    JScene* scene = m_SceneManager.GetActiveScene();
    if (!scene)
        return result;

    auto actors = m_SceneManager.ListAllActors();
    result.reserve(actors.size());

    for (auto* actor : actors)
    {
        if (!actor) continue;

        FHierarchySnapshot info{};
        info.id = actor->GetRuntimeID();
        info.parentID = actor->GetParentActor()
                            ? actor->GetParentActor()->GetRuntimeID()
                            : 0;
        info.name = actor->GetActorName();
        info.hasChildren = !actor->GetChildActors().empty();

        result.push_back(std::move(info));
    }

    return result;
}

void EditorSceneAPI::SetSelectedActors(const std::vector<ActorID> &ids)
{
    m_SelectedActors = ids;

    // Push to render-facing state (so renderer can outline)
    m_Context.GetEditorSelectionState().SetSelectedActors(ids);
}

void EditorSceneAPI::DeleteActors(const std::vector<ActorID> &ids)
{
    for (ActorID id : ids)
        m_SceneManager.ImmediateDestroyActor(id);
}

void EditorSceneAPI::DuplicateActors(const std::vector<ActorID> &ids)
{
    // TODO: implement cloning later
}

bool EditorSceneAPI::RaycastIntoTheScene(const FRay& ray, FRaycastHit& outHit) // TODO: Should detect based on AABB or mesh for future
{
    outHit = FRaycastHit{};

    JScene* scene = m_SceneManager.GetActiveScene();
    if (!scene)
        return false;

    const auto actors = m_SceneManager.ListAllActors();
    if (actors.empty())
        return false;

    const float pickRadius = 1.5f;  // we tweak this for how "fat" the ray feels
    const float pickRadiusSq = pickRadius * pickRadius;

    const float minDistance = 0.01f; // ignore stuff basically at the ray position

    bool bFound = false;
    float bestDistance = std::numeric_limits<float>::max();

    for (JActor* actor : actors)
    {
        if (!actor)
            continue;

        if (!actor->IsVisible()) continue;

        // Actor world position
        const FVector3 actorPos = actor->GetActorLocation();

        // Vector from ray origin to actor
        const FVector3 toActor = actorPos - ray.origin;

        // Project onto ray dir to get t of closest point on ray to the actor center
        const float t = FMath::Dot(toActor, ray.direction);
        if (t < minDistance)
            continue; // behind or too close

        const FVector3 closestPoint = ray.origin + ray.direction * t;

        // Distance from actor center to ray
        const float distSq = (actorPos - closestPoint).LengthSquared();
        if (distSq > pickRadiusSq)
            continue; // too far from ray

        // Choose the closest valid hit (smallest t)
        if (!bFound || t < bestDistance)
        {
            bFound = true;
            bestDistance = t;

            outHit.bHit = true;
            outHit.actorID = actor->GetRuntimeID();
            outHit.distance = t;
            outHit.position = closestPoint;
            // outHit.Normal   = ... // leave empty for now
        }
    }

    return bFound;
}

JCoreObject* EditorSceneAPI::TryResolveObjectByUUID(ActorID contextActorID, const std::string& objectUUID) const
{
    if (objectUUID.empty())
        return nullptr;

    JActor* actor = TryGetActor(contextActorID);
    if (!actor)
        return nullptr;

    if (actor->GetUUID() == objectUUID)
        return actor;

    if (JSceneComponent* root = actor->GetRootComponent())
        if (root->GetUUID() == objectUUID)
            return root;

    for (JSceneComponent* sc : actor->GetSceneComponents())
        if (sc && sc->GetUUID() == objectUUID)
            return sc;

    for (JActorComponent* ac : actor->GetActorComponents())
        if (ac && ac->GetUUID() == objectUUID)
            return ac;

    return nullptr;
}

bool EditorSceneAPI::TryReadReflectedProperty(ActorID contextActorID,
                                              const std::string& objectUUID,
                                              const std::string& declaringTypeName,
                                              const std::string& propName,
                                              REVariant& outValue) const
{
    JCoreObject* target = TryResolveObjectByUUID(contextActorID, objectUUID);
    if (!target)
        return false;

    auto& reg = RETypeRegistry::Get();

    REProperty* prop = FindPropertyMutable(reg, *target, declaringTypeName, propName);
    if (!prop)
        return false;

    const auto& rm = prop->GetResolvedMeta();
    if (rm.bHiddenInInspector)
        return false;

    const void* basePtrC = ResolveDeclaringBasePtr_Const(*target, declaringTypeName);
    return ReadVariantFromProperty(*prop, basePtrC, outValue);
}

bool EditorSceneAPI::TryWriteReflectedProperty(ActorID contextActorID,
                                               const std::string& objectUUID,
                                               const std::string& declaringTypeName,
                                               const std::string& propName,
                                               const REVariant& value)
{
    JCoreObject* target = TryResolveObjectByUUID(contextActorID, objectUUID);
    if (!target)
        return false;

    auto& reg = RETypeRegistry::Get();

    REProperty* prop = FindPropertyMutable(reg, *target, declaringTypeName, propName);
    if (!prop)
        return false;

    const auto& rm = prop->GetResolvedMeta();
    if (rm.bHiddenInInspector)
        return false;
    if (rm.editorVis == REEditorVis::Visible)
        return false;

    void* basePtr = ResolveDeclaringBasePtr(*target, declaringTypeName);
    return ApplyVariantToProperty(*prop, basePtr, value);
}
