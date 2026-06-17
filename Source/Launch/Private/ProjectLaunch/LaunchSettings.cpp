//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/LaunchSettings.h"

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

void LaunchSettings::Reset()
{
    m_LastOpenedProjectFilePath.clear();
}

std::string LaunchSettings::GetSettingsFilePath(const std::string& engineRootPath)
{
    const std::string normalizedEngineRoot = UPath::NormalizeVirtual(engineRootPath);
    const std::string settingsDir = UPath::NormalizeVirtual(UPath::Join(normalizedEngineRoot, "Saved", "Launch"));

    UFileSystem::CreateDirectory(settingsDir);
    return UPath::NormalizeVirtual(UPath::Join(settingsDir, "LaunchSettings.json"));
}

bool LaunchSettings::Load(const std::string& engineRootPath)
{
    Reset();

    const std::string filePath = GetSettingsFilePath(engineRootPath);
    if (!UFileSystem::FileExists(filePath))
        return true;

    JsonReader reader;
    if (!reader.LoadFromFile(filePath) || !reader.IsValid())
        return false;

    m_LastOpenedProjectFilePath =
        reader.Read<std::string>("lastOpenedProjectFilePath", "");

    return true;
}

bool LaunchSettings::Save(const std::string& engineRootPath) const
{
    JsonWriter writer;
    writer.Write("lastOpenedProjectFilePath", m_LastOpenedProjectFilePath);
    return writer.SaveToFile(GetSettingsFilePath(engineRootPath));
}