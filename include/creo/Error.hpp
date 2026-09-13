#pragma once
#include "creo/detail/ProtoolkitCompat.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

// Native ProTOOLKIT error code (ProError), re-exposed as-is: the wrapper
// does not reinvent this enum, it relies on the PTC SDK's own one when
// available (see detail/ProtoolkitCompat.hpp).
using ErrorCode = detail::ProErrorCode;

// Exception thrown by the wrapper for every failing ProTOOLKIT call.
// Keeps the native error code around to allow fine-grained handling
// upstream (catching on a specific code), in addition to the standard
// human-readable message inherited from std::runtime_error.
class ProToolkitError : public std::runtime_error {
public:
  ProToolkitError(ErrorCode code, std::string_view context);

  ErrorCode code() const noexcept { return code_; }

private:
  ErrorCode code_;
};

// Converts a ProError to text, e.g. ToString(-2) -> "PRO_TK_BAD_INPUTS".
// Covers the entire official ProError/ProErr enum (ProError.h, Creo 10).
// For a code outside this list (a different Creo version, or an
// application-defined code), returns the raw number instead of making up
// a label — see src/error.cpp for the full table.
std::string ToString(ErrorCode code);

// Throws a ProToolkitError if `code` is not PRO_TK_NO_ERROR. `context`
// serves as a diagnostic hint (typically the name of the ProTOOLKIT call
// involved) and appears in the exception's message.
inline void ThrowIfError(ErrorCode code, std::string_view context = {}) {
  if (code != detail::kNoError) {
    throw ProToolkitError(code, context);
  }
}

} // namespace creo

// Calls `expr` (a ProTOOLKIT function returning a ProError) and
// automatically turns a failure into a creo::ProToolkitError, capturing
// the expression itself as diagnostic context. Usage:
//
//   CREO_CHECK(ProMdlMdlnameGet(model.Raw(), name.Raw()));
//
#define CREO_CHECK(expr) ::creo::ThrowIfError((expr), #expr)
