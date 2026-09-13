#pragma once
#include "ProtoolkitCompat.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

using ErrorCode = detail::ProErrorCode;

class ProToolkitError : public std::runtime_error {
public:
  ProToolkitError(ErrorCode code, std::string_view context);

  ErrorCode code() const noexcept { return code_; }

private:
  ErrorCode code_;
};

std::string ToString(ErrorCode code);

inline void ThrowIfError(ErrorCode code, std::string_view context = {}) {
  if (code != detail::kNoError) {
    throw ProToolkitError(code, context);
  }
}

}

#define CREO_CHECK(expr) ::creo::ThrowIfError((expr), #expr)
