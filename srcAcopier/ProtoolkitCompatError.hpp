#pragma once
// Minimal drop-in alternative to ProtoolkitCompat.hpp, scoped to exactly
// what Error.hpp/Error.cpp need: creo::detail::ProErrorCode (aliasing
// the real ProError) and creo::detail::kNoError (PRO_TK_NO_ERROR).
// Nothing else -- no Array/ModelHandle/ModelItem/Geometry support, so
// only <ProToolkit.h> is required, not the seven other PTC headers the
// full ProtoolkitCompat.hpp pulls in for those.
//
// Use this INSTEAD OF ProtoolkitCompat.hpp when dropping only Error.hpp/
// Error.cpp into a project (e.g. as the first, narrowest integration
// step): copy this file alongside them and point Error.hpp's
// #include "ProtoolkitCompat.hpp" at this file instead (or rename this
// file to ProtoolkitCompat.hpp in that project, since it is not also
// getting the full one). Real-SDK-only, like every other srcAcopier
// file: no shim mode, the real SDK is required to build.

#define CREO_WRAPPER_HAS_REAL_SDK 1

#include <ProToolkit.h>

namespace creo::detail {

using ProErrorCode = ::ProError;

inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

}
