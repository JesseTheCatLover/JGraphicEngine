//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "CpuResource.h"

#include <iostream>

#include "Assets/AssetFile.h"
#include "Assets/FAssetRecord.h"
#include "Assets/AssetRegistrySubsystem.h"
#include "Utilities/UFileSystem.h"

bool CpuResource::LoadAssetBinary(const std::string& assetID, FAssetHeader& outHeader,
                                  std::vector<uint8_t>& outPayload)
{
    if (!m_AssetRegistry)
    {
        std::cerr << "[CpuResource]: Asset registry is null\n";
        return false;
    }

    const FAssetRecord* record = m_AssetRegistry->FindByAssetID(assetID);
    if (!record)
    {
        std::cerr << "[CpuResource]: Asset not found: " << assetID << "\n";
        return false;
    }

    const std::string& path = record->physicalPath;

    if (path.empty())
    {
        std::cerr << "[CpuResource]: Asset path empty: " << assetID << "\n";
        return false;
    }

    if (!UFileSystem::FileExists(path))
    {
        std::cerr << "[CpuResource]: File missing: " << path << "\n";
        return false;
    }

    if (!AssetFile::ReadBinaryAsset(path, outHeader, outPayload))
    {
        std::cerr << "[CpuResource]: Failed reading asset: " << path << "\n";
        return false;
    }

    if (outPayload.empty())
    {
        std::cerr << "[CpuResource]: Asset payload empty: " << path << "\n";
        return false;
    }

    return true;
}
