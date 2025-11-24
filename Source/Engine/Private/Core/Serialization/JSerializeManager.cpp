//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JSerializeManager.h"
#include "Core/Serialization/SerializeUtilities.h"
#include "Core/JCoreObject.h"

#include "Scene/JActor.h"
#include "Scene/ActorComponents/JActorComponent.h"
#include "Scene/SceneComponents/JSceneComponent.h"

bool JSerializeManager::SaveScene(const FSceneSaveInfo& info, const std::string& filePath)
{
    JsonWriter writer;

    // ---------------- Root object ----------------
    writer.BeginObject(); // root {}

    // ------- Metadata at root -------
    writer.Write("name", info.sceneName);
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
        writer.Write("type", std::string(obj->GetClassTypeName()));

        // ---------- Relations ----------
        writer.BeginObject("relation");

        // Actor hierarchy
        if (auto* actor = dynamic_cast<const JActor*>(obj)) // TODO: For future use a custom type ID system for casting.
        {
            const JActor* parent = actor->GetParentActor();
            if (parent)
            {
                writer.Write("parent_actor", parent->GetUUID());
            }
            // If root: we just omit "parent_actor".
        }

        // Scene component hierarchy
        if (auto* sceneComp = dynamic_cast<const JSceneComponent*>(obj))
        {
            if (JActor* owner = sceneComp->GetOwnerActor())
            {
                writer.Write("owner_actor", owner->GetUUID());
            }

            if (JSceneComponent* parentComp = sceneComp->GetParent())
            {
                writer.Write("parent_component", parentComp->GetUUID());
            }
        }

        // Logic components (non-scene actor components)
        if (auto* logicComp = dynamic_cast<const JActorComponent*>(obj))
        {
            if (JActor* owner = logicComp->GetOwnerActor())
            {
                writer.Write("owner_actor", owner->GetUUID());
            }
        }

        writer.EndObject(); // relation

        // ---------- Data (reflected + custom) ----------
        writer.BeginObject("data");
        obj->SerializeJObject(writer); // reflection + SerializeCustom()
        writer.EndObject(); // data

        writer.EndObject(); // this object
    }

    writer.EndArray(); // objects

    writer.EndObject(); // root

    return writer.SaveToFile(filePath);
}

bool JSerializeManager::LoadScene(const std::string& filePath, FSceneLoadResult& outResult)
{
    JsonReader reader;
    if (!reader.LoadFromFile(filePath))
        return false;

    // --- Metadata ---
    outResult.sceneName = reader.Read<std::string>("name", "");
    outResult.actorCount = reader.Read<unsigned int>("actor_count", 0u);

    if (reader.Has("meta"))
    {
        auto metaReader = reader.GetObject("meta");
        outResult.thumbnail = metaReader.Read<std::string>("thumbnail", "");
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

    for (const JsonReader& objReader : objectReaders)
    {
        std::string uuid = objReader.Read<std::string>("uuid", "");
        std::string typeName = objReader.Read<std::string>("type", "");

        if (uuid.empty() || typeName.empty())
            continue; // malformed entry

        JCoreObject* obj = CreateObjectByTypeName(typeName.c_str());
        if (!obj)
            continue;

        // JSerializeManager is friend of JCoreObject, so this is allowed:
        obj->m_UUID = uuid;

        // --------- Deserialize data ---------
        if (objReader.Has("data"))
        {
            JsonReader dataReader = objReader.GetObject("data");
            obj->DeserializeJObject(dataReader);
        }

        outResult.objects.push_back(obj);
        outResult.uuidMap[uuid] = obj;

        // --------- Capture relations (no wiring here) ---------
        FSceneObjectRelation rel;
        rel.object = obj;

        if (objReader.Has("relation"))
        {
            JsonReader relReader = objReader.GetObject("relation");
            rel.parentActorUUID = relReader.Read<std::string>("parent_actor", "");
            rel.ownerActorUUID = relReader.Read<std::string>("owner_actor", "");
            rel.parentComponentUUID = relReader.Read<std::string>("parent_component", "");
        }

        outResult.relations.push_back(std::move(rel));
    }

    // SceneManager will wire relationship using outResult.objects, outResult.uuidMap and outResult.relations.

    return true;
}

JCoreObject* JSerializeManager::CreateObjectByTypeName(const char* typeName)
{
    return RETypeRegistry::CreateInstanceByTypeName(typeName);
}
