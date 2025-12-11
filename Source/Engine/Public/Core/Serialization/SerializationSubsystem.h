//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class JSceneComponent;
class JActorComponent;
class JActor;
class JCoreObject;

struct FSceneObjectRelation
{
    JCoreObject* object = nullptr;

    std::string parentActorUUID;     // for JActor
    std::string rootComponentUUID;   // for JActor
    std::string ownerActorUUID;      // for components
    std::string parentComponentUUID; // for scene components
};

struct FSceneSaveInfo
{
    std::vector<JCoreObject*> objects;  // all actors + components

    // Metadata
    std::string sceneName;
    unsigned int actorCount = 0;
    std::string thumbnail;
    std::string lastModified;
};

struct FSceneLoadResult
{
    std::vector<JCoreObject*> objects;

    // Helper tables (internal to SceneManager/SerializationSubsystem)
    std::unordered_map<std::string, JCoreObject*> uuidMap;
    std::vector<FSceneObjectRelation> relations;

    // Metadata
    std::string sceneName;
    unsigned int actorCount = 0;
    std::string thumbnail;
    std::string lastModified;
};

class SerializationSubsystem
{
public:
    static SerializationSubsystem& Get()
    {
        static SerializationSubsystem instance;
        return instance;
    }

    bool SaveScene(const FSceneSaveInfo& info, const std::string& filePath);
    bool LoadScene(const std::string& filePath, FSceneLoadResult& outResult);

private:
    // helper to create object by type name from registry
    JCoreObject* CreateObjectByTypeName(const char* typeName);
};
