// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/AssetFile.h"

#include <cstring>
#include <iostream>

#include "Utilities/UFileSystem.h"

namespace
{
    struct FAssetBinaryPrefix
    {
        char magic[4] = { 'J', 'A', 'S', 'T' };
        uint32_t containerVersion = static_cast<uint32_t>(FAssetHeader::CurrentContainerVersion);
        uint64_t headerByteSize = 0;
        uint64_t payloadByteSize = 0;
    };

    static bool IsBinaryAssetBuffer(const std::vector<uint8_t>& bytes)
    {
        if (bytes.size() < sizeof(FAssetBinaryPrefix))
            return false;

        const auto* prefix = reinterpret_cast<const FAssetBinaryPrefix*>(bytes.data());
        return prefix->magic[0] == 'J'
            && prefix->magic[1] == 'A'
            && prefix->magic[2] == 'S'
            && prefix->magic[3] == 'T';
    }
}

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

bool AssetFile::WriteBinaryAsset(const std::string& filePath,
                                 const FAssetHeader& header,
                                 const std::vector<uint8_t>& payload)
{
    std::string headerJson;
    if (!BuildHeaderJson(header, headerJson))
        return false;

    FAssetBinaryPrefix prefix{};
    prefix.containerVersion = static_cast<uint32_t>(header.containerVersion);
    prefix.headerByteSize = static_cast<uint64_t>(headerJson.size());
    prefix.payloadByteSize = static_cast<uint64_t>(payload.size());

    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(FAssetBinaryPrefix) + headerJson.size() + payload.size());

    size_t offset = 0;

    std::memcpy(bytes.data() + offset, &prefix, sizeof(FAssetBinaryPrefix));
    offset += sizeof(FAssetBinaryPrefix);

    if (!headerJson.empty())
    {
        std::memcpy(bytes.data() + offset, headerJson.data(), headerJson.size());
        offset += headerJson.size();
    }

    if (!payload.empty())
    {
        std::memcpy(bytes.data() + offset, payload.data(), payload.size());
    }

    if (!UFileSystem::WriteBinaryFile(filePath, bytes, false))
    {
        std::cerr << "[JAssetFile]: Failed to write binary .jasset file: " << filePath << "\n";
        return false;
    }

    return true;
}

bool AssetFile::ReadHeader(const std::string& filePath,
                           FAssetHeader& outHeader)
{
    const auto bytesOpt = UFileSystem::ReadBinaryFile(filePath);
    if (!bytesOpt.has_value())
    {
        std::cerr << "[JAssetFile]: Failed to read .jasset file: " << filePath << "\n";
        return false;
    }

    const std::vector<uint8_t>& bytes = *bytesOpt;

    // Binary container path
    if (IsBinaryAssetBuffer(bytes))
    {
        if (bytes.size() < sizeof(FAssetBinaryPrefix))
        {
            std::cerr << "[JAssetFile]: Binary .jasset prefix is truncated: " << filePath << "\n";
            return false;
        }

        const auto* prefix = reinterpret_cast<const FAssetBinaryPrefix*>(bytes.data());
        const size_t requiredSize =
            sizeof(FAssetBinaryPrefix) +
            static_cast<size_t>(prefix->headerByteSize) +
            static_cast<size_t>(prefix->payloadByteSize);

        if (bytes.size() < requiredSize)
        {
            std::cerr << "[JAssetFile]: Binary .jasset is truncated: " << filePath << "\n";
            return false;
        }

        const char* headerBegin =
            reinterpret_cast<const char*>(bytes.data() + sizeof(FAssetBinaryPrefix));

        const std::string headerJson(headerBegin, static_cast<size_t>(prefix->headerByteSize));
        return ParseHeaderJson(headerJson, outHeader);
    }

    // Legacy JSON-only path
    JsonReader reader;
    if (!reader.LoadFromFile(filePath) || !reader.IsValid())
    {
        std::cerr << "[JAssetFile]: Failed to read JSON .jasset file: " << filePath << "\n";
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

bool AssetFile::ReadBinaryAsset(const std::string& filePath,
                                FAssetHeader& outHeader,
                                std::vector<uint8_t>& outPayload)
{
    outPayload.clear();

    const auto bytesOpt = UFileSystem::ReadBinaryFile(filePath);
    if (!bytesOpt.has_value())
    {
        std::cerr << "[JAssetFile]: Failed to read binary .jasset file: " << filePath << "\n";
        return false;
    }

    const std::vector<uint8_t>& bytes = *bytesOpt;

    if (!IsBinaryAssetBuffer(bytes))
    {
        std::cerr << "[JAssetFile]: File is not a binary .jasset: " << filePath << "\n";
        return false;
    }

    if (bytes.size() < sizeof(FAssetBinaryPrefix))
    {
        std::cerr << "[JAssetFile]: Binary .jasset prefix is truncated: " << filePath << "\n";
        return false;
    }

    const auto* prefix = reinterpret_cast<const FAssetBinaryPrefix*>(bytes.data());
    const size_t headerOffset = sizeof(FAssetBinaryPrefix);
    const size_t payloadOffset = headerOffset + static_cast<size_t>(prefix->headerByteSize);
    const size_t requiredSize = payloadOffset + static_cast<size_t>(prefix->payloadByteSize);

    if (bytes.size() < requiredSize)
    {
        std::cerr << "[JAssetFile]: Binary .jasset is truncated: " << filePath << "\n";
        return false;
    }

    const char* headerBegin = reinterpret_cast<const char*>(bytes.data() + headerOffset);
    const std::string headerJson(headerBegin, static_cast<size_t>(prefix->headerByteSize));

    if (!ParseHeaderJson(headerJson, outHeader))
        return false;

    const uint8_t* payloadBegin = bytes.data() + payloadOffset;
    outPayload.assign(payloadBegin, payloadBegin + static_cast<size_t>(prefix->payloadByteSize));
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

    return true;
}

bool AssetFile::ParseHeaderJson(const std::string &headerJson, FAssetHeader &outHeader)
{
    try
    {
        JJson parsed = JJson::parse(headerJson);
        JsonReader reader(parsed);
        return ReadHeaderObject(reader, outHeader);
    }
    catch (...)
    {
        std::cerr << "[JAssetFile]: Failed to parse binary asset header JSON\n";
        return false;
    }
}

bool AssetFile::BuildHeaderJson(const FAssetHeader &header, std::string &outHeaderJson)
{
    JsonWriter writer;
    WriteHeaderObject(writer, header);
    outHeaderJson = writer.GetData().dump(4);
    return true;
}
