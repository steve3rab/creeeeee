#pragma once

#include "Error.hpp"
#include "ScopeGuard.hpp"

#include <ProAssembly.h>
#include <ProMdl.h>
#include <ProSizeConst.h>
#include <ProSolid.h>
#include <ProToolkit.h>
#include <ProWindow.h>

#include <stdexcept>
#include <string>

namespace creo {

struct ModelInfo {
  std::wstring name;
  ProMdlType type;
  std::wstring path;
  int version;
  std::wstring device;
  std::wstring host;
  ProMdlType subclass;
  bool from_windchill;
};

class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(ProMdl handle) noexcept : handle_(handle) {}

  static ModelHandle CurrentVisible() {
    return ModelHandle(ResolveCurrentModel().model);
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

  bool IsModified() const {
    CheckExists();
    ProBoolean is_modified = PRO_B_FALSE;
    CREO_CHECK(ProMdlModificationVerify(handle_, &is_modified));
    return is_modified == PRO_B_TRUE;
  }

  ModelInfo Info() const {
    CheckExists();

    ModelInfo info{};

    wchar_t name_buffer[PRO_MDLNAME_SIZE] = {};
    CREO_CHECK(ProMdlMdlnameGet(handle_, name_buffer));
    info.name = name_buffer;

    CREO_CHECK(ProMdlTypeGet(handle_, &info.type));

    wchar_t path_buffer[PRO_PATH_SIZE] = {};
    CREO_CHECK(ProMdlDirectoryPathGet(handle_, path_buffer));
    info.path = path_buffer;

    wchar_t parsed_name[PRO_MDLNAME_SIZE] = {};
    wchar_t parsed_type[PRO_MDLEXTENSION_SIZE] = {};
    wchar_t device_buffer[PRO_NAME_SIZE] = {};
    wchar_t host_buffer[PRO_NAME_SIZE] = {};
    CREO_CHECK(ProFileMdlnameParse(name_buffer, parsed_name, parsed_type,
                                    &info.version, device_buffer, host_buffer,
                                    &info.subclass));
    info.device = device_buffer;
    info.host = host_buffer;
    info.from_windchill = !info.device.empty() && !info.host.empty();

    return info;
  }

  void Display() {
    CurrentModelContext ctx = ResolveCurrentModel();
    CREO_CHECK(ProMdlDisplay(ctx.model));
    CREO_CHECK(ProWindowRepaint(ctx.window_id));
    handle_ = ctx.model;
  }

  int DisplayInNewWindow(const std::wstring &window_label) {
    CurrentModelContext ctx = ResolveCurrentModel();

    int window_id = -1;
    CREO_CHECK(ProWindowCreate(const_cast<wchar_t *>(window_label.c_str()),
                                ctx.model, &window_id));

    auto destroy_window_on_failure = Defer([&] { ProWindowDelete(window_id); });

    CREO_CHECK(ProWindowCurrentSet(window_id));
    CREO_CHECK(ProMdlDisplay(ctx.model));
    CREO_CHECK(ProWindowRepaint(window_id));

    destroy_window_on_failure.Dismiss();
    handle_ = ctx.model;
    return window_id;
  }

  void ForceRefresh() {
    CurrentModelContext ctx = ResolveCurrentModel();
    CREO_CHECK(ProWindowRepaint(ctx.window_id));
    handle_ = ctx.model;
  }

  void Regenerate(bool resolve_mode = false) {
    CurrentModelContext ctx = ResolveCurrentModel();
    CREO_CHECK(ProSolidRegenerate(ctx.model,
                                   resolve_mode ? PRO_B_TRUE : PRO_B_FALSE));
    CREO_CHECK(ProWindowRepaint(ctx.window_id));
    handle_ = ctx.model;
  }

  void Save() {
    CurrentModelContext ctx = ResolveCurrentModel();
    CREO_CHECK(ProMdlSave(ctx.model));
    handle_ = ctx.model;
  }

  void Rename(const std::wstring &new_name) {
    CurrentModelContext ctx = ResolveCurrentModel();

    wchar_t old_name_buffer[PRO_MDLNAME_SIZE] = {};
    CREO_CHECK(ProMdlMdlnameGet(ctx.model, old_name_buffer));
    std::wstring old_name(old_name_buffer);

    CREO_CHECK(
        ProMdlnameRename(ctx.model, const_cast<wchar_t *>(new_name.c_str())));

    auto restore_name_on_failure = Defer([&] {
      ProMdlnameRename(ctx.model, const_cast<wchar_t *>(old_name.c_str()));
    });

    CREO_CHECK(ProWindowRepaint(ctx.window_id));

    restore_name_on_failure.Dismiss();
    handle_ = ctx.model;
  }

private:
  struct CurrentModelContext {
    ProMdl model;
    int window_id;
  };

  void CheckExists() const {
    if (handle_ == nullptr) {
      throw std::invalid_argument("ModelHandle: invalid (null) handle");
    }
    CREO_CHECK(ProMdlVerify(handle_));
  }

  static int CheckCurrentWindow() {
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

  static CurrentModelContext ResolveCurrentModel() {
    int window_id = CheckCurrentWindow();

    ProMdl model = nullptr;
    CREO_CHECK(ProMdlCurrentGet(&model));
    if (model == nullptr) {
      CREO_CHECK(ProWindowMdlGet(window_id, &model));
    }
    if (model == nullptr) {
      throw std::runtime_error("ModelHandle: no current model");
    }
    CREO_CHECK(ProMdlVerify(model));

    ProBoolean is_modified = PRO_B_FALSE;
    CREO_CHECK(ProMdlModificationVerify(model, &is_modified));

    int model_window_id = 0;
    CREO_CHECK(ProMdlWindowGet(model, &model_window_id));
    if (model_window_id != window_id) {
      throw std::runtime_error(
          "ModelHandle: current model is not visible in the current window");
    }

    return {model, window_id};
  }

  ProMdl handle_;
};

}
