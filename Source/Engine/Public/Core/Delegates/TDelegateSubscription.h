//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/Delegates/FDelegateHandle.h"

template<typename TMulticast>
class TDelegateSubscription
{
private:
    TMulticast* m_Delegate = nullptr;
    FDelegateHandle m_Handle{};

public:
    TDelegateSubscription() = default;

    TDelegateSubscription(TMulticast* delegate, FDelegateHandle handle)
        : m_Delegate(delegate), m_Handle(handle) {}

    TDelegateSubscription(const TDelegateSubscription&) = delete;
    TDelegateSubscription& operator=(const TDelegateSubscription&) = delete;

    TDelegateSubscription(TDelegateSubscription&& other) noexcept
        : m_Delegate(other.m_Delegate), m_Handle(other.m_Handle)
    {
        other.m_Delegate = nullptr;
        other.m_Handle.Reset();
    }

    TDelegateSubscription& operator=(TDelegateSubscription&& other) noexcept
    {
        if (this == &other) return *this;
        Reset();
        m_Delegate = other.m_Delegate;
        m_Handle = other.m_Handle;
        other.m_Delegate = nullptr;
        other.m_Handle.Reset();
        return *this;
    }

    ~TDelegateSubscription()
    {
        Reset();
    }

    void Reset()
    {
        if (m_Delegate && m_Handle.IsValid())
            m_Delegate->Remove(m_Handle);

        m_Delegate = nullptr;
        m_Handle.Reset();
    }
};
