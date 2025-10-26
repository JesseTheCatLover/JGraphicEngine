//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <memory>

#include "EGraphicsAPI.h"
#include "IRenderBackend.h"
#include "Core/Memory/SmartPointers.h"

TUniquePtr<IRenderBackend> MakeBackend(EGraphicsAPI api);
