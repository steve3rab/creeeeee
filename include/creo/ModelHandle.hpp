#pragma once
#include "creo/Array.hpp"
#include "creo/detail/ProtoolkitCompat.hpp"
#include "creo/Error.hpp"
#include "creo/ObjectType.hpp"
#include "creo/Text.hpp"

#include <optional>
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
  // "No current model" surfaces from the raw ProMdlCurrentGet call in
  // two different ways depending on the ProTOOLKIT build/version: some
  // report success with a null/sentinel result (confirmed by testing
  // against this wrapper's own real-SDK-mode test stand-in), others
  // return PRO_TK_E_NOT_FOUND outright as their ProError. TryGetCurrent()
  // treats both the same way, as "nothing there" (std::nullopt) rather
  // than an error — PRO_TK_E_NOT_FOUND already means exactly this in the
  // real ProError enum, so recognizing it here is not a fabricated
  // interpretation, just handling the code by its own documented
  // meaning. Any other failure (a real error CREO_CHECK would reject)
  // still throws. GetCurrent() then re-adds the outright-failure
  // behavior on top, for callers who consider "no current model"
  // exceptional rather than a normal case to check for.
  //
  // Without this handling, TryGetCurrent()/GetCurrent() would either
  // throw when a nullopt/false return was wanted, or hand back an
  // invalid ModelHandle with no signal at all, only surfacing as a
  // confusing failure at the next call that uses it.
  static std::optional<ModelHandle> TryGetCurrent() {
    detail::RawMdl raw_mdl = nullptr;
    detail::ProErrorCode err = detail::MdlCurrentGet(&raw_mdl);
    if (err == static_cast<ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
      return std::nullopt;
    }
    CREO_CHECK(err);
    if (raw_mdl == nullptr) {
      return std::nullopt;
    }
    return ModelHandle(raw_mdl);
  }

  static ModelHandle GetCurrent() {
    std::optional<ModelHandle> model = TryGetCurrent();
    if (!model.has_value()) {
      throw ProToolkitError(static_cast<ErrorCode>(-4), // PRO_TK_E_NOT_FOUND
                             "ProMdlCurrentGet (no current model)");
    }
    return *model;
  }

  // The model currently active in the Creo session (ProMdlActiveGet) --
  // a distinct PTC function and concept from GetCurrent()/
  // ProMdlCurrentGet above (e.g. across multiple windows, "current" and
  // "active" need not be the same model); this wrapper does not assert a
  // precise definition of the difference PTC intends, only that they are
  // two separate calls PTC exposes, wrapped separately here rather than
  // conflated into one. Same "not found" handling and rationale as
  // TryGetCurrent()/GetCurrent().
  static std::optional<ModelHandle> TryGetActive() {
    detail::RawMdl raw_mdl = nullptr;
    detail::ProErrorCode err = detail::MdlActiveGet(&raw_mdl);
    if (err == static_cast<ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
      return std::nullopt;
    }
    CREO_CHECK(err);
    if (raw_mdl == nullptr) {
      return std::nullopt;
    }
    return ModelHandle(raw_mdl);
  }

  static ModelHandle GetActive() {
    std::optional<ModelHandle> model = TryGetActive();
    if (!model.has_value()) {
      throw ProToolkitError(static_cast<ErrorCode>(-4), // PRO_TK_E_NOT_FOUND
                             "ProMdlActiveGet (no active model)");
    }
    return *model;
  }

  // All models of a given type currently loaded in the session
  // (ProSessionMdlList). PTC documents this as allocating a ProArray
  // that the caller must free with ProArrayFree() -- exactly the
  // ownership-transfer case Array<T>::Adopt() exists for. This works
  // because ModelHandle is trivially copyable (a single raw pointer, no
  // user-declared copy/move/destructor) and has the exact same layout as
  // the raw ProMdl the array actually holds, so reinterpreting the
  // returned ProMdl* as a ModelHandle* (what Adopt() does internally) is
  // valid -- Array<T>::Adopt()'s own static_assert enforces this even if
  // ModelHandle's implementation ever changed to break that assumption.
  static Array<ModelHandle> List(MdlType type) {
    detail::RawMdl *raw_array = nullptr;
    int count = 0;
    CREO_CHECK(detail::SessionMdlList(type, &raw_array, &count));
    return Array<ModelHandle>::Adopt(
        static_cast<detail::RawArray>(raw_array));
  }

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  // The model's name (ProMdlMdlnameGet, which replaces the now-deprecated
  // ProMdlNameGet in Creo 10 -- note the lowercase "n" in "Mdlname", see
  // the note on this trampoline in detail/ProtoolkitCompat.hpp).
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

  // The model's file extension (ProMdlExtensionGet). Same invalid-handle
  // guard and rationale as Name() above.
  ModelExtension Extension() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Extension() called on an invalid (null) "
          "handle");
    }
    ModelExtension ext;
    CREO_CHECK(detail::MdlExtensionGet(handle_, ext.Raw()));
    return ext;
  }

  // The directory the model will be saved to (ProMdlDirectoryPathGet).
  // Same invalid-handle guard and rationale as Name() above.
  Path DirectoryPath() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::DirectoryPath() called on an invalid (null) "
          "handle");
    }
    Path dir_path;
    CREO_CHECK(detail::MdlDirectoryPathGet(handle_, dir_path.Raw()));
    return dir_path;
  }

  // Displays the model in its window (ProMdlDisplay). Same invalid-handle
  // guard and rationale as Name() above.
  void Display() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Display() called on an invalid (null) "
          "handle");
    }
    CREO_CHECK(detail::MdlDisplay(handle_));
  }

  // The identifier of the window this (top-level) model is shown in
  // (ProMdlWindowGet). A plain int, like ProTOOLKIT itself: this wrapper
  // does not (yet) have a dedicated window handle type. Same
  // invalid-handle guard and rationale as Name() above.
  int WindowId() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::WindowId() called on an invalid (null) "
          "handle");
    }
    int window_id = 0;
    CREO_CHECK(detail::MdlWindowGet(handle_, &window_id));
    return window_id;
  }

  // Whether this assembly is currently exploded (ProAssemblyIsExploded),
  // Explode()/Unexplode() it (ProAssemblyExplode/ProAssemblyUnexplode) --
  // confirmed from a real ProTOOLKIT sample (pasted verbatim by the
  // user: ProTestAsmExplode(), which checks IsExploded() before calling
  // either one, exactly like these three are meant to be used together).
  // Same invalid-handle guard and rationale as Name()/Type()/etc. above.
  // `assembly` is passed as a RawMdl: the sample itself casts a ProMdl*
  // straight to ProAssembly (`ProAssembly assembly = *(ProAssembly
  // *)mdl;`), direct evidence of the interchangeability already assumed
  // elsewhere (e.g. Feature::Regenerate()'s ProSolid/ProMdl caveat).
  bool IsExploded() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::IsExploded() called on an invalid (null) "
          "handle");
    }
    detail::RawBoolean is_exploded = detail::kBooleanFalse;
    CREO_CHECK(detail::AssemblyIsExploded(handle_, &is_exploded));
    return ToBool(is_exploded);
  }

  void Explode() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Explode() called on an invalid (null) "
          "handle");
    }
    CREO_CHECK(detail::AssemblyExplode(handle_));
  }

  void Unexplode() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Unexplode() called on an invalid (null) "
          "handle");
    }
    CREO_CHECK(detail::AssemblyUnexplode(handle_));
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
