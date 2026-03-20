//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "ICpuResource.h"
#include "Assets/FAssetHeader.h"

class AssetRegistrySubsystem;

class CpuResource : public virtual ICpuResource
{
protected:
    CpuResource(AssetRegistrySubsystem* registry)
        : m_AssetRegistry(registry) {}

    AssetRegistrySubsystem* m_AssetRegistry = nullptr;

    bool LoadAssetBinary(const std::string& assetID, FAssetHeader& outHeader, std::vector<uint8_t>& outPayload);
};
