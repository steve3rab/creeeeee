#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/detail/utf8.hpp"
#include "creo/error.hpp"

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

  // Equivalent of Assign() for a UTF-8 string.
  void Assign(std::string_view utf8_text) {
    Assign(detail::FromUtf8(utf8_text));
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
  // ProTOOLKIT call like CREO_CHECK(ProMdlMdlNameGet(model, name.Raw()));
  std::wstring ToWString() const { return std::wstring(View()); }

  // Retrieves the buffer content converted to UTF-8. Choosing UTF-8 as
  // the std::string representation is deliberate: the size of wchar_t
  // (and therefore its implicit encoding) differs between Windows
  // (UTF-16) and Linux/macOS (UTF-32), and UTF-8 remains the only stable
  // representation on both sides (see creo/detail/utf8.hpp).
  std::string ToString() const { return detail::ToUtf8(View()); }

  // Raw buffer access, to pass directly to ProTOOLKIT C functions, e.g.
  // ProMdlMdlNameGet(model, name.Raw());
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
// a Creo Parametric model, as returned by ProMdlMdlNameGet (which
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

// Maximum number of assembly nesting levels supported by ProTOOLKIT
// (PRO_MAX_ASSEM_LEVEL). This is not a text buffer size, just a numeric
// limit — exposed here to sit alongside the wrapper's other ProTOOLKIT
// constants.
inline constexpr int MaxAssemLevel = detail::kMaxAssemLevel;

// Corresponds to `PRO_VALUE_UNUSED` (= -1): the "value not used"
// sentinel that many ProTOOLKIT functions accept in place of an explicit
// index or value (e.g. ProArrayObjectAdd: any negative index appends at
// the end of the array — PRO_VALUE_UNUSED is one example of that, not the
// only value that triggers this behavior).
inline constexpr int ValueUnused = detail::kValueUnused;

// Corresponds to `PRO_VALUE_DEFAULT` (= -5): the "default value"
// sentinel, distinct from PRO_VALUE_UNUSED despite the similar names — do
// not confuse the two in a ProTOOLKIT call.
inline constexpr int ValueDefault = detail::kValueDefault;

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

// ---------------------------------------------------------------------------
// ObjectType
// ---------------------------------------------------------------------------
// Corresponds to `ProType` (struct pro_obj_types, ProObjects.h): the
// broad Creo database object type — NOT just models. Models
// (part/assembly/drawing/manufacturing/...) are only a small part of it,
// alongside features, curves, simulation entities, mesh entities,
// animation entities, etc. Re-exposed as-is (direct alias, no RAII
// wrapper: it is a plain tagged integer).
//
// A few "model" values for reference: PRO_ASSEMBLY (1), PRO_PART (2),
// PRO_DRAWING (4), PRO_MFG (37), PRO_SUB_ASSEMBLY (34), PRO_DWGFORM (33),
// PRO_LAYOUT (19), PRO_REPORT (105), PRO_MARKUP (116), PRO_DIAGRAM (121).
using ObjectType = detail::RawObjectType;

// ---------------------------------------------------------------------------
// Boolean
// ---------------------------------------------------------------------------
// Corresponds to `ProBoolean`/`ProBool` (ProToolkit.h, enum ProBooleans:
// PRO_B_FALSE = 0, PRO_B_TRUE = 1): the ProTOOLKIT boolean, a type
// distinct from C++ bool even though its two values numerically coincide
// with false/true. Many ProTOOLKIT functions take or return precisely
// this type (not a C++ bool): Boolean is re-exposed as-is (direct alias,
// like ObjectType) to stay interoperable with them without a risky
// implicit conversion.
using Boolean = detail::RawBoolean;

// Explicit conversions with C++ bool: avoid writing `x == creo::Boolean{}`
// (hard to read) or casting by hand on every call. ToBool() tests
// `!= PRO_B_FALSE` rather than `== PRO_B_TRUE` out of defensive caution:
// nothing guarantees that a ProTOOLKIT function will only ever return one
// of the two documented values for a Boolean.
constexpr bool ToBool(Boolean value) noexcept {
  return value != detail::kBooleanFalse;
}
constexpr Boolean ToProBoolean(bool value) noexcept {
  return value ? detail::kBooleanTrue : detail::kBooleanFalse;
}

// ---------------------------------------------------------------------------
// ModelHandle
// ---------------------------------------------------------------------------
// Lightweight, non-owning wrapper around a `ProMdl`. The lifetime of a
// Creo model is managed by the ProTOOLKIT session itself (retrieve/erase
// go through dedicated ProTOOLKIT functions): this wrapper therefore does
// not do RAII over the model's lifetime. It simply provides a strong type
// (instead of the raw opaque handle) and an extension point for future
// wrapper methods (name, type, units, ...).
class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  // The model's name (ProMdlMdlNameGet, which replaces the now-deprecated
  // ProMdlNameGet in Creo 10). Convenience method: avoids rewriting the
  // CREO_CHECK + ModelName + .Raw() combo every time, already shown in
  // examples/hello_creo.cpp. Throws std::logic_error if the handle is
  // invalid (null), before even attempting the ProTOOLKIT call — a
  // clearer message than a generic PRO_TK_BAD_INPUTS coming back from the
  // SDK.
  ModelName Name() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Name() called on an invalid (null) handle");
    }
    ModelName name;
    CREO_CHECK(detail::MdlMdlNameGet(handle_, name.Raw()));
    return name;
  }

  // Raw handle, for direct calls to ProTOOLKIT functions not (yet)
  // wrapped by this library.
  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

// Two ModelHandle are equal if they refer to the same model (the same
// underlying ProTOOLKIT handle) — not if they share the same name, since
// two distinct models can share a generic name.
inline bool operator==(const ModelHandle &lhs, const ModelHandle &rhs) noexcept {
  return lhs.Raw() == rhs.Raw();
}
inline bool operator!=(const ModelHandle &lhs, const ModelHandle &rhs) noexcept {
  return !(lhs == rhs);
}

// ---------------------------------------------------------------------------
// ModelItem
// ---------------------------------------------------------------------------
// Corresponds to `pro_model_item` (ProObjects.h): a plain 3-field value
// struct {type, id, owner} that PTC gives roughly thirty different
// typedef names — ProGeomitem, ProFeature, ProDimension, ProNote,
// ProLayer, ... (see the aliases below) — one per kind of database
// object, even though they are bit-for-bit identical at the C level: a
// (type, id, owner) triple is how ProTOOLKIT identifies any database
// object within a model. Wrapped as a single class, exactly like PTC
// treats it as a single struct.
class ModelItem {
public:
  ModelItem() noexcept : raw_{} {}
  explicit ModelItem(const detail::RawModelItem &raw) noexcept : raw_(raw) {}

  ObjectType Type() const noexcept { return raw_.type; }
  int Id() const noexcept { return raw_.id; }
  ModelHandle Owner() const noexcept { return ModelHandle(raw_.owner); }

  // The item's name (ProModelitemNameGet). Returns a `Name` (ProName, 32
  // characters) — NOT a `ModelName` (ProMdlName, 180 characters):
  // ProModelitemNameGet names a database object (a feature, a dimension,
  // an explosion state, ...), which falls under "any other Creo
  // Parametric name" (see creo::Name), while ModelName/ProMdlName is
  // reserved specifically for the name of a whole ProMdl (see
  // ModelHandle::Name()). The two are easy to conflate since both are
  // ultimately "the name of a Pro-something", but mixing them up would
  // either truncate a wide name into too small a buffer, or waste space
  // — see the same warning on ModelName above.
  //
  // Named GetName(), not Name(): a member function named exactly like
  // the `creo::Name` type it returns does not compile (it shadows the
  // type within the class, including in its own return-type position).
  //
  // const_cast: ProModelitemNameGet takes a non-const `ProModelitem*` (a
  // pure "Get" function; this is a C API not being const-correct, not an
  // actual mutation of the item through that pointer).
  Name GetName() const {
    Name name;
    CREO_CHECK(detail::ModelitemNameGet(
        const_cast<detail::RawModelItem *>(&raw_), name.Raw()));
    return name;
  }

  // Raw struct, for direct calls to ProTOOLKIT functions not (yet)
  // wrapped by this library (most of them take a `ProModelitem*`).
  detail::RawModelItem *Raw() noexcept { return &raw_; }
  const detail::RawModelItem *Raw() const noexcept { return &raw_; }

private:
  detail::RawModelItem raw_;
};

// Two ModelItem are equal if they designate the same database object:
// same type, same id, same owning model — matching how ProTOOLKIT itself
// identifies a database object (a (type, id, owner) triple, not object
// identity of the C struct value).
inline bool operator==(const ModelItem &lhs, const ModelItem &rhs) noexcept {
  return lhs.Type() == rhs.Type() && lhs.Id() == rhs.Id() &&
         lhs.Owner() == rhs.Owner();
}
inline bool operator!=(const ModelItem &lhs, const ModelItem &rhs) noexcept {
  return !(lhs == rhs);
}

// PTC gives `pro_model_item` one typedef name per kind of database
// object; this wrapper mirrors that with one alias per name, all sharing
// the same ModelItem implementation (see the class comment above).
using GeomItem = ModelItem;
using ExtObj = ModelItem;
using Feature = ModelItem;
using ProcStep = ModelItem;
using SimpRep = ModelItem;
using ExpldState = ModelItem;
using Layer = ModelItem;
using Dimension = ModelItem;
using DtlNote = ModelItem;
using DtlSymInst = ModelItem;
using Gtol = ModelItem;
using CompDisp = ModelItem;
using DwgTable = ModelItem;
using Note = ModelItem;
using AnnotationElem = ModelItem;
using Annotation = ModelItem;
using AnnotationPlane = ModelItem;
using Symbol = ModelItem;
using SurfFinish = ModelItem;
using MechItem = ModelItem;
using MaterialItem = ModelItem;
using CombState = ModelItem;
using LayerState = ModelItem;
using ApprnState = ModelItem;
using SolidBody = ModelItem;
using Ply = ModelItem;
using Table = ModelItem;

} // namespace creo
