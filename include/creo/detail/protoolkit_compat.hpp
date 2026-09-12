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
// (creo::detail::RawMdl, creo::detail::k*Size) et n'a donc jamais besoin de
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
// ProSizeConst.h (constantes PRO_*_SIZE) est un en-tête autonome côté PTC
// (garde d'inclusion propre, ne dépend que de ProToolkit.h pour
// PRO_BEGIN_C_DECLS/PRO_END_C_DECLS) : rien ne garantit qu'il soit inclus
// transitivement par les en-têtes ci-dessous, on l'inclut donc
// explicitement plutôt que de compter dessus.
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

// Code d'erreur et handle de modèle : les seuls types PTC bruts dont le
// wrapper a réellement besoin (les buffers texte sont reconstruits par
// FixedWString<N>/FixedCharString<N> à partir des tailles ci-dessous, pas
// réutilisés directement).
using ProErrorCode = ::ProError;
using RawMdl = ::ProMdl;
using RawObjectType = ::ProType;
using RawArray = ::ProArray;
using RawBoolean = ::ProBoolean;

// Trampolines vers les fonctions ProArray réelles : creo::Array<T> (voir
// creo/array.hpp) appelle ces alias, jamais ::ProArrayXxx ni
// shim::ProArrayXxx directement, pour rester indépendant du mode. Array<T>
// n'utilise ProArray QUE comme fournisseur de mémoire brute (Alloc/Free) —
// pas ses fonctions de décalage/redimensionnement natives, qui déplacent
// les éléments par copie mémoire brute et casseraient tout T non
// trivialement copiable ; voir le commentaire en tête de array.hpp.
// SizeGet reste nécessaire pour Adopt(). MaxCountGet reste exposé comme
// utilitaire autonome (creo::MaxArrayCount<T>()).
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

// Trampoline vers ProMdlMdlNameGet (voir ModelHandle::Name() dans
// types.hpp), qui remplace en Creo 10 l'ancienne ProMdlNameGet dépréciée.
inline ProErrorCode MdlMdlNameGet(RawMdl model, wchar_t *name_out) {
  return ::ProMdlMdlNameGet(model, name_out);
}

// Tailles "atomiques" (constantes PTC officielles, ProSizeConst.h, Creo 10).
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
// PRO_MACRO_SIZE n'est plus une limite réelle pour ProMacroLoad() côté PTC
// (conservée par PTC pour compatibilité applicative uniquement) ; on la
// reprend ici pour la même raison, afin que creo::Macro reste dimensionné
// comme le typedef ProMacro officiel.
inline constexpr int kMacroSize = PRO_MACRO_SIZE;

inline constexpr ProErrorCode kNoError = PRO_TK_NO_ERROR;

// Sentinelles "valeur non utilisée" / "valeur par défaut" — voir
// creo::ValueUnused/ValueDefault dans types.hpp.
inline constexpr int kValueUnused = PRO_VALUE_UNUSED;
inline constexpr int kValueDefault = PRO_VALUE_DEFAULT;

inline constexpr RawBoolean kBooleanFalse = PRO_B_FALSE;
inline constexpr RawBoolean kBooleanTrue = PRO_B_TRUE;

#else

// Alias vers le shim de substitution (voir protoolkit_shim.hpp).
using ProErrorCode = shim::ProError;
using RawMdl = shim::ProMdl;
using RawObjectType = shim::ProType;
using RawArray = shim::ProArray;
using RawBoolean = shim::ProBoolean;

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

// Tailles composites : mêmes formules que les macros PTC correspondantes,
// valables à l'identique en mode SDK réel comme en mode shim puisqu'elles
// ne font que combiner les tailles atomiques ci-dessus.

// "name.ext.#" : dimensionne ProMdlFileName (nom de fichier complet d'un
// modèle Creo).
inline constexpr int kFileMdlNameSize =
    kMdlNameSize + kMdlExtensionSize + kVersionSize;

// "name.ext.#" : dimensionne ProFileName (cas générique).
inline constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

// Dimensionne ProFamtabClmDesc (description de colonne de table de
// famille) : PTC réutilise directement la taille d'un ProPath.
inline constexpr int kFamTabFieldNameSize = kPathSize;

// "instance[generic]" : dimensionne à la fois ProFamilyMdlName (nom de
// modèle d'une instance de table de famille) et ProDisplayModelName (nom
// d'affichage d'un modèle) — PTC leur donne la même taille.
inline constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;

// "instance[generic]" : dimensionne ProFamilyName (cas générique).
inline constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

} // namespace creo::detail
