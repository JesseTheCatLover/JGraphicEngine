// EditorLayoutModel.h
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

enum class EEditorPanelKind : uint8_t
{
    MultiPanel,
    SinglePanel
};

enum class EEditorPanelType : uint8_t
{
    SceneHierarchy,
    Inspector,
    AssetBrowser,
    Console,
    Count // trick, to count the number of types
};

struct FEditorPanelDesc
{
    EEditorPanelKind kind;
    std::string_view key;    // stable, owned by model
    std::string_view name;   // human friendly
};

class EditorLayoutModel
{
private:
    int m_ViewportCount = 1;

    std::array<bool, (size_t)EEditorPanelType::Count> m_ToolVisible{};
    bool m_ViewportCountDirty = true;
    std::array<bool, (size_t)EEditorPanelType::Count> m_ToolDirty{};

public:
    void ResetToDefaults();

    // --- Viewports ---
    [[nodiscard]] int GetViewportCount() const { return m_ViewportCount; }
    void SetViewportCount(int count);

    // Descriptor for viewport i (0..3)
    [[nodiscard]] FEditorPanelDesc GetViewportDesc(int index) const;

    // --- Single Panels ---
    [[nodiscard]] FEditorPanelDesc GetSinglePanelDesc(EEditorPanelType id) const;

    [[nodiscard]] bool IsPanelVisible(EEditorPanelType id) const;
    void SetPanelVisible(EEditorPanelType id, bool bVisible);
    void TogglePanelVisibility(EEditorPanelType id);

    // Change tracking
    bool ConsumeViewportCountChanged();
    bool ConsumePanelVisibilityChanged(EEditorPanelType id);

private:
    static std::string_view ViewportKey(int index);
    static std::string_view ViewportName(int index);
};
