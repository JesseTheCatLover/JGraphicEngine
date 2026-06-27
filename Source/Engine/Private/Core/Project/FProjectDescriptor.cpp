// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Project/FProjectDescriptor.h"

#include <iostream>

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Utilities/UPath.h"

bool FProjectDescriptor::LoadFromFile(const std::string& projectFilePath, FProjectDescriptor& outDescriptor, bool bAutoMigrate)
{
    JsonReader reader;
    if (!reader.LoadFromFile(projectFilePath) || !reader.IsValid())
    {
        std::cerr << "[FProjectDescriptor]: Failed to load .jproject file: " << projectFilePath << "\n";
        return false;
    }

    outDescriptor.descriptorVersion = reader.Read<int32_t>("descriptorVersion", 0);

    bool bNeedsMigration = false;
    if (outDescriptor.descriptorVersion != kCurrentVersion)
        bNeedsMigration = true;

    outDescriptor.projectVersion = reader.Read<int32_t>("projectVersion", kCurrentVersion);
    outDescriptor.projectName    = reader.Read<std::string>("projectName", "");
    outDescriptor.projectID      = reader.Read<std::string>("projectID", "");
    outDescriptor.startupScene   = reader.Read<std::string>("startupScene", "");

    outDescriptor.description           = reader.Read<std::string>("description", "");
    outDescriptor.thumbnailRelativePath = reader.Read<std::string>("thumbnailRelativePath", UPath::Join("Saved", "Thumbnail.png"));

    if (reader.IsObject("engineAssociation"))
    {
        JsonReader assoc = reader.GetObject("engineAssociation");
        outDescriptor.engineAssociation.identifier          = assoc.Read<std::string>("identifier", "");
        outDescriptor.engineAssociation.lastKnownEnginePath = assoc.Read<std::string>("lastKnownEnginePath", "");
    }

    if (reader.IsObject("folders"))
    {
        JsonReader folders = reader.GetObject("folders");
        outDescriptor.folders.assets       = folders.Read<std::string>("assets", "Assets");
        outDescriptor.folders.saved        = folders.Read<std::string>("saved", "Saved");
        outDescriptor.folders.intermediate = folders.Read<std::string>("intermediate", "Intermediate");
        outDescriptor.folders.config       = folders.Read<std::string>("config", "Configs");
    }

    // --- Version Migration Logic ---
    if (bAutoMigrate && bNeedsMigration)
    {
        SaveToFile(projectFilePath, outDescriptor);
    }

    return true;
}

bool FProjectDescriptor::SaveToFile(const std::string& projectFilePath, const FProjectDescriptor& descriptor)
{
    JsonWriter writer;
    writer.Write("descriptorVersion", kCurrentVersion);
    writer.Write("projectVersion", kCurrentVersion);
    writer.Write("projectName", descriptor.projectName);
    writer.Write("projectID", descriptor.projectID);
    writer.Write("startupScene", descriptor.startupScene);

    // Write metadata
    writer.Write("description", descriptor.description);
    writer.Write("thumbnailRelativePath", descriptor.thumbnailRelativePath);

    // Write Engine Association
    writer.BeginObject("engineAssociation");
    writer.Write("identifier", descriptor.engineAssociation.identifier);
    writer.Write("lastKnownEnginePath", descriptor.engineAssociation.lastKnownEnginePath);
    writer.EndObject();

    // Write Folders
    writer.BeginObject("folders");
    writer.Write("assets", descriptor.folders.assets);
    writer.Write("saved", descriptor.folders.saved);
    writer.Write("intermediate", descriptor.folders.intermediate);
    writer.Write("config", descriptor.folders.config);
    writer.EndObject();

    return writer.SaveToFile(projectFilePath);
}