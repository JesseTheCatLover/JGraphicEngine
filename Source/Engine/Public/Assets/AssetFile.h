// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

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
     */
    static bool WriteJsonAsset(const std::string& filePath,
                               const FAssetHeader& header,
                               const JJson& payload);

    /**
     * @brief Reads the common header from a .jasset file.
     */
    static bool ReadHeader(const std::string& filePath,
                           FAssetHeader& outHeader);

    /**
     * @brief Reads the full .jasset file as header + JSON payload.
     */
    static bool ReadJsonAsset(const std::string& filePath,
                              FAssetHeader& outHeader,
                              JJson& outPayload);

private:
    static void WriteHeaderObject(JsonWriter& writer, const FAssetHeader& header);
    static bool ReadHeaderObject(const JsonReader& reader, FAssetHeader& outHeader);
};