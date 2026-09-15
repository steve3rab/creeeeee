#pragma once
// -----------------------------------------------------------------------
// creo::ModelHandle: a thin, non-owning holder around a raw ProTOOLKIT
// ProMdl handle.
//
// Deliberately NOT built like the rest of this small file set used to be
// (no detail::RawMdl alias, no trampoline functions, no shim/no-SDK
// fallback): every method below calls the real ProTOOLKIT C functions
// directly, by their real names, exactly as Creo's own headers declare
// them. The only two non-raw-C helpers used anywhere in this file are:
//   - CREO_CHECK (Error.hpp): turns a failing ProError into a thrown
//     creo::ProToolkitError, instead of every caller checking it by hand.
//   - creo::Defer/ScopeGuard (ScopeGuard.hpp): RAII rollback for
//     DisplayInNewWindow()'s multi-step, partially-reversible sequence.
// Everything else here is plain C++17 (std::wstring, std::invalid_argument,
// a plain class with plain methods) — no additional abstraction layer.
//
// Because of this, this header REQUIRES the real ProTOOLKIT SDK on the
// include path (ProToolkit.h, ProMdl.h, ProSolid.h, ProWindow.h,
// ProSizeConst.h) — there is no shim/portable fallback, unlike
// ProtoolkitCompat.hpp/ProtoolkitShim.hpp.
//
// Confidence per function, since some of this is new ground for this
// project (previous confirmations came from PTC docs/samples pasted
// earlier by the user; this batch was not pasted the same way):
//   - Confirmed earlier in this project (from pasted PTC documentation /
//     real sample code): ProMdlDisplay, ProMdlWindowGet, ProMdlCurrentGet,
//     ProMdlActiveGet, ProMdlMdlnameGet, ProMdlTypeGet, PRO_MDLNAME_SIZE,
//     PRO_MDL_UNUSED.
//   - NOT independently confirmed in this project (standard, widely
//     documented ProTOOLKIT functions per general knowledge of the API,
//     but not verified here against a pasted reference or real header):
//     ProWindowCreate, ProWindowCurrentSet, ProWindowRepaint,
//     ProWindowDelete, ProMdlSave, ProMdlRename, ProSolidRegenerate.
//     Double-check these signatures against your actual SDK's
//     ProWindow.h/ProMdl.h/ProSolid.h before relying on this in
//     production — in particular ProMdlRename's exact contract (does it
//     also rename the file on disk? does it require the model to be
//     unmodified/not displayed first?) is the one most worth checking.
// -----------------------------------------------------------------------

#include "Error.hpp"
#include "ScopeGuard.hpp"

#include <ProMdl.h>
#include <ProSizeConst.h>
#include <ProSolid.h>
#include <ProToolkit.h>
#include <ProWindow.h>

#include <stdexcept>
#include <string>

namespace creo {

class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(ProMdl handle) noexcept : handle_(handle) {}

  // ProMdlCurrentGet / ProMdlActiveGet: PTC's own distinction between
  // "current" and "active" model is not re-explained here, only wrapped.
  static ModelHandle Current() {
    ProMdl model = nullptr;
    CREO_CHECK(ProMdlCurrentGet(&model));
    return ModelHandle(model);
  }
  static ModelHandle Active() {
    ProMdl model = nullptr;
    CREO_CHECK(ProMdlActiveGet(&model));
    return ModelHandle(model);
  }

  ProMdl Raw() const noexcept { return handle_; }
  bool IsValid() const noexcept { return handle_ != nullptr; }

  // ProMdlMdlnameGet.
  std::wstring Name() const {
    EnsureValid();
    wchar_t buffer[PRO_MDLNAME_SIZE] = {};
    CREO_CHECK(ProMdlMdlnameGet(handle_, buffer));
    return std::wstring(buffer);
  }

  // ProMdlTypeGet.
  ProMdlType Type() const {
    EnsureValid();
    ProMdlType type = PRO_MDL_UNUSED;
    CREO_CHECK(ProMdlTypeGet(handle_, &type));
    return type;
  }

  // ProMdlWindowGet: the id of the window this model is currently
  // displayed in.
  int WindowId() const {
    EnsureValid();
    int window_id = 0;
    CREO_CHECK(ProMdlWindowGet(handle_, &window_id));
    return window_id;
  }

  // Displays the model in its current/active window. ProMdlDisplay.
  void Display() const {
    EnsureValid();
    CREO_CHECK(ProMdlDisplay(handle_));
  }

  // Creates a brand new window and displays this model in it, returning
  // the new window's id. ProWindowCreate/ProWindowCurrentSet/
  // ProMdlDisplay/ProWindowRepaint are each a separate step that can
  // fail independently once the window already exists; a ScopeGuard
  // deletes that window again if any later step throws, so a failed call
  // never leaves an orphaned, half-set-up window behind.
  int DisplayInNewWindow(const std::wstring &window_label) const {
    EnsureValid();
    int window_id = -1;
    CREO_CHECK(ProWindowCreate(const_cast<wchar_t *>(window_label.c_str()),
                                handle_, &window_id));

    auto destroy_window_on_failure =
        Defer([&] { ProWindowDelete(window_id); });

    CREO_CHECK(ProWindowCurrentSet(window_id));
    CREO_CHECK(ProMdlDisplay(handle_));
    CREO_CHECK(ProWindowRepaint(window_id));

    destroy_window_on_failure.Dismiss();
    return window_id;
  }

  // Forces the window this model is displayed in to redraw, for when
  // something changed the model without Creo's UI already knowing to
  // repaint on its own. ProMdlWindowGet + ProWindowRepaint.
  void ForceRefresh() const {
    EnsureValid();
    int window_id = 0;
    CREO_CHECK(ProMdlWindowGet(handle_, &window_id));
    CREO_CHECK(ProWindowRepaint(window_id));
  }

  // ProSolidRegenerate. `handle_` (ProMdl) is passed directly where the
  // real API expects a ProSolid: this assumes the two are interchangeable
  // (the same assumption already made and flagged elsewhere in this
  // project for ProFeatureRegenerate) -- if a real SDK's ProSolid turns
  // out to be a genuinely distinct, incompatible type, this is where the
  // build fails to tell you so, rather than silently doing the wrong
  // thing. `resolve_mode` mirrors PTC's own ProBoolean flag for
  // interactively resolving regeneration failures.
  void Regenerate(bool resolve_mode = false) const {
    EnsureValid();
    CREO_CHECK(ProSolidRegenerate(handle_,
                                   resolve_mode ? PRO_B_TRUE : PRO_B_FALSE));
  }

  // ProMdlSave.
  void Save() const {
    EnsureValid();
    CREO_CHECK(ProMdlSave(handle_));
  }

  // ProMdlRename. PTC's C API takes a plain (non-const) wchar_t* even for
  // an input-only string; new_name.c_str() is const_cast away only for
  // that reason, on the assumption the call does not itself write through
  // this pointer.
  void Rename(const std::wstring &new_name) const {
    EnsureValid();
    CREO_CHECK(ProMdlRename(handle_, const_cast<wchar_t *>(new_name.c_str())));
  }

private:
  void EnsureValid() const {
    if (handle_ == nullptr) {
      throw std::invalid_argument("ModelHandle: invalid (null) handle");
    }
  }

  ProMdl handle_;
};

} // namespace creo
