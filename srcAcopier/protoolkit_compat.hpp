#pragma once

#define CREO_WRAPPER_HAS_REAL_SDK 1

#include <ProArray.h>
#include <ProMdl.h>
#include <ProObjects.h>
#include <ProSizeConst.h>
#include <ProToolkit.h>

namespace creo::detail {

using ProErrorCode = ::ProError;
using RawMdl = ::ProMdl;
using RawObjectType = ::ProType;
using RawArray = ::ProArray;
using RawBoolean = ::ProBoolean;
using RawModelItem = ::ProModelitem;

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

inline ProErrorCode MdlMdlNameGet(RawMdl model, wchar_t *name_out) {
  return ::ProMdlMdlNameGet(model, name_out);
}

inline ProErrorCode ModelitemNameGet(RawModelItem *item, wchar_t *name_out) {
  return ::ProModelitemNameGet(item, name_out);
}

inline ProErrorCode FeatureRegenerate(RawMdl solid, RawModelItem *feature) {
  return ::ProFeatureRegenerate(solid, feature);
}

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
inline constexpr int kMacroSize = PRO_MACRO_SIZE;

inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

inline constexpr int kValueUnused = PRO_VALUE_UNUSED;
inline constexpr int kValueDefault = PRO_VALUE_DEFAULT;

inline constexpr RawBoolean kBooleanFalse = PRO_B_FALSE;
inline constexpr RawBoolean kBooleanTrue = PRO_B_TRUE;

inline constexpr int kFileMdlNameSize =
    kMdlNameSize + kMdlExtensionSize + kVersionSize;

inline constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

inline constexpr int kFamTabFieldNameSize = kPathSize;

inline constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;

inline constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

}
