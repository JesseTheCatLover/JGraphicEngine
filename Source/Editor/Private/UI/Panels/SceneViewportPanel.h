#pragma once

#include <string>
#include "UI/IEditorPanels.h"

class EditorHost;
class EditorContext;

class SceneViewportPanel : public IEditorPanel
{
public:
    explicit SceneViewportPanel(int index)
    {
        m_PanelKey = "Viewport:" + std::to_string(index);

        char buf[64];
        snprintf(buf, sizeof(buf), "Viewport %d###Viewport_%d", index, index);
        m_WindowName = buf;
    }

    const char* GetName() const override { return m_WindowName.c_str(); }

    [[nodiscard]] const char* GetPanelKey() const override;

    void Draw(EditorHost& core) override;
    void OnCreate(EditorHost& core) override;
    void OnDestroy(EditorHost& core) override;

private:
    std::string m_PanelKey;
    std::string m_WindowName;
};
