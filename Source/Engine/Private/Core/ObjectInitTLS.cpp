//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/FObjectInitTLS.h"
#include "Core/FObjectInitializer.h"
#include <vector>
#include <cassert>

static thread_local std::vector<const FObjectInitializer*> g_InitStack;

const FObjectInitializer* FObjectInitTLS::Get()
{
    return g_InitStack.empty() ? nullptr : g_InitStack.back();
}

void FObjectInitTLS::Push(const FObjectInitializer& init)
{
    g_InitStack.push_back(&init);
}

void FObjectInitTLS::Pop()
{
    assert(!g_InitStack.empty());
    g_InitStack.pop_back();
}