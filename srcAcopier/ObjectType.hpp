#pragma once
#include "ProtoolkitCompat.hpp"

namespace creo {

using ObjectType = detail::RawObjectType;

using MdlType = detail::RawMdlType;

using Boolean = detail::RawBoolean;

constexpr bool ToBool(Boolean value) noexcept {
  return value != detail::kBooleanFalse;
}
constexpr Boolean ToProBoolean(bool value) noexcept {
  return value ? detail::kBooleanTrue : detail::kBooleanFalse;
}

}
