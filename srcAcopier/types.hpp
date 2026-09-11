#pragma once
#include "protoolkit_compat.hpp"
#include "utf8.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

template <std::size_t N> class FixedWString {
public:
  static_assert(N > 0);

  static constexpr std::size_t kCapacity = N;
  static constexpr std::size_t kMaxLength = N - 1;

  FixedWString() noexcept { buffer_[0] = L'\0'; }

  explicit FixedWString(std::wstring_view text) { Assign(text); }

  explicit FixedWString(std::string_view utf8_text)
      : FixedWString(detail::FromUtf8(utf8_text)) {}

  void Assign(std::wstring_view text) {
    if (text.size() > kMaxLength) {
      throw std::length_error(
          "valeur trop longue pour ce buffer ProTOOLKIT (" +
          std::to_string(text.size()) + " caractères, capacité max " +
          std::to_string(kMaxLength) + ")");
    }
    if (!text.empty()) {
      std::char_traits<wchar_t>::copy(buffer_, text.data(), text.size());
    }
    buffer_[text.size()] = L'\0';
  }

  void Assign(std::string_view utf8_text) {
    Assign(detail::FromUtf8(utf8_text));
  }

  std::size_t Length() const noexcept {
    const wchar_t *end =
        std::char_traits<wchar_t>::find(buffer_, kCapacity, L'\0');
    return end != nullptr ? static_cast<std::size_t>(end - buffer_)
                           : kCapacity;
  }

  std::wstring ToWString() const { return std::wstring(buffer_, Length()); }

  std::string ToString() const {
    return detail::ToUtf8(std::wstring_view(buffer_, Length()));
  }

  wchar_t *Raw() noexcept { return buffer_; }
  const wchar_t *Raw() const noexcept { return buffer_; }

  operator wchar_t *() noexcept { return buffer_; }
  operator const wchar_t *() const noexcept { return buffer_; }

private:
  wchar_t buffer_[kCapacity];
};

template <std::size_t N> class FixedCharString {
public:
  static_assert(N > 0);

  static constexpr std::size_t kCapacity = N;
  static constexpr std::size_t kMaxLength = N - 1;

  FixedCharString() noexcept { buffer_[0] = '\0'; }

  explicit FixedCharString(std::string_view text) { Assign(text); }

  void Assign(std::string_view text) {
    if (text.size() > kMaxLength) {
      throw std::length_error(
          "valeur trop longue pour ce buffer ProTOOLKIT (" +
          std::to_string(text.size()) + " caractères, capacité max " +
          std::to_string(kMaxLength) + ")");
    }
    if (!text.empty()) {
      std::char_traits<char>::copy(buffer_, text.data(), text.size());
    }
    buffer_[text.size()] = '\0';
  }

  std::size_t Length() const noexcept {
    const char *end = std::char_traits<char>::find(buffer_, kCapacity, '\0');
    return end != nullptr ? static_cast<std::size_t>(end - buffer_)
                           : kCapacity;
  }

  std::string ToString() const { return std::string(buffer_, Length()); }

  char *Raw() noexcept { return buffer_; }
  const char *Raw() const noexcept { return buffer_; }

  operator char *() noexcept { return buffer_; }
  operator const char *() const noexcept { return buffer_; }

private:
  char buffer_[kCapacity];
};

using Name = FixedWString<detail::kNameSize>;

using ModelName = FixedWString<detail::kMdlNameSize>;

using Line = FixedWString<detail::kLineSize>;

using Path = FixedWString<detail::kPathSize>;

using Comment = FixedWString<detail::kCommentSize>;

using Value = FixedWString<detail::kValueSize>;

using FeatRefKey = FixedWString<detail::kFeatRefKeySize>;

using ModelTypeCode = FixedWString<detail::kTypeSize>;
using Extension = FixedWString<detail::kExtensionSize>;
using VersionSuffix = FixedWString<detail::kVersionSize>;

using ModelExtension = FixedWString<detail::kMdlExtensionSize>;

inline constexpr int MaxAssemLevel = detail::kMaxAssemLevel;

using Macro = FixedWString<detail::kMacroSize>;

using MdlFileName = FixedWString<detail::kFileMdlNameSize>;

using FileName = FixedWString<detail::kFileNameSize>;

using FamTabColumnDesc = FixedWString<detail::kFamTabFieldNameSize>;

using FamilyMdlName = FixedWString<detail::kFamilyMdlNameSize>;

using FamilyName = FixedWString<detail::kFamilyNameSize>;

using DisplayModelName = FixedWString<detail::kFamilyMdlNameSize>;

using CharName = FixedCharString<detail::kNameSize>;

using CharPath = FixedCharString<detail::kPathSize>;

using CharLine = FixedCharString<detail::kLineSize>;

using MenuName = FixedCharString<detail::kNameSize>;

using MenuFileName = FixedCharString<detail::kNameSize>;

using MenuButtonName = FixedCharString<detail::kNameSize>;

using ObjectType = detail::RawObjectType;

class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

}
