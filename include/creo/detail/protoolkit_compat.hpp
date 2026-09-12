#pragma once
// -----------------------------------------------------------------------
// Single entry point to the native ProTOOLKIT types.
//
// Two build modes:
//   1) Real SDK present (Creo 10 installed + the CREO_TOOLKIT_ROOT
//      CMake/environment variable set, see cmake/FindProToolkit.cmake):
//      the PTC headers are included directly and their types are
//      re-exposed as-is. This is the mode to use for any build meant to
//      run inside a Creo session.
//   2) SDK absent: falls back to the minimal shim (protoolkit_shim.hpp)
//      so the wrapper can compile and be tested away from a Creo
//      workstation.
//
// The rest of the wrapper (creo::Name, creo::Line, creo::ModelHandle,
// creo::ProToolkitError, ...) only depends on the aliases defined here
// (creo::detail::RawMdl, creo::detail::k*Size) and therefore never needs
// to know which mode it was built in.
// -----------------------------------------------------------------------

#if defined(__has_include)
#if __has_include(<ProToolkit.h>)
#define CREO_WRAPPER_HAS_REAL_SDK 1
#endif
#endif
#ifndef CREO_WRAPPER_HAS_REAL_SDK
#define CREO_WRAPPER_HAS_REAL_SDK 0
#endif

#if CREO_WRAPPER_HAS_REAL_SDK
// Official PTC headers, shipped with the Creo 10 ProTOOLKIT SDK. Not
// redistributed in this repository: see the README for where to find
// them. ProSizeConst.h (the PRO_*_SIZE constants) is a self-contained
// header on the PTC side (its own include guard, only depending on
// ProToolkit.h for PRO_BEGIN_C_DECLS/PRO_END_C_DECLS): nothing guarantees
// it is included transitively by the headers below, so it is included
// explicitly instead of relying on that.
#include <ProArray.h>
#include <ProMdl.h>
#include <ProObjects.h>
#include <ProSizeConst.h>
#include <ProToolkit.h>
#else
#include "creo/detail/protoolkit_shim.hpp"
#endif

namespace creo::detail {

#if CREO_WRAPPER_HAS_REAL_SDK

// Error code and model handle: the only raw PTC types the wrapper
// actually needs (text buffers are rebuilt by FixedWString<N>/
// FixedCharString<N> from the sizes below, not reused directly).
using ProErrorCode = ::ProError;
using RawMdl = ::ProMdl;
using RawObjectType = ::ProType;
using RawArray = ::ProArray;
using RawBoolean = ::ProBoolean;
using RawModelItem = ::ProModelitem;

// Trampolines to the real ProArray functions: creo::Array<T> (see
// creo/array.hpp) calls these aliases, never ::ProArrayXxx nor
// shim::ProArrayXxx directly, to stay mode-independent. Array<T> only
// uses ProArray as a raw memory provider (Alloc/Free) — not its native
// shifting/resizing functions, which move elements by raw memory copy
// and would break any T that is not trivially copyable; see the comment
// at the top of array.hpp. SizeGet is still needed for Adopt().
// MaxCountGet is still exposed as a standalone utility
// (creo::MaxArrayCount<T>()).
inline ProErrorCode ArrayAlloc(int n_objs, int obj_size,
                                int reallocation_size, RawArray *p_array) {
  return ::ProArrayAlloc(n_objs, obj_size, reallocation_size, p_array);
}
inline ProErrorCode ArrayFree(RawArray *p_array) {
  return ::ProArrayFree(p_array);
}
inline ProErrorCode ArraySizeGet(RawArray array, int *p_size) {
  return ::ProArraySizeGet(array, p_size);
}
inline ProErrorCode ArrayMaxCountGet(int obj_size, int *max_num_objs) {
  return ::ProArrayMaxCountGet(obj_size, max_num_objs);
}

// Trampoline to ProMdlMdlNameGet (see ModelHandle::Name() in types.hpp),
// which replaces the now-deprecated ProMdlNameGet in Creo 10.
inline ProErrorCode MdlMdlNameGet(RawMdl model, wchar_t *name_out) {
  return ::ProMdlMdlNameGet(model, name_out);
}

// Trampoline to ProMdlCurrentGet (see ModelHandle::GetCurrent() in
// model_handle.hpp).
inline ProErrorCode MdlCurrentGet(RawMdl *p_mdl) {
  return ::ProMdlCurrentGet(p_mdl);
}

// Trampoline to ProModelitemNameGet (see ModelItem::Name() in types.hpp).
inline ProErrorCode ModelitemNameGet(RawModelItem *item, wchar_t *name_out) {
  return ::ProModelitemNameGet(item, name_out);
}

// Trampoline to ProFeatureRegenerate (see Feature::Regenerate() in
// model_item.hpp). `solid` is passed as a RawMdl: this assumes ProSolid
// is interchangeable with ProMdl (as described by the user; not verified
// against a real header) — if the real SDK's ProSolid is a genuinely
// distinct, incompatible type, this line is where the build will fail to
// tell you so.
inline ProErrorCode FeatureRegenerate(RawMdl solid, RawModelItem *feature) {
  return ::ProFeatureRegenerate(solid, feature);
}

// "Atomic" sizes (official PTC constants, ProSizeConst.h, Creo 10).
inline constexpr int kLineSize = PRO_LINE_SIZE;
inline constexpr int kPathSize = PRO_PATH_SIZE;
inline constexpr int kCommentSize = PRO_COMMENT_SIZE;
inline constexpr int kValueSize = PRO_VALUE_SIZE;
inline constexpr int kMdlNameSize = PRO_MDLNAME_SIZE;
inline constexpr int kNameSize = PRO_NAME_SIZE;
inline constexpr int kTypeSize = PRO_TYPE_SIZE;
inline constexpr int kExtensionSize = PRO_EXTENSION_SIZE;
inline constexpr int kMdlExtensionSize = PRO_MDLEXTENSION_SIZE;
inline constexpr int kVersionSize = PRO_VERSION_SIZE;
inline constexpr int kMaxAssemLevel = PRO_MAX_ASSEM_LEVEL;
inline constexpr int kFeatRefKeySize = PRO_FEATREF_KEY_SIZE;
// PRO_MACRO_SIZE is no longer a real limit for ProMacroLoad() on the PTC
// side (kept by PTC for application compatibility only); reused here for
// the same reason, so that creo::Macro stays sized like the official
// ProMacro typedef.
inline constexpr int kMacroSize = PRO_MACRO_SIZE;

inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

// "Value not used" / "default value" sentinels — see
// creo::ValueUnused/ValueDefault in types.hpp.
inline constexpr int kValueUnused = PRO_VALUE_UNUSED;
inline constexpr int kValueDefault = PRO_VALUE_DEFAULT;

inline constexpr RawBoolean kBooleanFalse = PRO_B_FALSE;
inline constexpr RawBoolean kBooleanTrue = PRO_B_TRUE;

#else

// Aliases to the substitute shim (see protoolkit_shim.hpp).
using ProErrorCode = shim::ProError;
using RawMdl = shim::ProMdl;
using RawObjectType = shim::ProType;
using RawArray = shim::ProArray;
using RawBoolean = shim::ProBoolean;
using RawModelItem = shim::ProModelitem;

inline ProErrorCode ArrayAlloc(int n_objs, int obj_size,
                                int reallocation_size, RawArray *p_array) {
  return shim::ProArrayAlloc(n_objs, obj_size, reallocation_size, p_array);
}
inline ProErrorCode ArrayFree(RawArray *p_array) {
  return shim::ProArrayFree(p_array);
}
inline ProErrorCode ArraySizeGet(RawArray array, int *p_size) {
  return shim::ProArraySizeGet(array, p_size);
}
inline ProErrorCode ArrayMaxCountGet(int obj_size, int *max_num_objs) {
  return shim::ProArrayMaxCountGet(obj_size, max_num_objs);
}
inline ProErrorCode MdlMdlNameGet(RawMdl model, wchar_t *name_out) {
  return shim::ProMdlMdlNameGet(model, name_out);
}
inline ProErrorCode MdlCurrentGet(RawMdl *p_mdl) {
  return shim::ProMdlCurrentGet(p_mdl);
}
inline ProErrorCode ModelitemNameGet(RawModelItem *item, wchar_t *name_out) {
  return shim::ProModelitemNameGet(item, name_out);
}
inline ProErrorCode FeatureRegenerate(RawMdl solid, RawModelItem *feature) {
  return shim::ProFeatureRegenerate(solid, feature);
}

inline constexpr int kLineSize = shim::kLineSize;
inline constexpr int kPathSize = shim::kPathSize;
inline constexpr int kCommentSize = shim::kCommentSize;
inline constexpr int kValueSize = shim::kValueSize;
inline constexpr int kMdlNameSize = shim::kMdlNameSize;
inline constexpr int kNameSize = shim::kNameSize;
inline constexpr int kTypeSize = shim::kTypeSize;
inline constexpr int kExtensionSize = shim::kExtensionSize;
inline constexpr int kMdlExtensionSize = shim::kMdlExtensionSize;
inline constexpr int kVersionSize = shim::kVersionSize;
inline constexpr int kMaxAssemLevel = shim::kMaxAssemLevel;
inline constexpr int kFeatRefKeySize = shim::kFeatRefKeySize;
inline constexpr int kMacroSize = shim::kMacroSize;

inline constexpr ProErrorCode kNoError = shim::PRO_TK_NO_ERROR;
inline constexpr int kValueUnused = shim::kValueUnused;
inline constexpr int kValueDefault = shim::kValueDefault;

inline constexpr RawBoolean kBooleanFalse = shim::PRO_B_FALSE;
inline constexpr RawBoolean kBooleanTrue = shim::PRO_B_TRUE;

#endif

// Composite sizes: the same formulas as the corresponding PTC macros,
// identically valid in real-SDK mode and in shim mode since they only
// combine the atomic sizes above.

// "name.ext.#": sizes ProMdlFileName (the full file name of a Creo
// model).
inline constexpr int kFileMdlNameSize =
    kMdlNameSize + kMdlExtensionSize + kVersionSize;

// "name.ext.#": sizes ProFileName (the generic case).
inline constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

// Sizes ProFamtabClmDesc (a family table column description): PTC
// directly reuses the size of a ProPath.
inline constexpr int kFamTabFieldNameSize = kPathSize;

// "instance[generic]": sizes both ProFamilyMdlName (the name of a family
// table instance for a model) and ProDisplayModelName (a model's display
// name) — PTC gives them the same size.
inline constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;

// "instance[generic]": sizes ProFamilyName (the generic case).
inline constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

} // namespace creo::detail
