//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <unordered_map>

#include "PanelRegistry.h"

template<typename TInput, typename TOutput, typename TController>
class TPanelChannel
{
public:
    using ControllerPtr = std::unique_ptr<TController>;
    using ControllerFactory = std::function<ControllerPtr(PanelID)>;

    explicit TPanelChannel(ControllerFactory factory)
        : m_Factory(std::move(factory))
    {}

    void SubmitInput(PanelID id, const TInput& input)
    {
        m_Inputs.insert_or_assign(id, input);

        if (m_Controllers.find(id) == m_Controllers.end())
        {
            m_Controllers.emplace(id, m_Factory(id));
        }
    }

    void Tick(float dt)
    {
        for (auto& [id, ctrl] : m_Controllers)
        {
            auto it = m_Inputs.find(id);
            if (it == m_Inputs.end())
                continue;

            // reuse output storage
            auto& out = m_Outputs[id];
            ctrl->Update(dt, it->second, out);
        }
    }

    const TOutput* GetOutput(PanelID id) const
    {
        auto it = m_Outputs.find(id);
        return (it != m_Outputs.end()) ? &it->second : nullptr;
    }

    void Destroy(PanelID id)
    {
        m_Controllers.erase(id);
        m_Inputs.erase(id);
        m_Outputs.erase(id);
    }

private:
    ControllerFactory m_Factory;

    std::unordered_map<PanelID, TInput> m_Inputs;
    std::unordered_map<PanelID, TOutput> m_Outputs;
    std::unordered_map<PanelID, ControllerPtr> m_Controllers;
};
