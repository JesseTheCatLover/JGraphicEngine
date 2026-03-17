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
     * This is useful for simple/debug-friendly asset types.
     */
    static bool WriteJsonAsset(const std::string& filePath,
                               const FAssetHeader& header,
                               const JJson& payload);

    /**
     * @brief Writes a .jasset file with a JSON header and binary payload.
     *
     * File shape:
     * [magic][containerVersion][headerByteSize][payloadByteSize][headerJsonBytes][payloadBytes]
     */
    static bool WriteBinaryAsset(const std::string& filePath,
                                 const FAssetHeader& header,
                                 const std::vector<uint8_t>& payload);

    /**
     * @brief Reads the common header from a .jasset file.
     *
     * Supports both JSON-only asset files and binary-payload asset files.
     */
    static bool ReadHeader(const std::string& filePath,
                           FAssetHeader& outHeader);

    /**
     * @brief Reads the full .jasset file as header + JSON payload.
     */
    static bool ReadJsonAsset(const std::string& filePath,
                              FAssetHeader& outHeader,
                              JJson& outPayload);

    /**
     * @brief Reads the full .jasset file as header + binary payload.
     */
    static bool ReadBinaryAsset(const std::string& filePath,
                                FAssetHeader& outHeader,
                                std::vector<uint8_t>& outPayload);

private:
    static void WriteHeaderObject(JsonWriter& writer, const FAssetHeader& header);
    static bool ReadHeaderObject(const JsonReader& reader, FAssetHeader& outHeader);

    static bool ParseHeaderJson(const std::string& headerJson, FAssetHeader& outHeader);
    static bool BuildHeaderJson(const FAssetHeader& header, std::string& outHeaderJson);
};