//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

class LaunchSettings
{
public:
    bool Load(const std::string& engineRootPath);
    bool Save(const std::string& engineRootPath) const;

    void Reset();

    const std::string& GetLastOpenedProjectFilePath() const { return m_LastOpenedProjectFilePath; }
    void SetLastOpenedProjectFilePath(const std::string& path) { m_LastOpenedProjectFilePath = path; }

private:
    static std::string GetSettingsFilePath(const std::string& engineRootPath);

private:
    std::string m_LastOpenedProjectFilePath;
};