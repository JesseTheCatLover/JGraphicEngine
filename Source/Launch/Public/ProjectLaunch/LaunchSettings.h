//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

class LaunchSettings
{
private:
    bool m_bShouldOpenLastProjectOnStartup = true;

    std::vector<std::string> m_RecentProjects;
    static constexpr size_t kMaxRecentProjects = 50;

public:
    bool Load(const std::string& engineRootPath);
    bool Save(const std::string& engineRootPath) const;

    void Reset();

    [[nodiscard]] bool GetShouldOpenLastProjectOnStartup() const { return m_bShouldOpenLastProjectOnStartup; }
    [[nodiscard]] const std::vector<std::string>& GetRecentProjects() const { return m_RecentProjects; }

    // Call this whenever a project successfully opens
    void RegisterOpenedProject(const std::string& projectFilePath);

private:
    static std::string GetSettingsFilePath(const std::string& engineRootPath);
};