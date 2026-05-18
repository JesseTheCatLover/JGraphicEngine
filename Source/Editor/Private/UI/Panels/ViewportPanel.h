#pragma once

#include <string>
#include "UI/IEditorPanel.h"

class EditorHost;
class EditorContext;

class ViewportPanel : public IEditorPanel
{
public:
    explicit ViewportPanel(int index)
    {
        m_PanelKey = "Viewport:" + std::to_string(index);

        char buf[64];
        snprintf(buf, sizeof(buf), "Viewport %d##Viewport%d", index, index);
        m_WindowName = buf;
    }

    void Draw(EditorHost& host) override;
    void OnCreate(EditorHost& host) override;
    void OnDestroy(EditorHost& host) override;

    const char* GetName() const override { return m_WindowName.c_str(); }

    [[nodiscard]] const char* GetPanelKey() const override;

    EPanelDockGroup GetDockGroup() const override { return EPanelDockGroup::Viewport; }

private:
    std::string m_PanelKey;
    std::string m_WindowName;
};
