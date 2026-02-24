//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyles/HotkeyChord/HotkeySerialization.h"

namespace
{
    constexpr int kHotkeyJsonVersion = 1;

    static uint8_t ToU8(EHotkeyPlatformMask mask)
    {
        return static_cast<uint8_t>(mask);
    }

    static EHotkeyPlatformMask ToPlatformMaskSafe(int v)
    {
        // Accept any bit combination; unknown bits are preserved in uint8 cast
        return static_cast<EHotkeyPlatformMask>(static_cast<uint8_t>(v));
    }

    static int ToInt(EPhysicalInput k)
    {
        return static_cast<int>(static_cast<uint16_t>(k));
    }

    static EPhysicalInput ToPhysicalInputSafe(int v)
    {
        if (v < 0)
            return EPhysicalInput::Unknown;

        const uint16_t uv = static_cast<uint16_t>(v);
        if (uv >= static_cast<uint16_t>(EPhysicalInput::Count))
            return EPhysicalInput::Unknown;

        return static_cast<EPhysicalInput>(uv);
    }

    static std::vector<EPhysicalInput> ReadKeysArray(const JJson& arr)
    {
        std::vector<EPhysicalInput> out;
        if (!arr.is_array())
            return out;

        out.reserve(arr.size());

        for (const auto& item : arr)
        {
            try
            {
                const int v = item.get<int>();
                out.push_back(ToPhysicalInputSafe(v));
            }
            catch (...)
            {
                out.push_back(EPhysicalInput::Unknown);
            }
        }

        return out;
    }

    static JJson WriteKeysArray(const std::vector<EPhysicalInput>& keys)
    {
        JJson arr = JJson::array();
        for (EPhysicalInput k : keys)
            arr.push_back(ToInt(k));
        return arr;
    }
}

// ---------------- FHotkeyChord ----------------

JJson ToJson(const FHotkeyChord& chord)
{
    JJson j;
    j["keys"] = WriteKeysArray(chord.keys);
    j["platforms"] = ToU8(chord.platforms);
    j["allowExtraModifiers"] = chord.allowExtraModifiers;
    j["allowExtraKeys"] = chord.allowExtraKeys;
    return j;
}

bool FromJson(const JJson& j, FHotkeyChord& outChord)
{
    if (!j.is_object())
        return false;

    outChord = FHotkeyChord{};

    if (j.contains("keys"))
        outChord.keys = ReadKeysArray(j["keys"]);

    outChord.platforms = ToPlatformMaskSafe(j.value("platforms", static_cast<int>(ToU8(EHotkeyPlatformMask::Any))));
    outChord.allowExtraModifiers = j.value("allowExtraModifiers", true);
    outChord.allowExtraKeys = j.value("allowExtraKeys", false);

    return true;
}

// ---------------- FHotkeyCommand ----------------

JJson ToJson(const FHotkeyCommand& command)
{
    JJson j;
    j["name"] = command.name;
    j["category"] = command.category;
    j["description"] = command.description;

    j["chords"] = JJson::array();
    for (const FHotkeyChord& c : command.chords)
        j["chords"].push_back(ToJson(c));

    j["defaultChords"] = JJson::array();
    for (const FHotkeyChord& c : command.defaultChords)
        j["defaultChords"].push_back(ToJson(c));

    return j;
}

bool FromJson(const JJson& j, FHotkeyCommand& outCommand)
{
    if (!j.is_object())
        return false;

    outCommand = FHotkeyCommand{};
    outCommand.name = j.value("name", "");
    outCommand.category = j.value("category", "");
    outCommand.description = j.value("description", "");

    if (j.contains("chords") && j["chords"].is_array())
    {
        for (const auto& jc : j["chords"])
        {
            FHotkeyChord c;
            if (FromJson(jc, c))
                outCommand.chords.push_back(std::move(c));
        }
    }

    if (j.contains("defaultChords") && j["defaultChords"].is_array())
    {
        for (const auto& jc : j["defaultChords"])
        {
            FHotkeyChord c;
            if (FromJson(jc, c))
                outCommand.defaultChords.push_back(std::move(c));
        }
    }

    // Backward/fallback behavior:
    // If file only has "chords", use them as defaults too.
    if (outCommand.defaultChords.empty() && !outCommand.chords.empty())
    {
        outCommand.defaultChords = outCommand.chords;
    }

    // If file only has defaults, use defaults as runtime active too.
    if (outCommand.chords.empty() && !outCommand.defaultChords.empty())
    {
        outCommand.chords = outCommand.defaultChords;
    }

    return !outCommand.name.empty();
}

// ---------------- FHotkeyMap ----------------

JJson ToJson(const FHotkeyMap& map)
{
    JJson j;
    j["version"] = kHotkeyJsonVersion;
    j["commands"] = JJson::array();

    for (const FHotkeyCommand& cmd : map.commands)
        j["commands"].push_back(ToJson(cmd));

    return j;
}

bool FromJson(const JJson& j, FHotkeyMap& outMap)
{
    if (!j.is_object())
        return false;

    outMap = FHotkeyMap{};

    // version currently unused but reserved
    const int version = j.value("version", 1);
    (void)version;

    if (!j.contains("commands") || !j["commands"].is_array())
        return false;

    for (const auto& jc : j["commands"])
    {
        FHotkeyCommand cmd;
        if (FromJson(jc, cmd))
            outMap.commands.push_back(std::move(cmd));
    }

    return true;
}

// ---------------- FHotkeyOverrideEntry ----------------

JJson ToJson(const FHotkeyOverrideEntry& entry)
{
    JJson j;
    j["commandName"] = entry.commandName;
    j["customChords"] = JJson::array();

    for (const FHotkeyChord& c : entry.customChords)
        j["customChords"].push_back(ToJson(c));

    return j;
}

bool FromJson(const JJson& j, FHotkeyOverrideEntry& outEntry)
{
    if (!j.is_object())
        return false;

    outEntry = FHotkeyOverrideEntry{};
    outEntry.commandName = j.value("commandName", "");
    if (outEntry.commandName.empty())
        return false;

    if (j.contains("customChords") && j["customChords"].is_array())
    {
        for (const auto& jc : j["customChords"])
        {
            FHotkeyChord c;
            if (FromJson(jc, c))
                outEntry.customChords.push_back(std::move(c));
        }
    }

    return true;
}

// ---------------- FHotkeyOverrides ----------------

JJson ToJson(const FHotkeyOverrides& overrides)
{
    JJson j;
    j["version"] = kHotkeyJsonVersion;
    j["entries"] = JJson::array();

    for (const FHotkeyOverrideEntry& e : overrides.entries)
        j["entries"].push_back(ToJson(e));

    return j;
}

bool FromJson(const JJson& j, FHotkeyOverrides& outOverrides)
{
    if (!j.is_object())
        return false;

    outOverrides = FHotkeyOverrides{};

    const int version = j.value("version", 1);
    (void)version;

    if (!j.contains("entries") || !j["entries"].is_array())
        return true; // valid empty overrides file

    for (const auto& je : j["entries"])
    {
        FHotkeyOverrideEntry e;
        if (FromJson(je, e))
            outOverrides.entries.push_back(std::move(e));
    }

    return true;
}

// ---------------- File helpers ----------------

bool SaveHotkeyMapToFile(const FHotkeyMap& map, const std::string& filePath)
{
    JsonWriter w;
    w.WriteObject("Hotkeys", ToJson(map));
    return w.SaveToFile(filePath);
}

bool LoadHotkeyMapFromFile(const std::string& filePath, FHotkeyMap& outMap)
{
    JsonReader r;
    if (!r.LoadFromFile(filePath))
        return false;

    // Supports either wrapped { "Hotkeys": ... } or raw root object
    if (r.IsObject("Hotkeys"))
        return FromJson(r.GetObject("Hotkeys").GetData(), outMap);

    return FromJson(r.GetData(), outMap);
}

bool SaveHotkeyOverridesToFile(const FHotkeyOverrides& overrides, const std::string& filePath)
{
    JsonWriter w;
    w.WriteObject("HotkeyOverrides", ToJson(overrides));
    return w.SaveToFile(filePath);
}

bool LoadHotkeyOverridesFromFile(const std::string& filePath, FHotkeyOverrides& outOverrides)
{
    JsonReader r;
    if (!r.LoadFromFile(filePath))
        return false;

    if (r.IsObject("HotkeyOverrides"))
        return FromJson(r.GetObject("HotkeyOverrides").GetData(), outOverrides);

    return FromJson(r.GetData(), outOverrides);
}