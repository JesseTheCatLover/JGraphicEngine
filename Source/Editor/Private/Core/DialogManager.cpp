//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "DialogManager.h"

#include "UI/IEditorDialog.h"

template<typename TDialog>
TDialog* DialogManager::FindDialogInstance()
{
    for (auto& dlg : m_Dialogs)
        if (auto* casted = dynamic_cast<TDialog*>(dlg.get()))
            return casted;
    return nullptr;
}

template<typename TDialog>
TDialog* DialogManager::OpenDialog()
{
    TDialog* instance = FindDialogInstance<TDialog>();
    if (!instance)
    {
        auto ptr = MakeUnique<TDialog>();
        instance = ptr.get();
        instance->OnCreate(m_Host, m_Runtime);
        m_Dialogs.emplace_back(std::move(ptr));
    }

    instance->OnOpen(m_Host, m_Runtime);
    return instance;
}

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
