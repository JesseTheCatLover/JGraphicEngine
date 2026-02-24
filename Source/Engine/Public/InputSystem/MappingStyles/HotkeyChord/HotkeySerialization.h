//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "HotkeyChordConfig.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"

// ---- Low-level conversion helpers ----
JJson ToJson(const FHotkeyChord& chord);
bool FromJson(const JJson& j, FHotkeyChord& outChord);

JJson ToJson(const FHotkeyCommand& command);
bool FromJson(const JJson& j, FHotkeyCommand& outCommand);

JJson ToJson(const FHotkeyMap& map);
bool FromJson(const JJson& j, FHotkeyMap& outMap);

JJson ToJson(const FHotkeyOverrideEntry& entry);
bool FromJson(const JJson& j, FHotkeyOverrideEntry& outEntry);

JJson ToJson(const FHotkeyOverrides& overrides);
bool FromJson(const JJson& j, FHotkeyOverrides& outOverrides);

// ---- File helpers (convenience) ----
bool SaveHotkeyMapToFile(const FHotkeyMap& map, const std::string& filePath);
bool LoadHotkeyMapFromFile(const std::string& filePath, FHotkeyMap& outMap);

bool SaveHotkeyOverridesToFile(const FHotkeyOverrides& overrides, const std::string& filePath);
bool LoadHotkeyOverridesFromFile(const std::string& filePath, FHotkeyOverrides& outOverrides);