#pragma once
#include "Array.hpp"
#include "Error.hpp"
#include "ObjectType.hpp"
#include "ProtoolkitCompat.hpp"
#include "Text.hpp"

#include <optional>
#include <stdexcept>

namespace creo {

class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

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

  static Array<ModelHandle> List(MdlType type) {
    detail::RawMdl *raw_array = nullptr;
    int count = 0;
    CREO_CHECK(detail::SessionMdlList(type, &raw_array, &count));
    return Array<ModelHandle>::Adopt(
        static_cast<detail::RawArray>(raw_array));
  }

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  ModelName Name() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Name() called on an invalid (null) handle");
    }
    ModelName name;
    CREO_CHECK(detail::MdlMdlNameGet(handle_, name.Raw()));
    return name;
  }

  MdlType Type() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Type() called on an invalid (null) handle");
    }
    MdlType type;
    CREO_CHECK(detail::MdlTypeGet(handle_, &type));
    return type;
  }

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

  void Display() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Display() called on an invalid (null) "
          "handle");
    }
    CREO_CHECK(detail::MdlDisplay(handle_));
  }

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

  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

inline bool operator==(const ModelHandle &lhs,
                        const ModelHandle &rhs) noexcept {
  return lhs.Raw() == rhs.Raw();
}
inline bool operator!=(const ModelHandle &lhs,
                        const ModelHandle &rhs) noexcept {
  return !(lhs == rhs);
}

}
