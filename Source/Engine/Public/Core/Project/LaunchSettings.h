//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "FProjectDescriptor.h"

class LaunchSettings
{
private:
    bool m_bShouldOpenLastProjectOnStartup = true;

    std::vector<std::string> m_RecentProjectPaths;
    static constexpr size_t kMaxRecentProjects = 50;

public:
    bool Load(const std::string& engineRootPath);
    bool Save(const std::string& engineRootPath) const;

    void Reset();

    [[nodiscard]] bool GetShouldOpenLastProjectOnStartup() const { return m_bShouldOpenLastProjectOnStartup; }
    void SetShouldOpenLastProjectOnStartup(bool bShould) { m_bShouldOpenLastProjectOnStartup = bShould; }

    [[nodiscard]] const std::vector<std::string>& GetRecentProjectPaths() const { return m_RecentProjectPaths; }

    /** Loads and populates the actual FProjectDescriptor structs for the valid paths */
    [[nodiscard]] std::vector<FProjectDescriptor> LoadRecentProjectDescriptors() const;

    /** Call this whenever a project successfully opens, adds a path to index 0 */
    void RegisterOpenedProject(const std::string& projectFilePath);

    /** Explicit API to remove a project from history (e.g., deleted from disk, or removed via UI button) */
    bool RemoveProjectFromHistory(const std::string& projectFilePath);

private:
    static std::string GetSettingsFilePath(const std::string& engineRootPath);
};
