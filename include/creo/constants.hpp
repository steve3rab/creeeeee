#pragma once
#include "creo/detail/protoolkit_compat.hpp"

namespace creo {

// Maximum number of assembly nesting levels supported by ProTOOLKIT
// (PRO_MAX_ASSEM_LEVEL). This is not a text buffer size, just a numeric
// limit.
inline constexpr int MaxAssemLevel = detail::kMaxAssemLevel;

// Corresponds to `PRO_VALUE_UNUSED` (= -1): the "value not used"
// sentinel that many ProTOOLKIT functions accept in place of an explicit
// index or value (e.g. ProArrayObjectAdd: any negative index appends at
// the end of the array — PRO_VALUE_UNUSED is one example of that, not the
// only value that triggers this behavior).
inline constexpr int ValueUnused = detail::kValueUnused;

// Corresponds to `PRO_VALUE_DEFAULT` (= -5): the "default value"
// sentinel, distinct from PRO_VALUE_UNUSED despite the similar names — do
// not confuse the two in a ProTOOLKIT call.
inline constexpr int ValueDefault = detail::kValueDefault;

} // namespace creo
