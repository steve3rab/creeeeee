#pragma once
// -----------------------------------------------------------------------
// Substituts MINIMAUX pour les types ProTOOLKIT de base, utilisés
// uniquement quand le SDK Creo réel n'est pas disponible sur la machine de
// compilation (poste de développement sans Creo installé, CI, etc.). Cela
// permet de compiler et de tester la logique du wrapper indépendamment de
// la présence du SDK PTC.
//
// IMPORTANT : ces définitions ne proviennent PAS des en-têtes PTC (SDK
// propriétaire, non redistribuable). Elles reproduisent seulement la forme
// documentée des types (tailles de buffer usuelles, nature opaque du
// handle de modèle) afin que le code appelant compile à l'identique dans
// les deux modes. Dès qu'un SDK Creo 10 réel est détecté (voir
// creo/detail/protoolkit_compat.hpp et cmake/FindProToolkit.cmake), ce
// fichier n'est plus inclus et les vrais en-têtes PTC (ProToolkit.h,
// ProMdl.h, ProObjects.h, ...) prennent le relais automatiquement.
//
// -> Avant toute compilation liée à une vraie session Creo 10, vérifiez ces
//    tailles contre le ProToolkit.h livré avec votre installation.
// -----------------------------------------------------------------------

namespace creo::detail::shim {

// Tailles de buffer telles que documentées par PTC pour ProName / ProLine /
// ProPath (constantes PRO_NAME_SIZE / PRO_LINE_SIZE / PRO_PATH_SIZE).
constexpr int kNameSize = 31;
constexpr int kLineSize = 80;
constexpr int kPathSize = 130;

// ProTOOLKIT manipule le texte en wchar_t depuis Pro/ENGINEER Wildfire.
using ProName = wchar_t[kNameSize];
using ProLine = wchar_t[kLineSize];
using ProPath = wchar_t[kPathSize];

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
