#pragma once
// -----------------------------------------------------------------------
// Point d'entrée unique vers les types natifs ProTOOLKIT.
//
// Deux modes de compilation :
//   1) SDK réel présent (Creo 10 installé + variable CMake/environnement
//      CREO_TOOLKIT_ROOT renseignée, cf. cmake/FindProToolkit.cmake) : on
//      inclut directement les en-têtes PTC et on réexpose leurs types tels
//      quels. C'est le mode à utiliser pour tout build destiné à tourner
//      dans une session Creo.
//   2) SDK absent : on retombe sur le shim minimal (protoolkit_shim.hpp)
//      pour permettre au wrapper de compiler et d'être testé hors poste
//      Creo.
//
// Le reste du wrapper (creo::Name, creo::Line, creo::ModelHandle,
// creo::ProToolkitError, ...) ne dépend que des alias définis ici
// (creo::detail::Raw*, creo::detail::k*Size) et n'a donc jamais besoin de
// savoir dans quel mode il est compilé.
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
// En-têtes officiels PTC, fournis avec le SDK ProTOOLKIT de Creo 10.
// Non redistribués dans ce dépôt : voir README pour leur emplacement.
#include <ProMdl.h>
#include <ProObjects.h>
#include <ProToolkit.h>
#else
#include "creo/detail/protoolkit_shim.hpp"
#endif

namespace creo::detail {

#if CREO_WRAPPER_HAS_REAL_SDK

// Alias directs vers les types PTC réels.
using ProErrorCode = ::ProError;
using RawName = ::ProName;
using RawLine = ::ProLine;
using RawPath = ::ProPath;
using RawMdl = ::ProMdl;

inline constexpr int kNameSize = PRO_NAME_SIZE;
inline constexpr int kLineSize = PRO_LINE_SIZE;
inline constexpr int kPathSize = PRO_PATH_SIZE;
inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

#else

// Alias vers le shim de substitution (voir protoolkit_shim.hpp).
using ProErrorCode = shim::ProError;
using RawName = shim::ProName;
using RawLine = shim::ProLine;
using RawPath = shim::ProPath;
using RawMdl = shim::ProMdl;

inline constexpr int kNameSize = shim::kNameSize;
inline constexpr int kLineSize = shim::kLineSize;
inline constexpr int kPathSize = shim::kPathSize;
inline constexpr ProErrorCode kNoError = shim::PRO_TK_NO_ERROR;

#endif

} // namespace creo::detail
