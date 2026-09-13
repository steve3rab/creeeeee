#pragma once
#include "creo/detail/protoolkit_compat.hpp"

// PropertyUtils (a standalone Win32 utility, no creo:: namespace, no
// other dependency on this wrapper) is the single implementation of
// UTF-8 <-> wide string conversion used here: FixedWString's UTF-8
// interop below calls PropertyUtils::stringToWideString/
// wideStringToString directly rather than keeping a second,
// near-identical conversion of its own. This makes creo_wrapper depend
// on PropertyUtils (one-directional -- PropertyUtils itself still has
// no dependency on creo_wrapper), and it means malformed UTF-8 now
// throws (PropertyUtils' contract: MB_ERR_INVALID_CHARS/
// WC_ERR_INVALID_CHARS) instead of substituting U+FFFD.
#include "../../windows/PropertyUtils.hpp"

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

// ---------------------------------------------------------------------------
// FixedWString<N>
// ---------------------------------------------------------------------------
// Common base for ProTOOLKIT's fixed-size text buffers: ProName, ProLine
// and ProPath are all, on the C side, `wchar_t[N]` arrays. ProTOOLKIT
// functions expect a raw pointer to such a buffer, both as input and
// output; this wrapper therefore keeps the same memory layout (a plain
// array of N wchar_t, no indirection) while adding safe conversions
// to/from std::wstring and std::string (UTF-8), plus capacity checks that
// raw C lacks.
template <std::size_t N> class FixedWString {
public:
  static_assert(N > 0, "FixedWString requires a non-zero capacity");

  // Total buffer capacity, including the null terminator (matches
  // PRO_NAME_SIZE / PRO_LINE_SIZE / PRO_PATH_SIZE depending on the
  // instantiated type).
  static constexpr std::size_t kCapacity = N;

  // Maximum usable length, excluding the null terminator. It is this
  // value, not kCapacity, that bounds the size of a string accepted by
  // Assign()/the constructor — kCapacity includes the terminator.
  static constexpr std::size_t kMaxLength = N - 1;

  FixedWString() noexcept { buffer_[0] = L'\0'; }

  // Construction from a wide string (e.g. L"gear_01"). Throws
  // std::length_error if the string does not fit in the buffer: unlike C,
  // an overly long value is never silently truncated.
  explicit FixedWString(std::wstring_view text) { Assign(text); }

  // Construction from a std::string assumed to be UTF-8 encoded (e.g.
  // "gear_01"). Convenient to avoid writing wide literals (L"...")
  // everywhere on the caller side.
  explicit FixedWString(std::string_view utf8_text)
      : FixedWString(PropertyUtils::stringToWideString(std::string(utf8_text))) {}

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

  // Equivalent of Assign() for a UTF-8 string.
  void Assign(std::string_view utf8_text) {
    Assign(PropertyUtils::stringToWideString(std::string(utf8_text)));
  }

  // Effective length of the string (excluding the terminator), useful
  // after a ProTOOLKIT function has filled the buffer via Raw(). Looks up
  // the terminator via char_traits::find (backed by wmemchr), faster than
  // a manual loop on large buffers (ProPath, ProComment, ...). If the
  // buffer is not null-terminated within its kCapacity elements
  // (corrupted buffer, or improperly filled by the caller), falls back to
  // kCapacity rather than risking an out-of-bounds read.
  std::size_t Length() const noexcept {
    const wchar_t *end =
        std::char_traits<wchar_t>::find(buffer_, kCapacity, L'\0');
    return end != nullptr ? static_cast<std::size_t>(end - buffer_)
                           : kCapacity;
  }

  // Copy-free view of the current content. Useful for comparing or
  // passing the value to an API expecting a std::wstring_view without
  // paying for an allocation (ToWString() builds a new one on every
  // call).
  std::wstring_view View() const noexcept {
    return std::wstring_view(buffer_, Length());
  }

  // Retrieves the buffer content as-is (wide), typically after a
  // ProTOOLKIT call like CREO_CHECK(ProMdlMdlnameGet(model, name.Raw()));
  std::wstring ToWString() const { return std::wstring(View()); }

  // Retrieves the buffer content converted to UTF-8 via PropertyUtils
  // (windows/PropertyUtils.hpp) — a stable std::string representation
  // for logging, comparisons, or any API that does not want to deal
  // with wide strings. Throws if the buffer's content is not valid
  // UTF-16 (PropertyUtils' contract; see the note at the top of this
  // file) -- normally unreachable since the buffer only ever holds
  // what ProTOOLKIT itself wrote into it.
  std::string ToString() const {
    return PropertyUtils::wideStringToString(std::wstring(View()));
  }

  // Raw buffer access, to pass directly to ProTOOLKIT C functions, e.g.
  // ProMdlMdlnameGet(model, name.Raw());
  wchar_t *Raw() noexcept { return buffer_; }
  const wchar_t *Raw() const noexcept { return buffer_; }

  // Lets the object be passed directly wherever a `wchar_t*` /
  // `const wchar_t*` is expected, without an explicit call to Raw().
  operator wchar_t *() noexcept { return buffer_; }
  operator const wchar_t *() const noexcept { return buffer_; }

private:
  wchar_t buffer_[kCapacity];
};

// Content-based comparisons, between two FixedWString (same capacity or
// not — comparing a Name and a ModelName makes sense) or against a
// std::wstring_view (and thus also a std::wstring, implicitly converted).
// Without these operators, comparing two values would require going
// through .ToWString() on both sides every time.
//
// The `const wchar_t*` overloads are NOT redundant with the
// std::wstring_view ones: FixedWString also has an implicit operator
// wchar_t*() (needed for direct interop with ProTOOLKIT C functions).
// Without an exact `const wchar_t*` overload, comparing against a literal
// (`name == L"text"`) would be AMBIGUOUS for the compiler between
// converting `name` to a pointer and then comparing pointers (the
// built-in operator==, which compares ADDRESSES — the wrong result,
// silently) and converting the literal to a wstring_view (our operator==,
// which compares content): both conversions have the same rank. The
// exact overload resolves the ambiguity in favor of the correct,
// content-based comparison.
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

// ---------------------------------------------------------------------------
// FixedCharString<N>
// ---------------------------------------------------------------------------
// Some ProTOOLKIT buffers are NOT wchar_t: ProCharName, ProCharPath,
// ProCharLine as well as the "menu" types (ProMenuName, ProMenufileName,
// ProMenubuttonName) are `char[N]` on the C side. Since both sides
// (buffer and std::string conversion) are already char, there is no
// encoding story to handle here — unlike FixedWString, a plain copy is
// enough.
template <std::size_t N> class FixedCharString {
public:
  static_assert(N > 0, "FixedCharString requires a non-zero capacity");

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
// Exact `const char*` overloads: same reason as for FixedWString and
// `const wchar_t*` above (resolves the ambiguity with FixedCharString's
// implicit operator char*() when facing a "text" literal).
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

// Corresponds to `ProName` (size PRO_NAME_SIZE = 32): "any other Creo
// Parametric name" (feature, parameter, datum, ...) — PTC explicitly
// distinguishes this type from ProMdlName (see ModelName below), the two
// sizes have nothing in common.
using Name = FixedWString<detail::kNameSize>;

// Corresponds to `ProMdlName` (size PRO_MDLNAME_SIZE = 180): the name of
// a Creo Parametric model, as returned by ProMdlMdlnameGet (which
// replaces the now-deprecated ProMdlNameGet function in Creo 10).
// WARNING: PTC reserves a much larger size for ProMdlName than for
// ProName — this is NOT a plain alias of Name.
using ModelName = FixedWString<detail::kMdlNameSize>;

// Corresponds to `ProLine` (size PRO_LINE_SIZE = 81): a line of text, as
// used by ProTOOLKIT message functions (ProMessageDisplay,
// ProUILabelTextSet, ...). Note: despite its name, this is NOT a
// geometric segment in the ProTOOLKIT API — `ProLine` here really refers
// to a text buffer, not a curve primitive.
using Line = FixedWString<detail::kLineSize>;

// Corresponds to `ProPath` (size PRO_PATH_SIZE = 260): a file or
// directory path.
using Path = FixedWString<detail::kPathSize>;

// Conversions between Path and std::filesystem::path: Path specifically
// represents a path (unlike Name/Line/Comment/...), so this interop only
// makes sense for this particular alias, not for the generic
// FixedWString<N> template.
inline std::filesystem::path ToFilesystemPath(const Path &path) {
  return std::filesystem::path(path.View());
}
inline Path PathFromFilesystem(const std::filesystem::path &fs_path) {
  return Path(fs_path.wstring());
}

// Corresponds to `ProComment` (size PRO_COMMENT_SIZE = 256): comment text
// (feature, parameter, ...).
using Comment = FixedWString<detail::kCommentSize>;

// Corresponds to `ProValue` (size PRO_VALUE_SIZE = 256): the textual
// representation of a parameter's value.
using Value = FixedWString<detail::kValueSize>;

// Corresponds to `ProFeatrefKey` (size PRO_FEATREF_KEY_SIZE = 81): a
// feature reference key.
using FeatRefKey = FixedWString<detail::kFeatRefKeySize>;

// PRO_TYPE_SIZE, PRO_EXTENSION_SIZE and PRO_VERSION_SIZE only ever appear
// in PTC headers combined inside ProMdlFileName / ProFileName (see
// below): there is no standalone PTC typedef "ProType"/"ProExtension"/
// "ProVersion". The following three aliases are therefore utility
// building blocks specific to this wrapper (handy for constructing/
// decomposing a file name piece by piece), not the re-exposure of an
// existing PTC type. All three are worth 4, but only hold 3 useful
// characters plus the NULL terminator — as PTC's own comment explicitly
// states for PRO_EXTENSION_SIZE ("size 3; plus NULL terminator"), and the
// same goes for PRO_TYPE_SIZE ("prt"/"asm"/"drw": 3 letters) and
// PRO_VERSION_SIZE.
using ModelTypeCode = FixedWString<detail::kTypeSize>;    // "prt"/"asm"/"drw"
using Extension = FixedWString<detail::kExtensionSize>;   // generic ext.
using VersionSuffix = FixedWString<detail::kVersionSize>; // "name.ext.<ver>"

// Corresponds to `ProMdlExtension` (size PRO_MDLEXTENSION_SIZE = 32): the
// file extension of a Creo Parametric model. This one, however, is a
// real PTC typedef.
using ModelExtension = FixedWString<detail::kMdlExtensionSize>;

// Corresponds to `ProMacro` (size PRO_MACRO_SIZE = 256). PTC note carried
// over here: this size is no longer a real limit for ProMacroLoad(), the
// typedef (and this alias) are only kept for compatibility.
using Macro = FixedWString<detail::kMacroSize>;

// ---------------------------------------------------------------------------
// Composite types (full file names)
// ---------------------------------------------------------------------------
// PTC builds these sizes by adding up the atomic sizes above (see
// detail/protoolkit_compat.hpp for the formulas): they cover a full file
// name "name.ext.#" or a family table instance "instance[generic]".

// Corresponds to `ProMdlFileName` (PRO_FILE_MDLNAME_SIZE): the full file
// name of a Creo Parametric model, in "name.ext.#" format.
using MdlFileName = FixedWString<detail::kFileMdlNameSize>;

// Corresponds to `ProFileName` (PRO_FILE_NAME_SIZE): a generic full file
// name, in "name.ext.#" format.
using FileName = FixedWString<detail::kFileNameSize>;

// Corresponds to `ProFamtabClmDesc` (PRO_FAMTAB_FIELDNAME_SIZE): a family
// table column description (same size as a ProPath).
using FamTabColumnDesc = FixedWString<detail::kFamTabFieldNameSize>;

// Corresponds to `ProFamilyMdlName` (PRO_FAMILY_MDLNAME_SIZE): the name
// of a family table instance for a model, in "instance[generic]" format.
using FamilyMdlName = FixedWString<detail::kFamilyMdlNameSize>;

// Corresponds to `ProFamilyName` (PRO_FAMILY_NAME_SIZE): the generic
// equivalent of FamilyMdlName, in "instance[generic]" format.
using FamilyName = FixedWString<detail::kFamilyNameSize>;

// Corresponds to `ProDisplayModelName`: a model's display name. PTC gives
// it the same size as ProFamilyMdlName (PRO_FAMILY_MDLNAME_SIZE).
using DisplayModelName = FixedWString<detail::kFamilyMdlNameSize>;

// ---------------------------------------------------------------------------
// Char (non-wide) buffers: ProCharName, ProCharPath, ProCharLine and the
// "menu" types below are, unlike Name/Line/Path, `char[N]` on the C
// side — see FixedCharString above.
// ---------------------------------------------------------------------------

// Corresponds to `ProCharName` (PRO_NAME_SIZE): the char variant of Name.
using CharName = FixedCharString<detail::kNameSize>;

// Corresponds to `ProCharPath` (PRO_PATH_SIZE): the char variant of Path.
using CharPath = FixedCharString<detail::kPathSize>;

// Corresponds to `ProCharLine` (PRO_LINE_SIZE): a "message constant" on
// the PTC side, the char variant of Line.
using CharLine = FixedCharString<detail::kLineSize>;

// Corresponds to `ProMenuName` (PRO_NAME_SIZE): the name of a ProTOOLKIT
// menu.
using MenuName = FixedCharString<detail::kNameSize>;

// Corresponds to `ProMenufileName` (PRO_NAME_SIZE): a menu file name
// (.mnu).
using MenuFileName = FixedCharString<detail::kNameSize>;

// Corresponds to `ProMenubuttonName` (PRO_NAME_SIZE): the name of a menu
// button.
using MenuButtonName = FixedCharString<detail::kNameSize>;

} // namespace creo
