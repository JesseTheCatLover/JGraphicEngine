//  Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include "Core/Reflection/ReflectMarkers.h"
#include "EProjectionType.generated.h"

JENUM()
enum class EProjectionType : uint8_t { Perspective, Orthographic };