//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "DialogManager.h"

#include "UI/IEditorDialog.h"

void DialogManager::DrawDialogs()
{
    for (auto it = m_Dialogs.begin(); it != m_Dialogs.end(); /* no increment */)
    {
        IEditorDialog* dlg = it->get();
        dlg->Draw(m_Host, m_Runtime);

        if (!dlg->IsOpen())
        {
            dlg->OnDestroy(m_Host, m_Runtime);
            it = m_Dialogs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
