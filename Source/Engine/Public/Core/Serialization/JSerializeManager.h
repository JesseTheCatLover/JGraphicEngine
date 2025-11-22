//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <vector>

class JActor;
class JCoreObject;

struct FSceneSaveInfo
{
    std::vector<JCoreObject*> Objects;  // all actors + components
    std::vector<JActor*> RootActors;
    std::unordered_map<JActor*, std::vector<JCoreObject*>> ActorComponents;
};

struct FSceneLoadResult
{
    std::vector<JCoreObject*> Objects;
    std::vector<JActor*> RootActors;
    std::unordered_map<JActor*, std::vector<JCoreObject*>> ActorComponents;
};

class JSerializeManager
{
public:
    static JSerializeManager& Get()
    {
        static JSerializeManager instance;
        return instance;
    }

    bool SaveScene(const FSceneSaveInfo& info, const std::string& filePath);
    bool LoadScene(const std::string& filePath, FSceneLoadResult& outResult);

private:
    // helper to create object by type name from registry
    JCoreObject* CreateObjectByTypeName(const char* typeName);
};
