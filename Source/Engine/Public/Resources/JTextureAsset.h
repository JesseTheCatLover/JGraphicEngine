//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include "Core/JCoreObject.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Rendering/JTexture.h"

class JTextureAsset : JCoreObject
{
    DECLARE_JOBJECT(JTextureAsset);

    std::string Name;
    std::string FilePath;

    void Serialize(JsonWriter& writer) const override;
    void Deserialize(const JsonReader& reader) override;

    std::unique_ptr<JTexture> CreateRuntimeTexture() const;
};
