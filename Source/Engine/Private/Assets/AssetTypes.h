// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>
#include <string_view>

enum class EAssetType : uint16_t
{
    Unknown = 0,
    Texture2D,
    StaticMesh,
    Material,
    AudioClip,
    Scene,
    Schematic
};

enum class EAssetEncoding : uint8_t
{
    Json = 0,
    Binary = 1
};

inline constexpr std::string_view ToString(EAssetType type)
{
    switch (type)
    {
        case EAssetType::Texture2D:  return "Texture2D";
        case EAssetType::StaticMesh: return "StaticMesh";
        case EAssetType::Material:   return "Material";
        case EAssetType::AudioClip:  return "AudioClip";
        case EAssetType::Scene:      return "Scene";
        case EAssetType::Schematic:  return "Schematic";
        default:                     return "Unknown";
    }
}

inline constexpr std::string_view ToString(EAssetEncoding encoding)
{
    switch (encoding)
    {
        case EAssetEncoding::Json:   return "Json";
        case EAssetEncoding::Binary: return "Binary";
        default:                     return "Json";
    }
}

inline EAssetType AssetTypeFromString(const std::string& value)
{
    if (value == "Texture2D")  return EAssetType::Texture2D;
    if (value == "StaticMesh") return EAssetType::StaticMesh;
    if (value == "Material")   return EAssetType::Material;
    if (value == "AudioClip")  return EAssetType::AudioClip;
    if (value == "Scene")      return EAssetType::Scene;
    if (value == "Schematic")     return EAssetType::Schematic;
    return EAssetType::Unknown;
}

inline EAssetEncoding AssetEncodingFromString(const std::string& value)
{
    if (value == "Binary") return EAssetEncoding::Binary;
    return EAssetEncoding::Json;
}