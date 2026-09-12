#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace creo::detail {

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
