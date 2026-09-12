#pragma once
// -----------------------------------------------------------------------
// UTF-8 <-> wide string conversion, via the native Win32 API.
//
// ProTOOLKIT manipulates text as wchar_t. This wrapper targets Windows
// (wchar_t is 2 bytes, UTF-16, matching Creo itself): rather than
// hand-rolling UTF-8/UTF-16 codecs, ToUtf8()/FromUtf8() delegate directly
// to WideCharToMultiByte()/MultiByteToWideChar(), the platform's own
// conversion, keeping exactly one implementation of "what UTF-16 means"
// (the OS's) instead of a second one to keep in sync by hand.
//
// Both directions call the API without MB_ERR_INVALID_CHARS/
// WC_ERR_INVALID_CHARS: the text handled here may come from an external
// model file, so there is no reason to trust it by default, and this
// wrapper's contract is to substitute rather than throw on malformed
// input. Since Windows Vista, CP_UTF8 conversions without that flag
// substitute U+FFFD for invalid/unrepresentable sequences instead of
// failing outright — the behavior this wrapper wants, provided natively
// instead of reimplemented.
// -----------------------------------------------------------------------

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

// Encodes a wide string (as returned by a ProTOOLKIT ProName/ProLine/
// ProPath buffer) to UTF-8.
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

// Decodes a UTF-8 string to a wide string, to build a ProTOOLKIT buffer
// (ProName/ProLine/ProPath) from a std::string.
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

} // namespace creo::detail
