#pragma once

#if defined(__has_include)
#if __has_include(<ProToolkit.h>)
#define CREO_WRAPPER_HAS_REAL_SDK 1
#endif
#endif
#ifndef CREO_WRAPPER_HAS_REAL_SDK
#define CREO_WRAPPER_HAS_REAL_SDK 0
#endif

#if CREO_WRAPPER_HAS_REAL_SDK
#include <ProMdl.h>
#include <ProObjects.h>
#include <ProToolkit.h>
#else
#include "protoolkit_shim.hpp"
#endif

namespace creo::detail {

#if CREO_WRAPPER_HAS_REAL_SDK

using ProErrorCode = ::ProError;
using RawMdl = ::ProMdl;

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

#else

using ProErrorCode = shim::ProError;
using RawMdl = shim::ProMdl;

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

#endif

inline constexpr int kFileMdlNameSize =
    kMdlNameSize + kMdlExtensionSize + kVersionSize;

inline constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

inline constexpr int kFamTabFieldNameSize = kPathSize;

inline constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;

inline constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

}
