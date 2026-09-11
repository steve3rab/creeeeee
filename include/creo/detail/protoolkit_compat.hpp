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
// PRO_MDLNAME_SIZE, PRO_TYPE_SIZE, PRO_MAX_ASSEM_LEVEL, etc. sont des
// macros censées être visibles transitivement via ces en-têtes ; si le
// SDK réel les déclare ailleurs, le compilateur signalera une macro
// manquante et il suffira d'ajouter l'en-tête correspondant ci-dessous.
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
// Hypothèse : le SDK réel suit la même convention de nommage que ProName /
// ProLine / ProPath pour le nom de modèle. À corriger si le typedef réel
// porte un autre nom dans vos en-têtes Creo 10.
using RawMdlName = ::ProMdlName;

// Tailles "atomiques" (constantes PTC officielles, Creo 10).
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

inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

#else

// Alias vers le shim de substitution (voir protoolkit_shim.hpp).
using ProErrorCode = shim::ProError;
using RawName = shim::ProName;
using RawLine = shim::ProLine;
using RawPath = shim::ProPath;
using RawMdl = shim::ProMdl;
using RawMdlName = shim::ProMdlName;

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

inline constexpr ProErrorCode kNoError = shim::PRO_TK_NO_ERROR;

#endif

// Tailles composites : mêmes formules que les macros PTC correspondantes,
// valables à l'identique en mode SDK réel comme en mode shim puisqu'elles
// ne font que combiner les tailles atomiques ci-dessus.

// "name.ext.#" (nom de fichier complet d'un modèle Creo).
inline constexpr int kFileMdlNameSize =
    kMdlNameSize + kMdlExtensionSize + kVersionSize;

// "name.ext.#" (nom de fichier complet, cas générique).
inline constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

// Nom de champ de table de famille : PTC réutilise directement la taille
// d'un ProPath.
inline constexpr int kFamTabFieldNameSize = kPathSize;

// "instance[generic]" (nom de modèle d'une instance de table de famille).
inline constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;

// "instance[generic]" (cas générique).
inline constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

} // namespace creo::detail
