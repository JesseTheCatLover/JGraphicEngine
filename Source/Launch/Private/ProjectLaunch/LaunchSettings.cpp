//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/LaunchSettings.h"

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

void LaunchSettings::Reset()
{
    m_RecentProjects.clear();
    m_bShouldOpenLastProjectOnStartup = true;
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

    m_bShouldOpenLastProjectOnStartup = reader.Read("shouldOpenLastProjectOnStartup", m_bShouldOpenLastProjectOnStartup);

    if (reader.Has("recentProjects"))
    {
        // Grab the raw JSON object to parse the array
        JJson data = reader.GetData();
        if (data["recentProjects"].is_array())
        {
            for (const auto& item : data["recentProjects"])
            {
                if (item.is_string())
                    m_RecentProjects.push_back(item.get<std::string>());
            }
        }
    }

    return true;
}

bool LaunchSettings::Save(const std::string& engineRootPath) const
{
    JsonWriter writer;
    JJson& data = writer.GetData();

    writer.Write("shouldOpenLastProjectOnStartup", m_bShouldOpenLastProjectOnStartup);
    writer.Write("recentProjects", m_RecentProjects);

    return writer.SaveToFile(GetSettingsFilePath(engineRootPath));
}

std::string LaunchSettings::GetSettingsFilePath(const std::string& engineRootPath)
{
    const std::string normalizedEngineRoot = UPath::NormalizePhysical(engineRootPath);
    const std::string settingsDir = UPath::NormalizePhysical(UPath::Join(normalizedEngineRoot, "Saved", "Launch"));

    UFileSystem::CreateDirectory(settingsDir);
    return UPath::NormalizePhysical(UPath::Join(settingsDir, "LaunchSettings.json"));
}

void LaunchSettings::RegisterOpenedProject(const std::string& projectFilePath)
{
    if (projectFilePath.empty())
        return;

    const std::string normalizedPath = UPath::NormalizePhysical(projectFilePath);

    // If it already exists, remove it so we can push it to the very top
    auto it = std::find(m_RecentProjects.begin(), m_RecentProjects.end(), normalizedPath);
    if (it != m_RecentProjects.end())
    {
        m_RecentProjects.erase(it);
    }

    // Insert at the front (Index 0 = most recent)
    m_RecentProjects.insert(m_RecentProjects.begin(), normalizedPath);

    // Enforce the hard cap
    if (m_RecentProjects.size() > kMaxRecentProjects)
    {
        m_RecentProjects.resize(kMaxRecentProjects);
    }
}