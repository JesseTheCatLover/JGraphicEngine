//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JSerializeManager.h"
#include "Core/Serialization/SerializeUtilities.h"
#include "Core/JCoreObject.h"

#include "Scene/JActor.h"

bool JSerializeManager::SaveScene(const FSceneSaveInfo& info, const std::string& filePath)
{
    JsonWriter writer;

    // Root object
    writer.BeginObject(); // root {}

    // ---------- "Objects" array ----------
    writer.BeginArray("Objects"); // "Objects": [ ]

    for (const JCoreObject* obj : info.Objects)
    {
        writer.BeginObject(); // { ... } inside Objects[]

        // Top-level identity & type
        writer.Write("uuid", obj->GetUUID());
        writer.Write("type", std::string(obj->GetClassTypeName()));

        // "data": { ...fields... }
        writer.BeginObject("data");
        obj->SerializeJObject(writer); // writes reflected properties + custom
        writer.EndObject(); // end "data"

        writer.EndObject(); // end this object entry
    }

    writer.EndArray(); // end "Objects"

    // ---------- "Scene" section ----------
    writer.BeginObject("Scene");

    // RootActors ...
    writer.BeginArray("RootActors");
    for (JActor* actor : info.RootActors)
        writer.WriteValue(actor->GetUUID());
    writer.EndArray();

    // ActorComponents
    writer.BeginObject("ActorComponents");
    for (auto& [actor, comps] : info.ActorComponents)
    {
        const std::string& actorUUID = actor->GetUUID();

        writer.BeginArray(actorUUID);
        for (JCoreObject* comp : comps)
            writer.WriteValue(comp->GetUUID());
        writer.EndArray();
    }
    writer.EndObject(); // ActorComponents

    writer.EndObject(); // Scene

    writer.EndObject(); // root

    return writer.SaveToFile(filePath);
}
bool JSerializeManager::LoadScene(const std::string& filePath, FSceneLoadResult& outResult)
{
    JsonReader reader;
    if (!reader.LoadFromFile(filePath))
        return false;

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
        outResult.Objects.push_back(obj);
    }

    // --- Scene: RootActors + ActorComponents ---
    JsonReader sceneReader = reader.GetObject("Scene");

    // RootActors
    auto rootActorUUIDReaders = sceneReader.GetArray("RootActors");
    for (const JsonReader& valueReader : rootActorUUIDReaders)
    {
        // each element of RootActors is a raw value, not an object,
        // so we access its data directly:
        const JJson& raw = valueReader.GetData();
        if (!raw.is_string()) continue;

        std::string uuid = raw.get<std::string>();
        auto it = uuidMap.find(uuid);
        if (it != uuidMap.end())
        {
            if (auto* actor = dynamic_cast<JActor*>(it->second))
                outResult.RootActors.push_back(actor);
        }
    }

    // ActorComponents: { "ActorUuid": ["CompUuid", ...] }
    const JJson& sceneJson = sceneReader.GetData();
    if (sceneJson.contains("ActorComponents") && sceneJson["ActorComponents"].is_object())
    {
        const JJson& ac = sceneJson["ActorComponents"];

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

            auto& compList = outResult.ActorComponents[actor];

            if (!compArray.is_array())
                continue;

            for (const auto& compUUIDJson : compArray)
            {
                if (!compUUIDJson.is_string())
                    continue;

                std::string compUUID = compUUIDJson.get<std::string>();
                auto itComp = uuidMap.find(compUUID);
                if (itComp != uuidMap.end())
                    compList.push_back(itComp->second);
            }
        }
    }

    return true;
}

JCoreObject* JSerializeManager::CreateObjectByTypeName(const char *typeName)
{
    return RETypeRegistry::CreateInstanceByTypeName(typeName);
}
