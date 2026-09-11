#pragma once
// -----------------------------------------------------------------------
// Substituts MINIMAUX pour les types ProTOOLKIT de base, utilisés
// uniquement quand le SDK Creo réel n'est pas disponible sur la machine de
// compilation (poste de développement sans Creo installé, CI, etc.). Cela
// permet de compiler et de tester la logique du wrapper indépendamment de
// la présence du SDK PTC.
//
// Les tailles ci-dessous sont les constantes officielles PTC pour Creo
// Parametric 10.0 (confirmées par l'utilisateur à partir des en-têtes
// réels) : PRO_LINE_SIZE, PRO_PATH_SIZE, PRO_COMMENT_SIZE, PRO_VALUE_SIZE,
// PRO_MDLNAME_SIZE, PRO_NAME_SIZE, PRO_TYPE_SIZE, PRO_EXTENSION_SIZE,
// PRO_MDLEXTENSION_SIZE, PRO_VERSION_SIZE, PRO_MAX_ASSEM_LEVEL et
// PRO_FEATREF_KEY_SIZE. Le reste de ce fichier ne fait qu'imiter la forme
// des types (buffers texte, handle opaque) : dès qu'un SDK Creo 10 réel
// est détecté (voir creo/detail/protoolkit_compat.hpp et
// cmake/FindProToolkit.cmake), ce fichier n'est plus inclus et les vrais
// en-têtes PTC prennent le relais automatiquement.
// -----------------------------------------------------------------------

namespace creo::detail::shim {

// --- Tailles "atomiques" (valeurs officielles PTC, Creo 10) --------------

constexpr int kLineSize = 81;
constexpr int kPathSize = 260;
constexpr int kCommentSize = 256;
constexpr int kValueSize = 256;

constexpr int kMdlNameSize = 180; // Nom de modèle Creo Parametric (ProMdl).
constexpr int kNameSize = 32;     // Tout autre nom Creo Parametric.
constexpr int kTypeSize = 4;      // "prt", "asm", "drw", etc. + terminateur.
constexpr int kExtensionSize = 4; // 3 caractères + terminateur NULL.
constexpr int kMdlExtensionSize = 32;
constexpr int kVersionSize = 4;
constexpr int kMaxAssemLevel = 25; // Pas une taille de buffer : nombre max
                                    // de niveaux d'imbrication d'assemblage.
constexpr int kFeatRefKeySize = 81;
// PRO_MACRO_SIZE : conservée par PTC pour compatibilité applicative
// uniquement, ProMacroLoad() n'est plus limitée par cette taille.
constexpr int kMacroSize = 256;

// --- Tailles composites (mêmes formules que les macros PTC) --------------

// "name.ext.#"
constexpr int kFileMdlNameSize = kMdlNameSize + kMdlExtensionSize + kVersionSize;
constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

constexpr int kFamTabFieldNameSize = kPathSize;

// "instance[generic]"
constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;
constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

// Handle de modèle Creo (ProMdl) : un pointeur opaque, jamais déréférencé
// par le code appelant. Seul ProTOOLKIT connaît la structure pointée ; on
// se contente ici de préserver la sémantique "pointeur opaque distinct".
struct ProMdlOpaque;
using ProMdl = ProMdlOpaque *;

// Code de retour des fonctions ProTOOLKIT. PRO_TK_NO_ERROR (0) est la seule
// valeur dont ce wrapper a réellement besoin en mode shim : la table
// complète des codes d'erreur vit dans ProToolkitErrors.h du SDK PTC.
enum ProError : int {
  PRO_TK_NO_ERROR = 0,
};

} // namespace creo::detail::shim
