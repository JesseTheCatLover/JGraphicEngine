//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Project/LaunchSettings.h"

#include <algorithm>

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

void LaunchSettings::Reset()
{
    m_RecentProjectPaths.clear();
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
                    m_RecentProjectPaths.push_back(item.get<std::string>());
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
    writer.Write("recentProjects", m_RecentProjectPaths);

    return writer.SaveToFile(GetSettingsFilePath(engineRootPath));
}

std::string LaunchSettings::GetSettingsFilePath(const std::string& engineRootPath)
{
    const std::string normalizedEngineRoot = UPath::NormalizePhysical(engineRootPath);
    const std::string settingsDir = UPath::NormalizePhysical(UPath::Join(normalizedEngineRoot, "Saved", "Launch"));

    UFileSystem::CreateDirectory(settingsDir);
    return UPath::NormalizePhysical(UPath::Join(settingsDir, "LaunchSettings.json"));
}

std::vector<FProjectDescriptor> LaunchSettings::LoadRecentProjectDescriptors() const
{
    std::vector<FProjectDescriptor> descriptors;
    for (const std::string& path : m_RecentProjectPaths)
    {
        FProjectDescriptor desc;
        // If it's missing, load a mock descriptor so it still appears in the UI grid
        if (!UFileSystem::FileExists(path))
        {
            desc.projectName = UPath::GetFileName(path, false) + " (Missing)";
            desc.description = "Warning: This project file could not be found on disk. The directory may have been moved or deleted.";
            descriptors.push_back(std::move(desc));
            continue;
        }

        // Enabled migration so descriptor cache profiles update smoothly (already loaded projects have been migrated safely)
        if (FProjectDescriptor::LoadFromFile(path, desc, true))
        {
            descriptors.push_back(std::move(desc));
        }
    }
    return descriptors;
}

void LaunchSettings::RegisterOpenedProject(const std::string& projectFilePath)
{
    if (projectFilePath.empty())
        return;

    const std::string normalizedPath = UPath::NormalizePhysical(projectFilePath);

    // If it already exists, remove it so we can push it to the very top
    auto it = std::find(m_RecentProjectPaths.begin(), m_RecentProjectPaths.end(), normalizedPath);
    if (it != m_RecentProjectPaths.end())
    {
        m_RecentProjectPaths.erase(it);
    }

    // Insert at the front (Index 0 = most recent)
    m_RecentProjectPaths.insert(m_RecentProjectPaths.begin(), normalizedPath);

    // Enforce the hard cap
    if (m_RecentProjectPaths.size() > kMaxRecentProjects)
    {
        m_RecentProjectPaths.resize(kMaxRecentProjects);
    }
}

bool LaunchSettings::RemoveProjectFromHistory(const std::string &projectFilePath)
{
    if (projectFilePath.empty()) return false;
    const std::string normalizedPath = UPath::NormalizePhysical(projectFilePath);

    auto it = std::remove(m_RecentProjectPaths.begin(), m_RecentProjectPaths.end(), normalizedPath);
    const bool bRemoved = (it != m_RecentProjectPaths.end());

    if (bRemoved)
    {
        m_RecentProjectPaths.erase(it, m_RecentProjectPaths.end());
    }
    return bRemoved;
}
