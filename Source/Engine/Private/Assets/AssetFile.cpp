// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "../../Public/Assets/AssetFile.h"

#include <iostream>

bool AssetFile::WriteJsonAsset(const std::string& filePath,
                                const FAssetHeader& header,
                                const JJson& payload)
{
    JsonWriter writer;

    writer.BeginObject("header");
    WriteHeaderObject(writer, header);
    writer.EndObject();

    writer.WriteObject("payload", payload);

    if (!writer.SaveToFile(filePath))
    {
        std::cerr << "[JAssetFile]: Failed to write .jasset file: " << filePath << "\n";
        return false;
    }

    return true;
}

bool AssetFile::ReadHeader(const std::string& filePath,
                            FAssetHeader& outHeader)
{
    JsonReader reader;
    if (!reader.LoadFromFile(filePath) || !reader.IsValid())
    {
        std::cerr << "[JAssetFile]: Failed to read .jasset file: " << filePath << "\n";
        return false;
    }

    if (!reader.IsObject("header"))
    {
        std::cerr << "[JAssetFile]: Missing 'header' object in .jasset: " << filePath << "\n";
        return false;
    }

    return ReadHeaderObject(reader.GetObject("header"), outHeader);
}

bool AssetFile::ReadJsonAsset(const std::string& filePath,
                               FAssetHeader& outHeader,
                               JJson& outPayload)
{
    JsonReader reader;
    if (!reader.LoadFromFile(filePath) || !reader.IsValid())
    {
        std::cerr << "[JAssetFile]: Failed to read .jasset file: " << filePath << "\n";
        return false;
    }

    if (!reader.IsObject("header"))
    {
        std::cerr << "[JAssetFile]: Missing 'header' object in .jasset: " << filePath << "\n";
        return false;
    }

    if (!ReadHeaderObject(reader.GetObject("header"), outHeader))
        return false;

    if (reader.Has("payload"))
        outPayload = reader.GetData()["payload"];
    else
        outPayload = JJson::object();

    return true;
}

void AssetFile::WriteHeaderObject(JsonWriter& writer, const FAssetHeader& header)
{
    writer.Write("containerVersion", header.containerVersion);
    writer.Write("payloadVersion", header.payloadVersion);
    writer.Write("assetType", std::string(ToString(header.assetType)));
    writer.Write("encoding", std::string(ToString(header.encoding)));

    writer.Write("assetID", header.assetID);
    writer.Write("assetName", header.assetName);
    writer.Write("virtualPath", header.virtualPath);

    writer.Write("sourcePath", header.sourcePath);
    writer.Write("importerName", header.importerName);
    writer.Write("dependencyAssetIDs", header.dependencyAssetIDs);
}

bool AssetFile::ReadHeaderObject(const JsonReader& reader, FAssetHeader& outHeader)
{
    outHeader = {};

    outHeader.containerVersion =
        reader.Read<int32_t>("containerVersion", FAssetHeader::CurrentContainerVersion);

    outHeader.payloadVersion =
        reader.Read<int32_t>("payloadVersion", 1);

    outHeader.assetType =
        AssetTypeFromString(reader.Read<std::string>("assetType", "Unknown"));

    outHeader.encoding =
        AssetEncodingFromString(reader.Read<std::string>("encoding", "Json"));

    outHeader.assetID =
        reader.Read<std::string>("assetID", "");

    outHeader.assetName =
        reader.Read<std::string>("assetName", "");

    outHeader.virtualPath =
        reader.Read<std::string>("virtualPath", "");

    outHeader.sourcePath =
        reader.Read<std::string>("sourcePath", "");

    outHeader.importerName =
        reader.Read<std::string>("importerName", "");

    outHeader.dependencyAssetIDs =
        reader.Read<std::vector<std::string>>("dependencyAssetIDs", {});

    if (outHeader.containerVersion <= 0)
    {
        std::cerr << "[JAssetFile]: Invalid containerVersion in .jasset\n";
        return false;
    }

    if (outHeader.containerVersion > FAssetHeader::CurrentContainerVersion)
    {
        std::cerr << "[JAssetFile]: Unsupported future containerVersion in .jasset: "
                  << outHeader.containerVersion << "\n";
        return false;
    }

    if (outHeader.assetID.empty())
    {
        std::cerr << "[JAssetFile]: assetID is empty in .jasset header\n";
        return false;
    }

    if (outHeader.virtualPath.empty())
    {
        std::cerr << "[JAssetFile]: virtualPath is empty in .jasset header\n";
        return false;
    }

    return true;
}