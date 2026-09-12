#pragma once
// -----------------------------------------------------------------------
// UTF-8 <-> wide string conversion, with no external dependency.
//
// ProTOOLKIT manipulates text as wchar_t, but the size (and therefore the
// implicit encoding) of wchar_t differs by platform:
//   - 2 bytes, UTF-16 (with surrogate pairs) on Windows;
//   - 4 bytes, UTF-32 (one wchar_t = one code point) on Linux/macOS.
//
// `std::wstring_convert`/`codecvt_utf8<wchar_t>` only handles the second
// case correctly and is deprecated since C++17 anyway: these functions do
// the conversion by hand, correctly in both cases, to offer a stable
// std::string (UTF-8) representation regardless of the build platform.
//
// Both directions strictly validate their input (truncated/malformed
// UTF-8 sequences, isolated UTF-16 surrogates, code points outside the
// valid Unicode range) and substitute the U+FFFD replacement character
// rather than truncating or letting an invalid sequence through: the text
// handled here may come from an external model file, so there is no
// reason to trust it by default.
// -----------------------------------------------------------------------

#include <cstdint>
#include <string>
#include <string_view>

namespace creo::detail {

inline constexpr std::uint32_t kUnicodeReplacementChar = 0xFFFD;
inline constexpr std::uint32_t kUnicodeMaxCodepoint = 0x10FFFF;

inline void AppendUtf8(std::string &out, std::uint32_t codepoint) {
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

// Appends a code point to a wide string, splitting it into a UTF-16
// surrogate pair if needed (2-byte wchar_t). Centralizes this logic for
// FromUtf8() (both the normal path and the U+FFFD substitution).
inline void AppendWide(std::wstring &out, std::uint32_t codepoint) {
  if constexpr (sizeof(wchar_t) == 2) {
    if (codepoint > 0xFFFF) {
      codepoint -= 0x10000;
      out.push_back(static_cast<wchar_t>(0xD800 + (codepoint >> 10)));
      out.push_back(static_cast<wchar_t>(0xDC00 + (codepoint & 0x3FF)));
      return;
    }
  }
  out.push_back(static_cast<wchar_t>(codepoint));
}

// Encodes a wide string (as returned by a ProTOOLKIT ProName/ProLine/
// ProPath buffer) to UTF-8. `reserve` sizes the output for the worst case
// (4 UTF-8 bytes per wide unit) to avoid any reallocation during the
// conversion.
inline std::string ToUtf8(std::wstring_view text) {
  std::string out;
  out.reserve(text.size() * 4);

  if constexpr (sizeof(wchar_t) == 2) {
    // wchar_t = UTF-16 unit: recombine surrogate pairs, and replace any
    // isolated surrogate (a high one with no following low one, or a low
    // one with no preceding high one) with U+FFFD rather than producing
    // invalid UTF-8 that directly encodes a surrogate code point.
    for (std::size_t i = 0; i < text.size(); ++i) {
      std::uint32_t unit = static_cast<std::uint16_t>(text[i]);
      if (unit >= 0xD800 && unit <= 0xDBFF && i + 1 < text.size()) {
        std::uint32_t low = static_cast<std::uint16_t>(text[i + 1]);
        if (low >= 0xDC00 && low <= 0xDFFF) {
          AppendUtf8(out, 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00));
          ++i;
          continue;
        }
      }
      if (unit >= 0xD800 && unit <= 0xDFFF) {
        AppendUtf8(out, kUnicodeReplacementChar);
      } else {
        AppendUtf8(out, unit);
      }
    }
  } else {
    // wchar_t = direct code point (UTF-32).
    for (wchar_t ch : text) {
      auto codepoint = static_cast<std::uint32_t>(ch);
      if (codepoint > kUnicodeMaxCodepoint ||
          (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        AppendUtf8(out, kUnicodeReplacementChar);
      } else {
        AppendUtf8(out, codepoint);
      }
    }
  }
  return out;
}

// Decodes a UTF-8 string to a wide string, to build a ProTOOLKIT buffer
// (ProName/ProLine/ProPath) from a std::string. Rejects (replaces with
// U+FFFD) truncated sequences, invalid continuation bytes, overlong
// encodings (e.g. an ASCII byte re-encoded on 2 bytes), and code points
// outside the valid Unicode range or in the range reserved for UTF-16
// surrogates: a UTF-8 sequence must never directly encode 0xD800-0xDFFF.
inline std::wstring FromUtf8(std::string_view text) {
  std::wstring out;
  out.reserve(text.size());

  std::size_t i = 0;
  while (i < text.size()) {
    unsigned char c0 = static_cast<unsigned char>(text[i]);
    std::uint32_t codepoint = 0;
    std::size_t extra = 0;
    std::uint32_t min_codepoint = 0;

    if (c0 < 0x80) {
      codepoint = c0;
    } else if ((c0 & 0xE0) == 0xC0) {
      codepoint = c0 & 0x1F;
      extra = 1;
      min_codepoint = 0x80;
    } else if ((c0 & 0xF0) == 0xE0) {
      codepoint = c0 & 0x0F;
      extra = 2;
      min_codepoint = 0x800;
    } else if ((c0 & 0xF8) == 0xF0) {
      codepoint = c0 & 0x07;
      extra = 3;
      min_codepoint = 0x10000;
    } else {
      AppendWide(out, kUnicodeReplacementChar);
      ++i;
      continue;
    }

    if (i + extra >= text.size()) {
      AppendWide(out, kUnicodeReplacementChar);
      break;
    }

    bool valid = true;
    for (std::size_t k = 1; k <= extra; ++k) {
      unsigned char c = static_cast<unsigned char>(text[i + k]);
      if ((c & 0xC0) != 0x80) {
        valid = false;
        break;
      }
      codepoint = (codepoint << 6) | (c & 0x3F);
    }

    if (!valid || codepoint < min_codepoint ||
        codepoint > kUnicodeMaxCodepoint ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
      AppendWide(out, kUnicodeReplacementChar);
      ++i;
      continue;
    }

    i += extra + 1;
    AppendWide(out, codepoint);
  }
  return out;
}

} // namespace creo::detail
