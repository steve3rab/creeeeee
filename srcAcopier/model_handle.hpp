#pragma once
#include "error.hpp"
#include "object_type.hpp"
#include "protoolkit_compat.hpp"
#include "text.hpp"

#include <stdexcept>

namespace creo {

class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

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
