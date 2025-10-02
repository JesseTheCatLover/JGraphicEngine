//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include "Core/JCoreObject.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Rendering/JMesh.h"

class JMeshAsset : JCoreObject
{
    DECLARE_JOBJECT(JMeshAsset)

public:
    std::string Name;
    std::string FilePath;

    // Serialization
    void Serialize(JsonWriter& writer) const override;
    void Deserialize(const JsonReader& reader) override;

    // Convert to runtime mesh
    std::unique_ptr<JMesh> CreateRuntimeMesh() const;
};