#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/error.hpp"
#include "creo/object_type.hpp"
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

  // The model currently active in the Creo session (ProMdlCurrentGet).
  // Throws a ProToolkitError if there is none (e.g. no model loaded, or
  // this is not running inside a real Creo session at all — shim builds
  // never have a current model). This wraps only ProMdlCurrentGet
  // itself: it does not fall back to the current window's model, nor
  // substitute anything for an active sub-solid of an assembly — those
  // are application-level policies, not something this library should
  // decide on your behalf. Build that fallback in your own code on top
  // of this method if you need it, the way userMdlCurrentGet-style
  // helpers commonly do in ProTOOLKIT applications.
  //
  // Defensive null check: confirmed by testing against this wrapper's
  // own real-SDK-mode test stand-in for ProMdlCurrentGet, some ProTOOLKIT
  // "current X" getters can report success with a null/sentinel result
  // rather than failing outright when there is no current X. Treated the
  // same as an outright failure here, as PRO_TK_E_NOT_FOUND (the code
  // that already means exactly this in the real ProError enum) — not a
  // fabricated code, just this wrapper's own choice of which existing
  // code to attach to a case the raw call itself did not flag as an
  // error. Without this check, the caller would get an invalid
  // ModelHandle out of GetCurrent() with no exception at all, only
  // finding out from a confusing failure at the next call that uses it.
  static ModelHandle GetCurrent() {
    detail::RawMdl raw_mdl = nullptr;
    CREO_CHECK(detail::MdlCurrentGet(&raw_mdl));
    if (raw_mdl == nullptr) {
      throw ProToolkitError(static_cast<ErrorCode>(-4), // PRO_TK_E_NOT_FOUND
                             "ProMdlCurrentGet (no current model)");
    }
    return ModelHandle(raw_mdl);
  }

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  // The model's name (ProMdlMdlnameGet, which replaces the now-deprecated
  // ProMdlNameGet in Creo 10 -- note the lowercase "n" in "Mdlname", see
  // the note on this trampoline in detail/protoolkit_compat.hpp).
  // Convenience method: avoids rewriting the CREO_CHECK + ModelName +
  // .Raw() combo every time, already shown in examples/hello_creo.cpp.
  // Throws std::logic_error if the handle is invalid (null), before even
  // attempting the ProTOOLKIT call — a clearer message than a generic
  // PRO_TK_BAD_INPUTS coming back from the SDK.
  ModelName Name() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Name() called on an invalid (null) handle");
    }
    ModelName name;
    CREO_CHECK(detail::MdlMdlNameGet(handle_, name.Raw()));
    return name;
  }

  // The model's type (ProMdlTypeGet) — e.g. MdlType::PRO_MDL_ASSEMBLY,
  // MdlType::PRO_MDL_PART. Same invalid-handle guard and rationale as
  // Name() above.
  MdlType Type() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Type() called on an invalid (null) handle");
    }
    MdlType type;
    CREO_CHECK(detail::MdlTypeGet(handle_, &type));
    return type;
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
