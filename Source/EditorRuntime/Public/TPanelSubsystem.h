//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "TPanelChannel.h"

template<typename TInput, typename TOutput, typename TController>
class TPanelSubsystem
{
public:
    using Channel = TPanelChannel<TInput, TOutput, TController>;
    using ControllerFactory = typename Channel::ControllerFactory;

    explicit TPanelSubsystem(ControllerFactory factory)
        : m_Channel(std::move(factory))
    {}

    void SubmitInput(const TInput& input)
    {
        if (!input.panelKey || input.panelKey[0] == '\0')
            return;

        PanelID id = m_Registry.GetOrCreate(input.panelKey);
        m_Channel.SubmitInput(id, input);
    }

    const TOutput* GetOutput(const char* panelKey) const
    {
        if (!panelKey || panelKey[0] == '\0')
            return nullptr;

        PanelID id = m_Registry.Find(panelKey);
        if (id == UDynamicID::InvalidID)
            return nullptr;

        return m_Channel.GetOutput(id);
    }

    void Tick(float dt)
    {
        m_Channel.Tick(dt);
    }

    void Destroy(const char* panelKey)
    {
        if (!panelKey || panelKey[0] == '\0')
            return;

        PanelID id = m_Registry.Find(panelKey);
        if (id == UDynamicID::InvalidID)
            return;

        m_Channel.Destroy(id);
        m_Registry.Release(panelKey);
    }

private:
    PanelRegistry m_Registry;
    Channel m_Channel;
};
