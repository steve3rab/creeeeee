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
//   2) SDK absent: falls back to the minimal shim (ProtoolkitShim.hpp)
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
#include <ProAssembly.h>
#include <ProMdl.h>
#include <ProObjects.h>
#include <ProSizeConst.h>
#include <ProToolkit.h>
#else
#include "creo/detail/ProtoolkitShim.hpp"
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
using RawMdlType = ::ProMdlType;

// Trampolines to the real ProArray functions: creo::Array<T> (see
// creo/Array.hpp) calls these aliases, never ::ProArrayXxx nor
// shim::ProArrayXxx directly, to stay mode-independent. Array<T> only
// uses ProArray as a raw memory provider (Alloc/Free) — not its native
// shifting/resizing functions, which move elements by raw memory copy
// and would break any T that is not trivially copyable; see the comment
// at the top of Array.hpp. SizeGet is still needed for Adopt().
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

// Trampoline to ProMdlMdlnameGet (see ModelHandle::Name() in
// ModelHandle.hpp), which replaces the now-deprecated ProMdlNameGet in
// Creo 10. Note the lowercase "n" in "Mdlname": confirmed by the user
// from two independent PTC references, not a typo (see
// ProtoolkitShim.hpp for the fuller rationale).
inline ProErrorCode MdlMdlNameGet(RawMdl model, wchar_t *name_out) {
  return ::ProMdlMdlnameGet(model, name_out);
}

// Trampoline to ProMdlCurrentGet (see ModelHandle::GetCurrent() in
// ModelHandle.hpp).
inline ProErrorCode MdlCurrentGet(RawMdl *p_mdl) {
  return ::ProMdlCurrentGet(p_mdl);
}

// Trampoline to ProMdlTypeGet (see ModelHandle::Type() in
// ModelHandle.hpp).
inline ProErrorCode MdlTypeGet(RawMdl model, RawMdlType *p_type) {
  return ::ProMdlTypeGet(model, p_type);
}

// Trampoline to ProModelitemNameGet (see ModelItem::Name() in Types.hpp).
inline ProErrorCode ModelitemNameGet(RawModelItem *item, wchar_t *name_out) {
  return ::ProModelitemNameGet(item, name_out);
}

// Trampoline to ProFeatureRegenerate (see Feature::Regenerate() in
// ModelItem.hpp). `solid` is passed as a RawMdl: this assumes ProSolid
// is interchangeable with ProMdl (as described by the user; not verified
// against a real header) — if the real SDK's ProSolid is a genuinely
// distinct, incompatible type, this line is where the build will fail to
// tell you so.
inline ProErrorCode FeatureRegenerate(RawMdl solid, RawModelItem *feature) {
  return ::ProFeatureRegenerate(solid, feature);
}

// Visit functions (per PTC's "Visit Functions" documentation): ProAppData
// is void*; ProFeatureVisitAction/ProFeatureFilterAction are the
// callback typedefs ProSolidFeatVisit expects. See
// creo::VisitFeatures() (ModelItem.hpp).
using RawAppData = ::ProAppData;
using RawFeatureVisitAction = ::ProFeatureVisitAction;
using RawFeatureFilterAction = ::ProFeatureFilterAction;

inline ProErrorCode SolidFeatVisit(RawMdl solid,
                                    RawFeatureVisitAction visit_action,
                                    RawFeatureFilterAction filter_action,
                                    RawAppData app_data) {
  return ::ProSolidFeatVisit(solid, visit_action, filter_action, app_data);
}

// Additional visit functions confirmed from PTC's own ProUtilVisit.c
// sample utility (pasted verbatim by the user): all five below share
// ProSolidFeatVisit's "modelitem-style" calling convention (a 3-param
// action taking a pointer to a pro_model_item-shaped struct, plus a
// 2-param filter) since ExpldState/Note/ProcStep/SimpRep/GeomItem are
// themselves just further typedef names for pro_model_item — so the same
// callback types apply, renamed here only for readability at call sites
// that are not specifically about features.
using RawModelItemVisitAction = RawFeatureVisitAction;
using RawModelItemFilterAction = RawFeatureFilterAction;

// See creo::VisitExpldStates() (ModelItem.hpp).
inline ProErrorCode
SolidExpldstateVisit(RawMdl assembly, RawModelItemVisitAction visit_action,
                      RawModelItemFilterAction filter_action,
                      RawAppData app_data) {
  return ::ProSolidExpldstateVisit(assembly, visit_action, filter_action,
                                    app_data);
}

// See creo::VisitNotes() (ModelItem.hpp).
inline ProErrorCode MdlNoteVisit(RawMdl model,
                                  RawModelItemVisitAction visit_action,
                                  RawModelItemFilterAction filter_action,
                                  RawAppData app_data) {
  return ::ProMdlNoteVisit(model, visit_action, filter_action, app_data);
}

// See creo::VisitProcSteps() (ModelItem.hpp).
inline ProErrorCode ProcstepVisit(RawMdl solid,
                                   RawModelItemVisitAction visit_action,
                                   RawModelItemFilterAction filter_action,
                                   RawAppData app_data) {
  return ::ProProcstepVisit(solid, visit_action, filter_action, app_data);
}

// ProSolidSimprepVisit takes filter BEFORE action, unlike every other
// visit function here (confirmed from ProUtilVisit.c). Reordered back to
// the wrapper's usual (visit_action, filter_action) order at this single
// boundary, so creo::VisitSimpReps() (ModelItem.hpp) needs no special
// case of its own.
inline ProErrorCode SolidSimprepVisit(RawMdl solid,
                                       RawModelItemVisitAction visit_action,
                                       RawModelItemFilterAction filter_action,
                                       RawAppData app_data) {
  return ::ProSolidSimprepVisit(solid, filter_action, visit_action, app_data);
}

// ProFeatureGeomitemVisit takes an extra ProType (which kind of geomitem
// to visit) between the owning feature and the action/filter pair. See
// creo::VisitGeomitems() (ModelItem.hpp).
inline ProErrorCode
FeatureGeomitemVisit(RawModelItem *feature, RawObjectType item_type,
                      RawModelItemVisitAction visit_action,
                      RawModelItemFilterAction filter_action,
                      RawAppData app_data) {
  return ::ProFeatureGeomitemVisit(feature, item_type, visit_action,
                                    filter_action, app_data);
}

// Trampolines below are to ProAssembly.h functions (see the
// ModelHandle methods of the same name, minus "Mdl"/"Session", in
// ModelHandle.hpp).
inline ProErrorCode MdlActiveGet(RawMdl *p_mdl) {
  return ::ProMdlActiveGet(p_mdl);
}
inline ProErrorCode MdlExtensionGet(RawMdl model, wchar_t *ext_out) {
  return ::ProMdlExtensionGet(model, ext_out);
}
inline ProErrorCode MdlDirectoryPathGet(RawMdl model, wchar_t *dir_path_out) {
  return ::ProMdlDirectoryPathGet(model, dir_path_out);
}
inline ProErrorCode MdlDisplay(RawMdl model) { return ::ProMdlDisplay(model); }
inline ProErrorCode MdlWindowGet(RawMdl model, int *window_id) {
  return ::ProMdlWindowGet(model, window_id);
}
inline ProErrorCode SessionMdlList(RawMdlType model_type,
                                    RawMdl **p_model_array, int *p_count) {
  return ::ProSessionMdlList(model_type, p_model_array, p_count);
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
// creo::ValueUnused/ValueDefault in Types.hpp.
inline constexpr int kValueUnused = PRO_VALUE_UNUSED;
inline constexpr int kValueDefault = PRO_VALUE_DEFAULT;

inline constexpr RawBoolean kBooleanFalse = PRO_B_FALSE;
inline constexpr RawBoolean kBooleanTrue = PRO_B_TRUE;

#else

// Aliases to the substitute shim (see ProtoolkitShim.hpp).
using ProErrorCode = shim::ProError;
using RawMdl = shim::ProMdl;
using RawObjectType = shim::ProType;
using RawArray = shim::ProArray;
using RawBoolean = shim::ProBoolean;
using RawModelItem = shim::ProModelitem;
using RawMdlType = shim::ProMdlType;

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
  return shim::ProMdlMdlnameGet(model, name_out);
}
inline ProErrorCode MdlCurrentGet(RawMdl *p_mdl) {
  return shim::ProMdlCurrentGet(p_mdl);
}
inline ProErrorCode MdlTypeGet(RawMdl model, RawMdlType *p_type) {
  return shim::ProMdlTypeGet(model, p_type);
}
inline ProErrorCode ModelitemNameGet(RawModelItem *item, wchar_t *name_out) {
  return shim::ProModelitemNameGet(item, name_out);
}
inline ProErrorCode FeatureRegenerate(RawMdl solid, RawModelItem *feature) {
  return shim::ProFeatureRegenerate(solid, feature);
}
using RawAppData = shim::ProAppData;
using RawFeatureVisitAction = shim::ProFeatureVisitAction;
using RawFeatureFilterAction = shim::ProFeatureFilterAction;
inline ProErrorCode SolidFeatVisit(RawMdl solid,
                                    RawFeatureVisitAction visit_action,
                                    RawFeatureFilterAction filter_action,
                                    RawAppData app_data) {
  return shim::ProSolidFeatVisit(solid, visit_action, filter_action,
                                  app_data);
}
using RawModelItemVisitAction = RawFeatureVisitAction;
using RawModelItemFilterAction = RawFeatureFilterAction;
inline ProErrorCode
SolidExpldstateVisit(RawMdl assembly, RawModelItemVisitAction visit_action,
                      RawModelItemFilterAction filter_action,
                      RawAppData app_data) {
  return shim::ProSolidExpldstateVisit(assembly, visit_action, filter_action,
                                        app_data);
}
inline ProErrorCode MdlNoteVisit(RawMdl model,
                                  RawModelItemVisitAction visit_action,
                                  RawModelItemFilterAction filter_action,
                                  RawAppData app_data) {
  return shim::ProMdlNoteVisit(model, visit_action, filter_action, app_data);
}
inline ProErrorCode ProcstepVisit(RawMdl solid,
                                   RawModelItemVisitAction visit_action,
                                   RawModelItemFilterAction filter_action,
                                   RawAppData app_data) {
  return shim::ProProcstepVisit(solid, visit_action, filter_action,
                                 app_data);
}
inline ProErrorCode SolidSimprepVisit(RawMdl solid,
                                       RawModelItemVisitAction visit_action,
                                       RawModelItemFilterAction filter_action,
                                       RawAppData app_data) {
  return shim::ProSolidSimprepVisit(solid, filter_action, visit_action,
                                     app_data);
}
inline ProErrorCode
FeatureGeomitemVisit(RawModelItem *feature, RawObjectType item_type,
                      RawModelItemVisitAction visit_action,
                      RawModelItemFilterAction filter_action,
                      RawAppData app_data) {
  return shim::ProFeatureGeomitemVisit(feature, item_type, visit_action,
                                        filter_action, app_data);
}
inline ProErrorCode MdlActiveGet(RawMdl *p_mdl) {
  return shim::ProMdlActiveGet(p_mdl);
}
inline ProErrorCode MdlExtensionGet(RawMdl model, wchar_t *ext_out) {
  return shim::ProMdlExtensionGet(model, ext_out);
}
inline ProErrorCode MdlDirectoryPathGet(RawMdl model, wchar_t *dir_path_out) {
  return shim::ProMdlDirectoryPathGet(model, dir_path_out);
}
inline ProErrorCode MdlDisplay(RawMdl model) {
  return shim::ProMdlDisplay(model);
}
inline ProErrorCode MdlWindowGet(RawMdl model, int *window_id) {
  return shim::ProMdlWindowGet(model, window_id);
}
inline ProErrorCode SessionMdlList(RawMdlType model_type,
                                    RawMdl **p_model_array, int *p_count) {
  return shim::ProSessionMdlList(model_type, p_model_array, p_count);
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
