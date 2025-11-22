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

    // Root object
    writer.BeginObject(); // root {}

    // ------- Metadata at root -------
    writer.Write("name",        info.sceneName);
    writer.Write("actor_count", static_cast<int>(info.actorCount));

    // Editor meta section
    writer.BeginObject("meta");
    writer.Write("thumbnail",     info.thumbnail);
    writer.Write("last_modified", info.lastModified);
    writer.EndObject(); // meta

    // ---------- "Objects" array ----------
    writer.BeginArray("Objects"); // "Objects": [ ]

    for (const JCoreObject* obj : info.objects)
    {
        if (!obj) continue;

        writer.BeginObject(); // { ... } inside Objects[]

        // Top-level identity & type
        writer.Write("uuid", obj->GetUUID());
        writer.Write("type", std::string(obj->GetClassTypeName()));

        // "data": { ...fields... }
        writer.BeginObject("data");
        obj->SerializeJObject(writer); // reflected + custom
        writer.EndObject(); // end "data"

        writer.EndObject(); // end this object entry
    }

    writer.EndArray(); // end "Objects"

    // ---------- "scene" section ----------
    writer.BeginObject("scene");

    // RootActors array
    writer.BeginArray("root_actors");
    for (JActor* actor : info.rootActors)
        writer.WriteValue(actor->GetUUID());
    writer.EndArray();

    // Actor logic components
    writer.BeginObject("actor_components");
    for (auto& [actor, comps] : info.actorComponents)
    {
        if (!actor) continue;

        writer.BeginArray(actor->GetUUID());
        for (JActorComponent* comp : comps)
        {
            if (!comp) continue;
            writer.WriteValue(comp->GetUUID());
        }
        writer.EndArray();
    }
    writer.EndObject();

    // Scene components (transform/render)
    writer.BeginObject("scene_components");
    for (auto& [actor, comps] : info.sceneComponents)
    {
        if (!actor) continue;

        writer.BeginArray(actor->GetUUID());
        for (JSceneComponent* comp : comps)
        {
            if (!comp) continue;
            writer.WriteValue(comp->GetUUID());
        }
        writer.EndArray();
    }
    writer.EndObject();

    writer.EndObject(); // scene

    writer.EndObject(); // root

    return writer.SaveToFile(filePath);
}

bool JSerializeManager::LoadScene(const std::string& filePath, FSceneLoadResult& outResult)
{
    JsonReader reader;
    if (!reader.LoadFromFile(filePath))
        return false;

    // --- Metadata ---
    outResult.sceneName  = reader.Read<std::string>("name", "");
    outResult.actorCount = reader.Read<unsigned int>("actor_count", 0u);

    if (reader.Has("meta"))
    {
        auto metaReader       = reader.GetObject("meta");
        outResult.thumbnail   = metaReader.Read<std::string>("thumbnail", "");
        outResult.lastModified = metaReader.Read<std::string>("last_modified", "");
    }

    // --- Objects array ---
    auto objectReaders = reader.GetArray("Objects");

    // uuid -> object map for wiring
    std::unordered_map<std::string, JCoreObject*> uuidMap;
    uuidMap.reserve(objectReaders.size());

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

        // "data" object
        JsonReader dataReader = objReader.GetObject("data");
        obj->DeserializeJObject(dataReader);

        uuidMap[uuid] = obj;
        outResult.objects.push_back(obj);
    }

    // --- Scene: root actors + components ---
    if (!reader.Has("scene"))
        return true; // ok: no wiring, just objects + metadata

    JsonReader sceneReader = reader.GetObject("scene");
    const JJson& sceneJson = sceneReader.GetData();

    // RootActors
    auto rootActorUUIDReaders = sceneReader.GetArray("root_actors");
    for (const JsonReader& valueReader : rootActorUUIDReaders)
    {
        const JJson& raw = valueReader.GetData();
        if (!raw.is_string()) continue;

        std::string uuid = raw.get<std::string>();
        auto it = uuidMap.find(uuid);
        if (it != uuidMap.end())
        {
            if (auto* actor = dynamic_cast<JActor*>(it->second))
                outResult.rootActors.push_back(actor);
        }
    }

    // ---- ActorComponents: { "ActorUuid": ["CompUuid", ...] } ----
    if (sceneJson.contains("actor_components") && sceneJson["actor_components"].is_object())
    {
        const JJson& ac = sceneJson["actor_components"];

        for (auto it = ac.begin(); it != ac.end(); ++it)
        {
            const std::string actorUUID = it.key();
            const JJson& compArray = it.value();

            auto itActor = uuidMap.find(actorUUID);
            if (itActor == uuidMap.end())
                continue;

            JActor* actor = dynamic_cast<JActor*>(itActor->second);
            if (!actor)
                continue;

            auto& compList = outResult.actorComponents[actor];

            if (!compArray.is_array())
                continue;

            for (const auto& compUUIDJson : compArray)
            {
                if (!compUUIDJson.is_string())
                    continue;

                std::string compUUID = compUUIDJson.get<std::string>();
                auto itComp = uuidMap.find(compUUID);
                if (itComp == uuidMap.end())
                    continue;

                if (auto* actorComp = dynamic_cast<JActorComponent*>(itComp->second))
                    compList.push_back(actorComp);
            }
        }
    }

    // ---- SceneComponents: { "ActorUuid": ["CompUuid", ...] } ----
    if (sceneJson.contains("scene_components") && sceneJson["scene_components"].is_object())
    {
        const JJson& sc = sceneJson["scene_components"];

        for (auto it = sc.begin(); it != sc.end(); ++it)
        {
            const std::string actorUUID = it.key();
            const JJson& compArray = it.value();

            auto itActor = uuidMap.find(actorUUID);
            if (itActor == uuidMap.end())
                continue;

            JActor* actor = dynamic_cast<JActor*>(itActor->second);
            if (!actor)
                continue;

            auto& compList = outResult.sceneComponents[actor];

            if (!compArray.is_array())
                continue;

            for (const auto& compUUIDJson : compArray)
            {
                if (!compUUIDJson.is_string())
                    continue;

                std::string compUUID = compUUIDJson.get<std::string>();
                auto itComp = uuidMap.find(compUUID);
                if (itComp == uuidMap.end())
                    continue;

                if (auto* sceneComp = dynamic_cast<JSceneComponent*>(itComp->second))
                    compList.push_back(sceneComp);
            }
        }
    }

    return true;
}


JCoreObject* JSerializeManager::CreateObjectByTypeName(const char *typeName)
{
    return RETypeRegistry::CreateInstanceByTypeName(typeName);
}
