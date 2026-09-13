#pragma once
#include "protoolkit_compat.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

namespace detail {

inline std::string ToUtf8(std::wstring_view text) {
  if (text.empty()) {
    return {};
  }
  if (text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
    throw std::length_error("text too long to convert to UTF-8");
  }
  const int wide_len = static_cast<int>(text.size());

  const int required = WideCharToMultiByte(CP_UTF8, 0, text.data(), wide_len,
                                            nullptr, 0, nullptr, nullptr);
  if (required <= 0) {
    throw std::runtime_error("WideCharToMultiByte failed");
  }

  std::string out(static_cast<std::size_t>(required), '\0');
  if (WideCharToMultiByte(CP_UTF8, 0, text.data(), wide_len, out.data(),
                          required, nullptr, nullptr) != required) {
    throw std::runtime_error("WideCharToMultiByte failed (second pass)");
  }
  return out;
}

inline std::wstring FromUtf8(std::string_view text) {
  if (text.empty()) {
    return {};
  }
  if (text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
    throw std::length_error("text too long to convert from UTF-8");
  }
  const int narrow_len = static_cast<int>(text.size());

  const int required =
      MultiByteToWideChar(CP_UTF8, 0, text.data(), narrow_len, nullptr, 0);
  if (required <= 0) {
    throw std::runtime_error("MultiByteToWideChar failed");
  }

  std::wstring out(static_cast<std::size_t>(required), L'\0');
  if (MultiByteToWideChar(CP_UTF8, 0, text.data(), narrow_len, out.data(),
                          required) != required) {
    throw std::runtime_error("MultiByteToWideChar failed (second pass)");
  }
  return out;
}

}

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
          "value too long for this ProTOOLKIT buffer (" +
          std::to_string(text.size()) + " characters, max capacity " +
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

  std::wstring_view View() const noexcept {
    return std::wstring_view(buffer_, Length());
  }

  std::wstring ToWString() const { return std::wstring(View()); }

  std::string ToString() const { return detail::ToUtf8(View()); }

  wchar_t *Raw() noexcept { return buffer_; }
  const wchar_t *Raw() const noexcept { return buffer_; }

  operator wchar_t *() noexcept { return buffer_; }
  operator const wchar_t *() const noexcept { return buffer_; }

private:
  wchar_t buffer_[kCapacity];
};

template <std::size_t N, std::size_t M>
bool operator==(const FixedWString<N> &lhs,
                const FixedWString<M> &rhs) noexcept {
  return lhs.View() == rhs.View();
}
template <std::size_t N, std::size_t M>
bool operator!=(const FixedWString<N> &lhs,
                const FixedWString<M> &rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator==(const FixedWString<N> &lhs, std::wstring_view rhs) noexcept {
  return lhs.View() == rhs;
}
template <std::size_t N>
bool operator==(std::wstring_view lhs, const FixedWString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedWString<N> &lhs, std::wstring_view rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(std::wstring_view lhs, const FixedWString<N> &rhs) noexcept {
  return !(rhs == lhs);
}
template <std::size_t N>
bool operator==(const FixedWString<N> &lhs, const wchar_t *rhs) noexcept {
  return lhs.View() == std::wstring_view(rhs);
}
template <std::size_t N>
bool operator==(const wchar_t *lhs, const FixedWString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedWString<N> &lhs, const wchar_t *rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(const wchar_t *lhs, const FixedWString<N> &rhs) noexcept {
  return !(rhs == lhs);
}

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
          "value too long for this ProTOOLKIT buffer (" +
          std::to_string(text.size()) + " characters, max capacity " +
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

  std::string_view View() const noexcept {
    return std::string_view(buffer_, Length());
  }

  std::string ToString() const { return std::string(View()); }

  char *Raw() noexcept { return buffer_; }
  const char *Raw() const noexcept { return buffer_; }

  operator char *() noexcept { return buffer_; }
  operator const char *() const noexcept { return buffer_; }

private:
  char buffer_[kCapacity];
};

template <std::size_t N, std::size_t M>
bool operator==(const FixedCharString<N> &lhs,
                const FixedCharString<M> &rhs) noexcept {
  return lhs.View() == rhs.View();
}
template <std::size_t N, std::size_t M>
bool operator!=(const FixedCharString<N> &lhs,
                const FixedCharString<M> &rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator==(const FixedCharString<N> &lhs,
                std::string_view rhs) noexcept {
  return lhs.View() == rhs;
}
template <std::size_t N>
bool operator==(std::string_view lhs,
                const FixedCharString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedCharString<N> &lhs,
                std::string_view rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(std::string_view lhs,
                const FixedCharString<N> &rhs) noexcept {
  return !(rhs == lhs);
}
template <std::size_t N>
bool operator==(const FixedCharString<N> &lhs, const char *rhs) noexcept {
  return lhs.View() == std::string_view(rhs);
}
template <std::size_t N>
bool operator==(const char *lhs, const FixedCharString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedCharString<N> &lhs, const char *rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(const char *lhs, const FixedCharString<N> &rhs) noexcept {
  return !(rhs == lhs);
}

using Name = FixedWString<detail::kNameSize>;

using ModelName = FixedWString<detail::kMdlNameSize>;

using Line = FixedWString<detail::kLineSize>;

using Path = FixedWString<detail::kPathSize>;

inline std::filesystem::path ToFilesystemPath(const Path &path) {
  return std::filesystem::path(path.View());
}
inline Path PathFromFilesystem(const std::filesystem::path &fs_path) {
  return Path(fs_path.wstring());
}

using Comment = FixedWString<detail::kCommentSize>;

using Value = FixedWString<detail::kValueSize>;

using FeatRefKey = FixedWString<detail::kFeatRefKeySize>;

using ModelTypeCode = FixedWString<detail::kTypeSize>;
using Extension = FixedWString<detail::kExtensionSize>;
using VersionSuffix = FixedWString<detail::kVersionSize>;

using ModelExtension = FixedWString<detail::kMdlExtensionSize>;

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

}
