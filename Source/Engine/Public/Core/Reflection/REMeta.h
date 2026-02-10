//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

// ------------------------------------------------------------
// Raw meta storage
// ------------------------------------------------------------

struct REMeta
{
    std::string key;
    std::string value; // empty => flag
};

using REMetaList = std::vector<REMeta>;

inline void REAddMeta(REMetaList& out, std::string key, std::string value = {})
{
    out.push_back({ std::move(key), std::move(value) });
}

inline const char* REMetaFindValue(const REMetaList& meta, const char* key)
{
    if (!key) return nullptr;
    for (const auto& m : meta)
        if (m.key == key)
            return m.value.c_str();
    return nullptr;
}

inline bool REMetaHasFlag(const REMetaList& meta, const char* key)
{
    if (!key) return false;
    for (const auto& m : meta)
        if (m.key == key && m.value.empty())
            return true;
    return false;
}

// ------------------------------------------------------------
// Meta IDs (stable, internal) + schema (aliases, parsing)
// NOTE: Names/aliases can change later without touching engine code.
// ------------------------------------------------------------

enum class REMetaID : uint16_t
{
    // ---- Editor exposure (property edit/visible) ----
    HiddenInInspector,     // serialize normally, but editor UI should not show it
    EditAnywhere,          // default
    EditSchemaOnly,
    EditInstanceOnly,

    VisibleAnywhere,
    VisibleSchemaOnly,
    VisibleInstanceOnly,

    // ---- Script exposure ----
    Scriptable,            // ReadWrite to scripts
    ScriptReadonly,        // ReadOnly to scripts
    HiddenToScript,        // default

    // ---- Serialization flags ----
    Transient,
    SaveGame,
    SkipSerialization,

    // ---- Presentation ----
    Category,
    DisplayName,
    Tooltip,
    AdvancedDisplay,
    DisableCondition,      // (takes expression/key)
    HideCondition,         // (takes expression/key)
    NoResetToDefault,

    // ---- Numeric behavior ----
    Clamp,                 // (min,max)
    ClampMin,              // (min)
    ClampMax,              // (max)

    Range,                 // (min,max)
    RangeMin,              // (min)
    RangeMax,              // (max)

    Step,                  // (step)
    Units,                 // ("cm", "deg", etc)
    Multiple,              // (multiple)
    Precision,             // (digits)
};

struct REMetaSpec
{
    REMetaID id;
    std::vector<std::string> keys; // accepted names (aliases)
    bool takesValue;              // flag vs value
};

// Property-level resolved view for editor/tooling.
// This is NOT stored yet (MVP) — you can compute it on demand.

enum class REEditorVis : uint8_t
{
    Edit,
    Visible,
};

enum class REScriptVis : uint8_t
{
    Hidden,
    ReadOnly,
    ReadWrite,
};

enum class REEditorScope : uint8_t
{
    Anywhere,
    SchemaOnly,
    InstanceOnly,
};

struct REPropertyMetaResolved
{
    // Defaults you requested:
    // - visible everywhere
    // - writable (Edit)
    // - hidden to script
    REEditorVis   editorVis   = REEditorVis::Edit;
    REEditorScope editorScope = REEditorScope::Anywhere;

    REScriptVis scriptVis = REScriptVis::Hidden;

    bool bHiddenInInspector = false;

    // Presentation
    std::string category;
    std::string displayName;
    std::string tooltip;

    // Serialization flags
    bool bTransient = false;
    bool bSaveGame = false;
    bool bSkipSerialization = false;

    // Numeric behavior (optional)
    bool  bHasClamp = false;
    float clampMin = 0.0f;
    float clampMax = 0.0f;

    bool  bHasRange = false;
    float rangeMin = 0.0f;
    float rangeMax = 0.0f;

    bool  bHasStep = false;
    float step = 0.0f;

    bool  bHasMultiple = false;
    float multiple = 0.0f;

    bool bHasPrecision = false;
    int  precision = 0;

    std::string units;
};

class REMetaSchema
{
public:
    static const REMetaSchema& Get()
    {
        static REMetaSchema s;
        return s;
    }

    // Basic query
    bool Has(const REMetaList& meta, REMetaID id) const
    {
        const REMetaSpec* spec = FindSpec(id);
        if (!spec) return false;
        return FindAnyKey(meta, *spec) != nullptr;
    }

    bool GetString(const REMetaList& meta, REMetaID id, std::string& out) const
    {
        const REMetaSpec* spec = FindSpec(id);
        if (!spec) return false;
        const REMeta* m = FindAnyKey(meta, *spec);
        if (!m) return false;
        out = m->value;
        return true;
    }

    bool GetFloat1(const REMetaList& meta, REMetaID id, float& out) const
    {
        std::string s;
        if (!GetString(meta, id, s)) return false;
        return TryParseFloat(s.c_str(), out);
    }

    bool GetInt1(const REMetaList& meta, REMetaID id, int& out) const
    {
        std::string s;
        if (!GetString(meta, id, s)) return false;
        return TryParseInt(s.c_str(), out);
    }

    bool GetFloat2(const REMetaList& meta, REMetaID id, float& a, float& b) const
    {
        std::string s;
        if (!GetString(meta, id, s)) return false;
        return TryParseFloat2(s, a, b);
    }

    // Resolve into a tool-friendly struct (editor + scripts + serialization).
    // This is intentionally forgiving; if multiple contradictory tags exist,
    // the *last* one in the list wins.
    REPropertyMetaResolved ResolvePropertyMeta(const REMetaList& meta) const
    {
        REPropertyMetaResolved r;

        // ----- Editor exposure -----
        // Defaults: EditAnywhere
        // Visible* => read-only in editor
        // Edit*    => writable in editor
        // HiddenInInspector => participate in serialization but doesn't show up in the inspector
        r.bHiddenInInspector = Has(meta, REMetaID::HiddenInInspector);
        for (const REMeta& m : meta)
        {
            // EDIT
            if (KeyMatches(m.key, REMetaID::EditAnywhere))      { r.editorVis = REEditorVis::Edit;    r.editorScope = REEditorScope::Anywhere; }
            if (KeyMatches(m.key, REMetaID::EditSchemaOnly))    { r.editorVis = REEditorVis::Edit;    r.editorScope = REEditorScope::SchemaOnly; }
            if (KeyMatches(m.key, REMetaID::EditInstanceOnly))  { r.editorVis = REEditorVis::Edit;    r.editorScope = REEditorScope::InstanceOnly; }

            // VISIBLE
            if (KeyMatches(m.key, REMetaID::VisibleAnywhere))     { r.editorVis = REEditorVis::Visible; r.editorScope = REEditorScope::Anywhere; }
            if (KeyMatches(m.key, REMetaID::VisibleSchemaOnly))   { r.editorVis = REEditorVis::Visible; r.editorScope = REEditorScope::SchemaOnly; }
            if (KeyMatches(m.key, REMetaID::VisibleInstanceOnly)) { r.editorVis = REEditorVis::Visible; r.editorScope = REEditorScope::InstanceOnly; }
        }

        // ----- Script exposure -----
        // Defaults: HiddenToScript
        for (const REMeta& m : meta)
        {
            if (KeyMatches(m.key, REMetaID::HiddenToScript)) { r.scriptVis = REScriptVis::Hidden; }
            if (KeyMatches(m.key, REMetaID::ScriptReadonly)) { r.scriptVis = REScriptVis::ReadOnly; }
            if (KeyMatches(m.key, REMetaID::Scriptable))     { r.scriptVis = REScriptVis::ReadWrite; }
        }

        // ----- Serialization flags -----
        for (const REMeta& m : meta)
        {
            if (KeyMatches(m.key, REMetaID::Transient))         r.bTransient = true;
            if (KeyMatches(m.key, REMetaID::SaveGame))          r.bSaveGame = true;
            if (KeyMatches(m.key, REMetaID::SkipSerialization)) r.bSkipSerialization = true;
        }

        // ----- Presentation -----
        {
            std::string tmp;
            if (GetString(meta, REMetaID::Category, tmp))    r.category = tmp;
            if (GetString(meta, REMetaID::DisplayName, tmp)) r.displayName = tmp;
            if (GetString(meta, REMetaID::Tooltip, tmp))     r.tooltip = tmp;
            // AdvancedDisplay / conditions / NoResetToDefault are NIY in runtime tools
        }

        // ----- Numeric behavior -----
        // Clamp
        {
            float a = 0.f, b = 0.f;
            if (GetFloat2(meta, REMetaID::Clamp, a, b))
            {
                r.bHasClamp = true;
                r.clampMin = a;
                r.clampMax = b;
            }
            float v = 0.f;
            if (GetFloat1(meta, REMetaID::ClampMin, v)) { r.bHasClamp = true; r.clampMin = v; }
            if (GetFloat1(meta, REMetaID::ClampMax, v)) { r.bHasClamp = true; r.clampMax = v; }
        }

        // Range
        {
            float a = 0.f, b = 0.f;
            if (GetFloat2(meta, REMetaID::Range, a, b))
            {
                r.bHasRange = true;
                r.rangeMin = a;
                r.rangeMax = b;
            }
            float v = 0.f;
            if (GetFloat1(meta, REMetaID::RangeMin, v)) { r.bHasRange = true; r.rangeMin = v; }
            if (GetFloat1(meta, REMetaID::RangeMax, v)) { r.bHasRange = true; r.rangeMax = v; }
        }

        // Step / Units / Multiple / Precision
        {
            float v = 0.f;
            if (GetFloat1(meta, REMetaID::Step, v)) { r.bHasStep = true; r.step = v; }
            if (GetFloat1(meta, REMetaID::Multiple, v)) { r.bHasMultiple = true; r.multiple = v; }

            int p = 0;
            if (GetInt1(meta, REMetaID::Precision, p)) { r.bHasPrecision = true; r.precision = p; }

            std::string u;
            if (GetString(meta, REMetaID::Units, u)) r.units = u;
        }

        return r;
    }

private:
    REMetaSchema();
    std::vector<REMetaSpec> m_Specs;

    [[nodiscard]] const REMetaSpec* FindSpec(REMetaID id) const
    {
        for (const auto& s : m_Specs)
            if (s.id == id)
                return &s;
        return nullptr;
    }

    static const REMeta* FindAnyKey(const REMetaList& meta, const REMetaSpec& spec)
    {
        // "last wins" behavior: iterate forward but keep replacing
        const REMeta* found = nullptr;
        for (const auto& m : meta)
        {
            for (const auto& k : spec.keys)
            {
                if (m.key == k)
                {
                    found = &m;
                    break;
                }
            }
        }
        return found;
    }

    [[nodiscard]] bool KeyMatches(const std::string& key, REMetaID id) const
    {
        const REMetaSpec* s = FindSpec(id);
        if (!s) return false;
        for (const auto& k : s->keys)
            if (key == k)
                return true;
        return false;
    }

    static const char* SkipWS(const char* p)
    {
        while (p && *p && std::isspace((unsigned char)*p)) ++p;
        return p;
    }

    static bool TryParseFloat(const char* s, float& out)
    {
        if (!s) return false;
        s = SkipWS(s);

        // Allow optional surrounding quotes
        if (*s == '"') ++s;

        char* end = nullptr;
        const double v = std::strtod(s, &end);
        if (end == s)
            return false;

        // allow trailing 'f' or 'F'
        if (*end == 'f' || *end == 'F') ++end;

        end = const_cast<char*>(SkipWS(end));
        if (*end == '"') ++end;
        end = const_cast<char*>(SkipWS(end));

        if (*end != '\0')
            return false;

        out = (float)v;
        return true;
    }

    static bool TryParseInt(const char* s, int& out)
    {
        if (!s) return false;
        s = SkipWS(s);
        if (*s == '"') ++s;

        char* end = nullptr;
        long v = std::strtol(s, &end, 10);
        if (end == s)
            return false;

        end = const_cast<char*>(SkipWS(end));
        if (*end == '"') ++end;
        end = const_cast<char*>(SkipWS(end));
        if (*end != '\0')
            return false;

        out = (int)v;
        return true;
    }

    static bool TryParseFloat2(const std::string& s, float& a, float& b)
    {
        // Accept forms:
        //  "0,1"  "0, 1"  "0.f,1.f"  "(0,1)"  "[0,1]"
        std::string t = s;
        t.erase(std::remove_if(t.begin(), t.end(), [](unsigned char c){ return std::isspace(c) != 0; }), t.end());

        // strip wrappers
        auto strip_one = [&](char L, char R)
        {
            if (t.size() >= 2 && t.front() == L && t.back() == R)
            {
                t.erase(t.begin());
                t.pop_back();
            }
        };
        strip_one('"', '"');
        strip_one('(', ')');
        strip_one('[', ']');

        const size_t comma = t.find(',');
        if (comma == std::string::npos)
            return false;

        const std::string aS = t.substr(0, comma);
        const std::string bS = t.substr(comma + 1);

        float aa = 0.f, bb = 0.f;
        if (!TryParseFloat(aS.c_str(), aa)) return false;
        if (!TryParseFloat(bS.c_str(), bb)) return false;

        a = aa;
        b = bb;
        return true;
    }
};