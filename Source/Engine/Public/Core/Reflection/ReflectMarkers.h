//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

/*
===============================================================================
 Reflection / Codegen Markers
-------------------------------------------------------------------------------
 These macros are *semantic markers* used by the reflection code generator
 (JReflectGen). They intentionally expand to nothing in C++ and have zero
 runtime cost.

 Their sole purpose is to make engine intent explicit in headers so external
 tools can:
   - Discover reflected types, members, and metadata
   - Generate reflection data, factories, and serializers
   - Enable editor tooling, inspection, and scripting layers

 The C++ compiler ignores these completely.
===============================================================================
*/

// -----------------------------------------------------------------------------
// Type-level markers
// -----------------------------------------------------------------------------

// Marks a reflected engine class (actors, components, assets, etc.)
#define JCLASS(...)

// Marks a reflected struct (value types, math, POD-style data)
#define JSTRUCT(...)

// Marks a reflected enum
#define JENUM(...)

// -----------------------------------------------------------------------------
// Member-level markers
// -----------------------------------------------------------------------------

// Marks a reflected data member
// Participates in serialization, inspection, and editor UI
#define JPROPERTY(...)

// Marks a reflected member function
// Used for editor buttons, scripting, RPCs, or tooling hooks
#define JFUNCTION(...)

// -----------------------------------------------------------------------------
// Metadata & attributes
// -----------------------------------------------------------------------------

// Attaches metadata to the next reflected symbol
// Example:
//   JMETA(DisplayName="Field of View", Min=1, Max=179)
#define JMETA(...)

// -----------------------------------------------------------------------------
// Generated glue
// -----------------------------------------------------------------------------

// Injects generated reflection glue into the type declaration
// (StaticREType, GetREType override, helpers, etc.)
#define GENERATED_BODY()