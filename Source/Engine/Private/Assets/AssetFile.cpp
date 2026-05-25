// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/AssetFile.h"

#include <cstring>
#include <iostream>

#include "Assets/FAssetFileHeader.h"
#include "Utilities/UFileSystem.h"

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

    FAssetFileHeader fileHeader{};
    fileHeader.containerVersion = static_cast<uint32_t>(header.containerVersion);
    fileHeader.headerByteSize  = static_cast<uint64_t>(headerJson.size());
    fileHeader.payloadByteSize = static_cast<uint64_t>(payload.size());

    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(FAssetFileHeader) + headerJson.size() + payload.size());

    size_t offset = 0;
    std::memcpy(bytes.data() + offset, &fileHeader, sizeof(FAssetFileHeader));
    offset += sizeof(FAssetFileHeader);

    if (!headerJson.empty())
    {
        std::memcpy(bytes.data() + offset, headerJson.data(), headerJson.size());
        offset += headerJson.size();
    }

    if (!payload.empty())
        std::memcpy(bytes.data() + offset, payload.data(), payload.size());

    if (!UFileSystem::WriteBinaryFile(filePath, bytes, false))
    {
        std::cerr << "[JAssetFile]: Failed to write binary .jasset file: " << filePath << "\n";
        return false;
    }

    return true;
}

bool AssetFile::RewriteAssetID(const std::string& filePath,
                               const std::string& newAssetID)
{
    if (newAssetID.empty())
    {
        std::cerr << "[JAssetFile]: RewriteAssetID failed: newAssetID is empty\n";
        return false;
    }

    const auto bytesOpt = UFileSystem::ReadBinaryFile(filePath);
    if (!bytesOpt.has_value())
    {
        std::cerr << "[JAssetFile]: RewriteAssetID failed: could not read file: "
                  << filePath << "\n";
        return false;
    }

    const std::vector<uint8_t>& bytes = *bytesOpt;

    // ---------------------------------------------------------------------
    // Binary container format
    // ---------------------------------------------------------------------
    if (bytes.size() >= sizeof(FAssetFileHeader))
    {
        FAssetFileHeader container{};
        std::memcpy(&container, bytes.data(), sizeof(FAssetFileHeader));

        if (std::memcmp(container.magic, "JAST", 4) == 0)
        {
            const uint64_t headerSize64  = container.headerByteSize;
            const uint64_t payloadSize64 = container.payloadByteSize;

            // Prevent overflow / insane sizes on 32-bit size_t platforms or corrupt files
            if (headerSize64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
                payloadSize64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            {
                std::cerr << "[JAssetFile]: RewriteAssetID failed: header/payload size too large: "
                          << filePath << "\n";
                return false;
            }

            const size_t headerSize  = static_cast<size_t>(headerSize64);
            const size_t payloadSize = static_cast<size_t>(payloadSize64);

            const size_t headerOffset  = sizeof(FAssetFileHeader);
            const size_t payloadOffset = headerOffset + headerSize;

            if (payloadOffset < headerOffset) // overflow guard
            {
                std::cerr << "[JAssetFile]: RewriteAssetID failed: overflow computing offsets: "
                          << filePath << "\n";
                return false;
            }

            const size_t requiredSize = payloadOffset + payloadSize;
            if (requiredSize < payloadOffset || bytes.size() < requiredSize)
            {
                std::cerr << "[JAssetFile]: RewriteAssetID failed: truncated binary .jasset: "
                          << filePath << "\n";
                return false;
            }

            // Extract + parse header JSON
            const char* headerBegin = reinterpret_cast<const char*>(bytes.data() + headerOffset);
            std::string headerJson(headerBegin, headerSize);

            FAssetHeader header{};
            if (!ParseHeaderJson(headerJson, header))
            {
                std::cerr << "[JAssetFile]: RewriteAssetID failed: could not parse header JSON: "
                          << filePath << "\n";
                return false;
            }

            std::string newHeaderJson;
            if (!BuildHeaderJson(header, newHeaderJson))
            {
                std::cerr << "[JAssetFile]: RewriteAssetID failed: could not build new header JSON\n";
                return false;
            }

            // Slice payload bytes (preserve exactly)
            const auto payloadBeginIt = bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset);
            const auto payloadEndIt   = payloadBeginIt + static_cast<std::ptrdiff_t>(payloadSize);
            std::vector<uint8_t> payload(payloadBeginIt, payloadEndIt);

            // Rebuild container
            FAssetFileHeader newContainer{};
            std::memcpy(newContainer.magic, "JAST", 4);
            newContainer.containerVersion = static_cast<uint32_t>(header.containerVersion);
            newContainer.headerByteSize   = static_cast<uint64_t>(newHeaderJson.size());
            newContainer.payloadByteSize  = static_cast<uint64_t>(payload.size());

            std::vector<uint8_t> outBytes;
            outBytes.resize(sizeof(FAssetFileHeader) + newHeaderJson.size() + payload.size());

            size_t offset = 0;
            std::memcpy(outBytes.data() + offset, &newContainer, sizeof(FAssetFileHeader));
            offset += sizeof(FAssetFileHeader);

            if (!newHeaderJson.empty())
            {
                std::memcpy(outBytes.data() + offset, newHeaderJson.data(), newHeaderJson.size());
                offset += newHeaderJson.size();
            }

            if (!payload.empty())
                std::memcpy(outBytes.data() + offset, payload.data(), payload.size());

            if (!UFileSystem::WriteBinaryFile(filePath, outBytes, /*bOverwrite=*/true))
            {
                std::cerr << "[JAssetFile]: RewriteAssetID failed: could not write file: "
                          << filePath << "\n";
                return false;
            }

            return true;
        }
    }

    // ---------------------------------------------------------------------
    // Legacy JSON-only asset format
    // ---------------------------------------------------------------------
    JsonReader reader;
    if (!reader.LoadFromFile(filePath) || !reader.IsValid())
    {
        std::cerr << "[JAssetFile]: RewriteAssetID failed: invalid JSON .jasset: "
                  << filePath << "\n";
        return false;
    }

    if (!reader.IsObject("header"))
    {
        std::cerr << "[JAssetFile]: RewriteAssetID failed: missing 'header' object: "
                  << filePath << "\n";
        return false;
    }

    JJson root = reader.GetData();
    if (!root.is_object())
    {
        std::cerr << "[JAssetFile]: RewriteAssetID failed: root is not object: "
                  << filePath << "\n";
        return false;
    }

    try
    {
        const std::string text = root.dump(4);
        std::vector<uint8_t> out(text.begin(), text.end());

        if (!UFileSystem::WriteBinaryFile(filePath, out, /*bOverwrite=*/true))
        {
            std::cerr << "[JAssetFile]: RewriteAssetID failed: could not write JSON .jasset: "
                      << filePath << "\n";
            return false;
        }

        return true;
    }
    catch (...)
    {
        std::cerr << "[JAssetFile]: RewriteAssetID failed: exception during JSON serialization\n";
        return false;
    }
}

bool AssetFile::ReadHeader(const std::string& filePath, FAssetHeader& outHeader)
{
    outHeader = {};

    const auto bytesOpt = UFileSystem::ReadBinaryFile(filePath);
    if (!bytesOpt.has_value())
    {
        std::cerr << "[JAssetFile]: Failed to read .jasset file: " << filePath << "\n";
        return false;
    }

    const std::vector<uint8_t>& bytes = *bytesOpt;

    // ---------------------------------------------------------------------
    // Binary container format
    // ---------------------------------------------------------------------
    if (bytes.size() >= sizeof(FAssetFileHeader))
    {
        FAssetFileHeader container;
        std::memcpy(&container, bytes.data(), sizeof(FAssetFileHeader));

        if (std::memcmp(container.magic, "JAST", 4) == 0)
        {
            const size_t headerOffset = sizeof(FAssetFileHeader);
            const size_t payloadOffset =
                headerOffset + static_cast<size_t>(container.headerByteSize);

            const size_t requiredSize =
                payloadOffset + static_cast<size_t>(container.payloadByteSize);

            if (bytes.size() < requiredSize)
            {
                std::cerr << "[JAssetFile]: Truncated binary .jasset file: "
                          << filePath << "\n";
                return false;
            }

            const char* headerBegin =
                reinterpret_cast<const char*>(bytes.data() + headerOffset);

            const std::string headerJson(
                headerBegin,
                static_cast<size_t>(container.headerByteSize));

            return ParseHeaderJson(headerJson, outHeader);
        }
    }

    // ---------------------------------------------------------------------
    // Legacy JSON-only asset format
    // ---------------------------------------------------------------------
    JsonReader reader;
    if (!reader.LoadFromFile(filePath) || !reader.IsValid())
    {
        std::cerr << "[JAssetFile]: Failed to read JSON .jasset file: "
                  << filePath << "\n";
        return false;
    }

    if (!reader.IsObject("header"))
    {
        std::cerr << "[JAssetFile]: Missing 'header' object in .jasset: "
                  << filePath << "\n";
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
        return false;

    const std::vector<uint8_t>& bytes = *bytesOpt;
    if (bytes.size() < sizeof(FAssetFileHeader))
        return false;

    const auto* fileHeader = reinterpret_cast<const FAssetFileHeader*>(bytes.data());
    if (std::memcmp(fileHeader->magic, "JAST", 4) != 0)
    {
        std::cerr << "[JAssetFile]: Invalid magic for .jasset\n";
        return false;
    }

    const size_t headerOffset = sizeof(FAssetFileHeader);
    const size_t payloadOffset = headerOffset + static_cast<size_t>(fileHeader->headerByteSize);

    if (bytes.size() < payloadOffset + static_cast<size_t>(fileHeader->payloadByteSize))
    {
        std::cerr << "[JAssetFile]: Truncated .jasset file\n";
        return false;
    }

    const char* headerBegin = reinterpret_cast<const char*>(bytes.data() + headerOffset);
    std::string headerJson(headerBegin, fileHeader->headerByteSize);
    if (!ParseHeaderJson(headerJson, outHeader))
        return false;

    const uint8_t* payloadBegin = bytes.data() + payloadOffset;
    outPayload.assign(payloadBegin, payloadBegin + static_cast<size_t>(fileHeader->payloadByteSize));

    return true;
}

void AssetFile::WriteHeaderObject(JsonWriter& writer, const FAssetHeader& header)
{
    writer.Write("containerVersion", header.containerVersion);
    writer.Write("payloadVersion", header.payloadVersion);
    writer.Write("assetType", std::string(ToString(header.assetType)));
    writer.Write("encoding", std::string(ToString(header.encoding)));

    writer.Write("assetID", header.assetID);

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
