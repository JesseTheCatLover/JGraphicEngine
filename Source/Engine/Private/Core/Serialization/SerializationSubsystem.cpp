//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/SerializationSubsystem.h"

#include "Core/FObjectInitializer.h"
#include "Core/Serialization/SerializeUtilities.h"
#include "Core/JCoreObject.h"

#include "Core/Reflection/RETypeRegistry.h"
#include "Core/Reflection/ReflectSerialize.h"

#include "Scene/JActor.h"
#include "Scene/ActorComponents/JActorComponent.h"
#include "Scene/SceneComponents/JSceneComponent.h"

// ------------------------------------------------------------
// MVP resolver plumbing: function pointer can't capture uuidMap,
// so we temporarily point a static at the active load map.
// ------------------------------------------------------------
static std::unordered_map<std::string, JCoreObject*>* g_LoadUUIDMap = nullptr;

static JCoreObject* ResolveObjectByUUID_MVP(const std::string& uuid)
{
    if (!g_LoadUUIDMap || uuid.empty())
        return nullptr;

    auto it = g_LoadUUIDMap->find(uuid);
    return (it == g_LoadUUIDMap->end()) ? nullptr : it->second;
}

void SerializationSubsystem::Initialize()
{
    // Must be called once, early (engine init).
    RETypeRegistry::Get().Finalize();

    // Optional: default resolver is null; LoadScene installs a scoped one anyway.
    ReflectSerialize::SetObjectResolver(nullptr);
#ifndef NDEBUG
    // RETypeRegistry::Get().DebugDumpAllTypes();
#endif
}

bool SerializationSubsystem::SaveScene(const FSceneSaveInfo& info, const std::string& filePath)
{
    JsonWriter writer;

    writer.BeginObject(); // root {}

    // ------- Scene metadata at root -------
    writer.Write("scene_name", info.sceneName);
    writer.Write("actor_count", info.actorCount);

    writer.BeginObject("meta");
    writer.Write("thumbnail", info.thumbnail);
    writer.Write("last_modified", info.lastModified);
    writer.EndObject(); // meta

    // ---------------- Objects array ----------------
    writer.BeginArray("objects");

    for (const JCoreObject* obj : info.objects)
    {
        if (!obj) continue;

        writer.BeginObject(); // one object entry

        // Identity + type
        writer.Write("uuid", obj->GetUUID());

        // Type name comes from reflection (used for factory creation on load)
        const REType* t = obj->GetREType();
        if (!t) t = RETypeRegistry::Get().FindType(typeid(*obj)); // note: typeid(*obj) not typeid(obj)
        writer.Write("type", t ? t->name : "");

        writer.Write("object_name", obj->GetObjectName());

        // ------------------------------------------------------------
        // overrides:
        //   *reflected* properties that differ from the type's CDO defaults.
        //   This keeps scene files small and diff-friendly.
        // ------------------------------------------------------------
        writer.BeginObject("overrides");

        JCoreObject* cdo = RETypeRegistry::Get().GetCDO(t);

        RETypeRegistry::Get().ForEachProperty_BaseToDerived(t, [&](const REType& declaring, const REProperty& prop)
        {
            if (!cdo) return;

            const void* instBase = obj;
            const void* cdoBase  = cdo;

            // If declaring is not the most-derived, you need base pointers
            if (&declaring != t && declaring.upcastFromMostDerived)
            {
                instBase = declaring.upcastFromMostDerived(obj);
                cdoBase  = declaring.upcastFromMostDerived(cdo);
            }

            if (ReflectSerialize::IsPropertyOverridden(prop, instBase, cdoBase))
                ReflectSerialize::SerializeProperty(writer, prop, instBase);
        });

        writer.EndObject(); // overrides

        // ------------------------------------------------------------
        // relation:
        //   references that connect objects together (actor hierarchy,
        //   component ownership/attachment). Stored as UUID links.
        // ------------------------------------------------------------
        writer.BeginObject("relation");

        // Actor hierarchy
        if (auto* actor = dynamic_cast<const JActor*>(obj)) // TODO: replace with reflection IsA later
        {
            if (const JActor* parent = actor->GetParentActor())
                writer.Write("parent_actor", parent->GetUUID());

            if (JSceneComponent* root = actor->GetRootComponent())
                writer.Write("root_component", root->GetUUID());
        }

        // Scene component hierarchy
        if (auto* sceneComp = dynamic_cast<const JSceneComponent*>(obj))
        {
            if (JActor* owner = sceneComp->GetOwnerActor())
                writer.Write("owner_actor", owner->GetUUID());

            if (JSceneComponent* parentComp = sceneComp->GetParent())
                writer.Write("parent_component", parentComp->GetUUID());
        }

        // Logic components (non-scene actor components)
        if (auto* logicComp = dynamic_cast<const JActorComponent*>(obj))
        {
            if (JActor* owner = logicComp->GetOwnerActor())
                writer.Write("owner_actor", owner->GetUUID());
        }

        writer.EndObject(); // relation

        // ------------------------------------------------------------
        // custom:
        //   object-defined extra JSON written by SerializeCustom()
        //   (and read by DeserializeCustom()).
        //
        //   This is for non-reflected or special-case data you don't want
        //   in the reflection system yet.
        //
        //   NOTE: reflected properties should NOT be written here—those go
        //   into "overrides".
        // ------------------------------------------------------------
        writer.BeginObject("custom");
        obj->SerializeJObject(writer); // reflection layer + SerializeCustom()
        writer.EndObject(); // custom

        writer.EndObject(); // object entry
    }

    writer.EndArray();  // objects
    writer.EndObject(); // root

    return writer.SaveToFile(filePath);
}

bool SerializationSubsystem::LoadScene(const std::string& filePath, FSceneLoadResult& outResult)
{
    JsonReader reader;
    if (!reader.LoadFromFile(filePath))
        return false;

    // --- Metadata ---
    outResult.sceneName  = reader.Read<std::string>("scene_name", "");
    outResult.actorCount = reader.Read<unsigned int>("actor_count", 0u);

    if (reader.Has("meta"))
    {
        auto metaReader = reader.GetObject("meta");
        outResult.thumbnail    = metaReader.Read<std::string>("thumbnail", "");
        outResult.lastModified = metaReader.Read<std::string>("last_modified", "");
    }

    // --- Objects array ---
    auto objectReaders = reader.GetArray("objects");

    // Reset output containers
    outResult.objects.clear();
    outResult.relations.clear();
    outResult.uuidMap.clear();

    outResult.objects.reserve(objectReaders.size());
    outResult.relations.reserve(objectReaders.size());
    outResult.uuidMap.reserve(objectReaders.size());

    // ---------------- PASS 1: Create all objects + UUID map + relations ----------------
    for (const JsonReader& objReader : objectReaders)
    {
        auto uuid       = objReader.Read<std::string>("uuid", "");
        auto typeName   = objReader.Read<std::string>("type", "");
        const auto name = objReader.Read<std::string>("object_name", "");

        if (uuid.empty() || typeName.empty())
            continue;

        // Load-time initializer (Scene/Owner will be fixed by SceneManager wiring later)
        FObjectInitializer init{};
        init.Scene  = nullptr; // unknown until the scene is built / applied
        init.Owner  = nullptr; // unknown until relations are wired
        init.Name   = name;
        init.bIsCDO = false;

        JCoreObject* obj = RETypeRegistry::Get().CreateInstanceByTypeName(typeName, init);
        if (!obj)
            continue;

        // Friend access: adopt saved UUID
        obj->m_UUID = uuid;

        outResult.objects.push_back(obj);
        outResult.uuidMap[uuid] = obj;

        // Capture relations (no wiring here)
        FSceneObjectRelation rel;
        rel.object = obj;

        if (objReader.Has("relation"))
        {
            JsonReader relReader = objReader.GetObject("relation");
            rel.parentActorUUID     = relReader.Read<std::string>("parent_actor", "");
            rel.ownerActorUUID      = relReader.Read<std::string>("owner_actor", "");
            rel.parentComponentUUID = relReader.Read<std::string>("parent_component", "");
            rel.rootComponentUUID   = relReader.Read<std::string>("root_component", "");
        }

        outResult.relations.push_back(std::move(rel));
    }

    // ---------------- PASS 2: Deserialize payload (now UUID map is complete) ----------------
    // Install resolver for this load
    g_LoadUUIDMap = &outResult.uuidMap;
    ReflectSerialize::SetObjectResolver(&ResolveObjectByUUID_MVP);

    // NOTE: We must match readers to objects.
    // Since pass 1 skips malformed entries, we do pass 2 by iterating readers again
    // and using uuid->object lookup (stable).
    for (const JsonReader& objReader : objectReaders)
    {
        auto uuid = objReader.Read<std::string>("uuid", "");
        if (uuid.empty())
            continue;

        auto it = outResult.uuidMap.find(uuid);
        if (it == outResult.uuidMap.end())
            continue;

        JCoreObject* obj = it->second;
        if (!obj)
            continue;

        // Apply reflected overrides first so custom deserialization sees final values.
        if (objReader.Has("overrides"))
        {
            JsonReader over = objReader.GetObject("overrides");
            if (over.IsValid())
                ReflectSerialize::DeserializeTypeProperties(over, *obj->GetREType(), obj);
        }

        // Backward compat: accept old key name "data" as fallback.
        if (objReader.Has("custom") || objReader.Has("data"))
        {
            JsonReader customReader = objReader.Has("custom")
                ? objReader.GetObject("custom")
                : objReader.GetObject("data"); // legacy

            obj->DeserializeJObject(customReader);
        }
    }

    // Remove resolver context (avoid dangling pointer use)
    g_LoadUUIDMap = nullptr;
    ReflectSerialize::SetObjectResolver(nullptr);

    // SceneManager wires relationships using outResult.objects + uuidMap + relations.
    return true;
}

JCoreObject* SerializationSubsystem::CreateObjectByTypeName(const char* typeName)
{
    const std::string name = typeName ? std::string(typeName) : std::string();

    FObjectInitializer Init{};

    return RETypeRegistry::Get().CreateInstanceByTypeName(name, Init);
}