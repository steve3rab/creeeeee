#pragma once

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

  static ModelHandle Current() {
    ProMdl model = nullptr;
    CREO_CHECK(ProMdlCurrentGet(&model));
    if (model == nullptr) {
      throw std::runtime_error("ModelHandle::Current: no current model");
    }
    return ModelHandle(model);
  }

  static ModelHandle Active() {
    ProMdl model = nullptr;
    CREO_CHECK(ProMdlActiveGet(&model));
    if (model == nullptr) {
      throw std::runtime_error("ModelHandle::Active: no active model");
    }
    return ModelHandle(model);
  }

  ProMdl Raw() const noexcept { return handle_; }
  bool IsValid() const noexcept { return handle_ != nullptr; }

  std::wstring Name() const {
    CheckExists();
    wchar_t buffer[PRO_MDLNAME_SIZE] = {};
    CREO_CHECK(ProMdlMdlnameGet(handle_, buffer));
    return std::wstring(buffer);
  }

  ProMdlType Type() const {
    CheckExists();
    ProMdlType type = PRO_MDL_UNUSED;
    CREO_CHECK(ProMdlTypeGet(handle_, &type));
    return type;
  }

  int WindowId() const {
    CheckExists();
    return CheckDisplayed();
  }

  void Display() const {
    CheckExists();
    int window_id = CheckCurrentWindow();
    CREO_CHECK(ProMdlDisplay(handle_));
    CREO_CHECK(ProWindowRepaint(window_id));
  }

  int DisplayInNewWindow(const std::wstring &window_label) const {
    CheckExists();
    int window_id = -1;
    CREO_CHECK(ProWindowCreate(const_cast<wchar_t *>(window_label.c_str()),
                                handle_, &window_id));

    auto destroy_window_on_failure = Defer([&] { ProWindowDelete(window_id); });

    CREO_CHECK(ProWindowCurrentSet(window_id));
    CREO_CHECK(ProMdlDisplay(handle_));
    CREO_CHECK(ProWindowRepaint(window_id));

    destroy_window_on_failure.Dismiss();
    return window_id;
  }

  void ForceRefresh() const {
    CheckExists();
    int window_id = CheckDisplayed();
    CREO_CHECK(ProWindowRepaint(window_id));
  }

  void Regenerate(bool resolve_mode = false) const {
    CheckExists();
    CREO_CHECK(
        ProSolidRegenerate(handle_, resolve_mode ? PRO_B_TRUE : PRO_B_FALSE));
    int window_id = 0;
    if (ProMdlWindowGet(handle_, &window_id) == PRO_TK_NO_ERROR) {
      ProWindowRepaint(window_id);
    }
  }

  void Save() const {
    CheckExists();
    CREO_CHECK(ProMdlSave(handle_));
  }

  void Rename(const std::wstring &new_name) {
    int window_id = CheckCurrentWindow();

    ProMdl model = nullptr;
    CREO_CHECK(ProMdlCurrentGet(&model));
    if (model == nullptr) {
      throw std::runtime_error("ModelHandle::Rename: no current model");
    }
    CREO_CHECK(ProMdlVerify(model));

    wchar_t old_name_buffer[PRO_MDLNAME_SIZE] = {};
    CREO_CHECK(ProMdlMdlnameGet(model, old_name_buffer));
    std::wstring old_name(old_name_buffer);

    CREO_CHECK(ProMdlRename(model, const_cast<wchar_t *>(new_name.c_str())));

    auto restore_name_on_failure = Defer([&] {
      ProMdlRename(model, const_cast<wchar_t *>(old_name.c_str()));
    });

    CREO_CHECK(ProWindowRepaint(window_id));

    restore_name_on_failure.Dismiss();
    handle_ = model;
  }

private:
  void CheckExists() const {
    if (handle_ == nullptr) {
      throw std::invalid_argument("ModelHandle: invalid (null) handle");
    }
    CREO_CHECK(ProMdlVerify(handle_));
  }

  int CheckCurrentWindow() const {
    int window_id = 0;
    CREO_CHECK(ProWindowCurrentGet(&window_id));
    if (window_id <= 0) {
      throw std::runtime_error("ModelHandle: no current window");
    }
    return window_id;
  }

  int CheckDisplayed() const {
    int window_id = 0;
    CREO_CHECK(ProMdlWindowGet(handle_, &window_id));
    if (window_id <= 0) {
      throw std::runtime_error("ModelHandle: model not visible in any window");
    }
    return window_id;
  }

  ProMdl handle_;
};

}
