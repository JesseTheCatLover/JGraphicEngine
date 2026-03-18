// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "FAssetHeader.h"
#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"

class AssetFile
{
public:
    /**
     * @brief Writes a .jasset file with a common header and JSON payload.
     *
     * File shape:
     * {
     *   "header": { ... },
     *   "payload": { ... }
     * }
     *
     * This format is useful for simple or debug-friendly asset types
     * where human readability is desirable.
     */
    static bool WriteJsonAsset(const std::string& filePath,
                               const FAssetHeader& header,
                               const JJson& payload);

    /**
     * @brief Writes a .jasset file with a JSON asset header and binary payload.
     *
     * Binary container layout:
     *
     * [FAssetFileHeader]
     * [headerJsonBytes]
     * [payloadBytes]
     *
     * The container header stores:
     *  - magic identifier ('JAST')
     *  - container version
     *  - byte size of the serialized JSON asset header
     *  - byte size of the binary payload
     *
     * The payload format is defined by the specific asset importer.
     */
    static bool WriteBinaryAsset(const std::string& filePath,
                                 const FAssetHeader& header,
                                 const std::vector<uint8_t>& payload);

    /**
     * @brief Reads the asset header from a .jasset file.
     *
     * Supports both JSON-only asset files and binary-payload asset files.
     * Only the metadata header is parsed; payload data is skipped.
     */
    static bool ReadHeader(const std::string& filePath,
                           FAssetHeader& outHeader);

    /**
     * @brief Reads the full .jasset file as header + JSON payload.
     *
     * This is used for JSON-based asset types written via WriteJsonAsset().
     */
    static bool ReadJsonAsset(const std::string& filePath,
                              FAssetHeader& outHeader,
                              JJson& outPayload);

    /**
     * @brief Reads the full .jasset file as header + binary payload.
     *
     * The binary container header is validated and the JSON asset header
     * is parsed before the raw payload bytes are returned.
     */
    static bool ReadBinaryAsset(const std::string& filePath,
                                FAssetHeader& outHeader,
                                std::vector<uint8_t>& outPayload);

private:
    /**
     * @brief Serializes the FAssetHeader into a JSON object.
     */
    static void WriteHeaderObject(JsonWriter& writer, const FAssetHeader& header);

    /**
     * @brief Deserializes a JSON object into an FAssetHeader.
     */
    static bool ReadHeaderObject(const JsonReader& reader, FAssetHeader& outHeader);

    /**
     * @brief Parses a JSON string into an FAssetHeader.
     */
    static bool ParseHeaderJson(const std::string& headerJson, FAssetHeader& outHeader);

    /**
     * @brief Builds a JSON string from an FAssetHeader.
     */
    static bool BuildHeaderJson(const FAssetHeader& header, std::string& outHeaderJson);
};
