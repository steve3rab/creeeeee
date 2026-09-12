#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/error.hpp"
#include "creo/text.hpp"

#include <stdexcept>

namespace creo {

// ---------------------------------------------------------------------------
// ModelHandle
// ---------------------------------------------------------------------------
// Lightweight, non-owning wrapper around a `ProMdl`. The lifetime of a
// Creo model is managed by the ProTOOLKIT session itself (retrieve/erase
// go through dedicated ProTOOLKIT functions): this wrapper therefore does
// not do RAII over the model's lifetime. It simply provides a strong type
// (instead of the raw opaque handle) and an extension point for future
// wrapper methods (name, type, units, ...).
class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  // The model's name (ProMdlMdlNameGet, which replaces the now-deprecated
  // ProMdlNameGet in Creo 10). Convenience method: avoids rewriting the
  // CREO_CHECK + ModelName + .Raw() combo every time, already shown in
  // examples/hello_creo.cpp. Throws std::logic_error if the handle is
  // invalid (null), before even attempting the ProTOOLKIT call — a
  // clearer message than a generic PRO_TK_BAD_INPUTS coming back from the
  // SDK.
  ModelName Name() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Name() called on an invalid (null) handle");
    }
    ModelName name;
    CREO_CHECK(detail::MdlMdlNameGet(handle_, name.Raw()));
    return name;
  }

  // Raw handle, for direct calls to ProTOOLKIT functions not (yet)
  // wrapped by this library.
  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

// Two ModelHandle are equal if they refer to the same model (the same
// underlying ProTOOLKIT handle) — not if they share the same name, since
// two distinct models can share a generic name.
inline bool operator==(const ModelHandle &lhs, const ModelHandle &rhs) noexcept {
  return lhs.Raw() == rhs.Raw();
}
inline bool operator!=(const ModelHandle &lhs, const ModelHandle &rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace creo
