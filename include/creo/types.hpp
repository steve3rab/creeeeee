#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/detail/utf8.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

// ---------------------------------------------------------------------------
// FixedWString<N>
// ---------------------------------------------------------------------------
// Base commune aux buffers texte à taille fixe de ProTOOLKIT : ProName,
// ProLine et ProPath sont tous, côté C, des tableaux `wchar_t[N]`. Les
// fonctions ProTOOLKIT attendent un pointeur brut vers un tel buffer, en
// entrée comme en sortie ; ce wrapper conserve donc le même layout mémoire
// (un simple tableau de N wchar_t, pas d'indirection) tout en ajoutant des
// conversions sûres vers/depuis std::wstring et std::string (UTF-8), ainsi
// que des vérifications de capacité absentes du C brut.
template <std::size_t N> class FixedWString {
public:
  // Capacité totale du buffer, terminateur nul inclus (correspond à
  // PRO_NAME_SIZE / PRO_LINE_SIZE / PRO_PATH_SIZE selon le type instancié).
  static constexpr std::size_t kCapacity = N;

  FixedWString() noexcept { buffer_[0] = L'\0'; }

  // Construction à partir d'une chaîne wide (ex: L"engrenage_01"). Lève
  // std::length_error si la chaîne ne tient pas dans le buffer :
  // contrairement au C, on ne tronque jamais silencieusement une valeur
  // trop longue.
  explicit FixedWString(std::wstring_view text) { Assign(text); }

  // Construction à partir d'une chaîne std::string supposée encodée en
  // UTF-8 (ex: "engrenage_01"). Pratique pour éviter d'écrire des
  // littéraux wide (L"...") partout côté appelant.
  explicit FixedWString(std::string_view utf8_text)
      : FixedWString(detail::FromUtf8(utf8_text)) {}

  void Assign(std::wstring_view text) {
    if (text.size() >= kCapacity) {
      throw std::length_error(
          "valeur trop longue pour ce buffer ProTOOLKIT (capacité "
          "insuffisante)");
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
      buffer_[i] = text[i];
    }
    buffer_[text.size()] = L'\0';
  }

  // Équivalent de Assign() pour une chaîne UTF-8.
  void Assign(std::string_view utf8_text) {
    Assign(detail::FromUtf8(utf8_text));
  }

  // Longueur effective de la chaîne (hors terminateur), utile après qu'une
  // fonction ProTOOLKIT a rempli le buffer via Raw().
  std::size_t Length() const noexcept {
    std::size_t len = 0;
    while (len < kCapacity && buffer_[len] != L'\0') {
      ++len;
    }
    return len;
  }

  // Récupère le contenu du buffer tel quel (wide), typiquement après un
  // appel ProTOOLKIT du type CREO_CHECK(ProMdlMdlNameGet(model, name.Raw()));
  std::wstring ToWString() const { return std::wstring(buffer_, Length()); }

  // Récupère le contenu du buffer converti en UTF-8. Le choix de l'UTF-8
  // comme représentation std::string est délibéré : la taille de wchar_t
  // (donc son encodage implicite) diffère entre Windows (UTF-16) et
  // Linux/macOS (UTF-32), l'UTF-8 reste la seule représentation stable des
  // deux côtés (voir creo/detail/utf8.hpp).
  std::string ToString() const {
    return detail::ToUtf8(std::wstring_view(buffer_, Length()));
  }

  // Accès au buffer brut, pour passer directement aux fonctions C
  // ProTOOLKIT, ex : ProMdlMdlNameGet(model, name.Raw());
  wchar_t *Raw() noexcept { return buffer_; }
  const wchar_t *Raw() const noexcept { return buffer_; }

  // Permet de passer l'objet directement là où un `wchar_t*` /
  // `const wchar_t*` est attendu, sans appel explicite à Raw().
  operator wchar_t *() noexcept { return buffer_; }
  operator const wchar_t *() const noexcept { return buffer_; }

private:
  wchar_t buffer_[kCapacity];
};

// Correspond à `ProName` (taille PRO_NAME_SIZE = 32) : "tout autre nom
// Creo Parametric" (feature, paramètre, repère, ...) — PTC distingue
// explicitement ce type de ProMdlName (voir ModelName ci-dessous), les deux
// tailles n'ont rien de commun.
using Name = FixedWString<detail::kNameSize>;

// Correspond à `ProMdlName` (taille PRO_MDLNAME_SIZE = 180) : nom d'un
// modèle Creo Parametric, tel que renvoyé par ProMdlMdlNameGet (qui
// remplace en Creo 10 l'ancienne fonction ProMdlNameGet désormais
// dépréciée). ATTENTION : PTC réserve à ProMdlName une taille bien plus
// grande que ProName — ce n'est PAS un simple alias de Name.
using ModelName = FixedWString<detail::kMdlNameSize>;

// Correspond à `ProLine` (taille PRO_LINE_SIZE = 81) : une ligne de texte,
// telle qu'utilisée par les fonctions de message ProTOOLKIT
// (ProMessageDisplay, ProUILabelTextSet, ...). Attention : malgré son nom,
// ce n'est PAS un segment géométrique dans l'API ProTOOLKIT — `ProLine` y
// désigne bien un buffer de texte, pas une primitive de courbe.
using Line = FixedWString<detail::kLineSize>;

// Correspond à `ProPath` (taille PRO_PATH_SIZE = 260) : chemin de fichier
// ou de répertoire.
using Path = FixedWString<detail::kPathSize>;

// Correspond à `ProComment` (taille PRO_COMMENT_SIZE = 256) : texte de
// commentaire (feature, paramètre, ...).
using Comment = FixedWString<detail::kCommentSize>;

// Correspond à `ProValue` (taille PRO_VALUE_SIZE = 256) : représentation
// textuelle de la valeur d'un paramètre.
using Value = FixedWString<detail::kValueSize>;

// Correspond à `ProFeatrefKey` (taille PRO_FEATREF_KEY_SIZE = 81) : clé de
// référence de feature.
using FeatRefKey = FixedWString<detail::kFeatRefKeySize>;

// Correspond à la taille PRO_TYPE_SIZE = 4 : code court de type de modèle
// ("prt", "asm", "drw", etc. + terminateur).
using ModelTypeCode = FixedWString<detail::kTypeSize>;

// Correspond à la taille PRO_EXTENSION_SIZE = 4 : extension de fichier
// générique (3 caractères + terminateur NULL).
using Extension = FixedWString<detail::kExtensionSize>;

// Correspond à la taille PRO_MDLEXTENSION_SIZE = 32 : extension de fichier
// d'un modèle Creo Parametric.
using ModelExtension = FixedWString<detail::kMdlExtensionSize>;

// Correspond à la taille PRO_VERSION_SIZE = 4 : numéro de version tel qu'il
// apparaît dans un nom de fichier ("nom.ext.<version>").
using VersionSuffix = FixedWString<detail::kVersionSize>;

// Nombre maximum de niveaux d'imbrication d'assemblage pris en charge par
// ProTOOLKIT (PRO_MAX_ASSEM_LEVEL). Ce n'est pas une taille de buffer texte,
// juste une limite numérique — exposée ici pour rester à côté des autres
// constantes ProTOOLKIT du wrapper.
inline constexpr int MaxAssemLevel = detail::kMaxAssemLevel;

// ---------------------------------------------------------------------------
// Types composites (noms de fichiers complets)
// ---------------------------------------------------------------------------
// PTC construit ces tailles en additionnant les tailles atomiques
// ci-dessus (voir detail/protoolkit_compat.hpp pour les formules) : elles
// couvrent un nom de fichier complet "nom.ext.#" ou une instance de table
// de famille "instance[generic]".

// Correspond à `ProFileMdlname` (PRO_FILE_MDLNAME_SIZE) : nom de fichier
// complet d'un modèle Creo Parametric, au format "nom.ext.#".
using FileMdlName = FixedWString<detail::kFileMdlNameSize>;

// Correspond à `ProFileName` (PRO_FILE_NAME_SIZE) : nom de fichier complet
// générique, au format "nom.ext.#".
using FileName = FixedWString<detail::kFileNameSize>;

// Correspond à `ProFamtabFieldname` (PRO_FAMTAB_FIELDNAME_SIZE) : nom d'un
// champ de table de famille (même taille qu'un ProPath).
using FamTabFieldName = FixedWString<detail::kFamTabFieldNameSize>;

// Correspond à `ProFamilyMdlname` (PRO_FAMILY_MDLNAME_SIZE) : nom d'une
// instance de table de famille pour un modèle, au format
// "instance[generic]".
using FamilyMdlName = FixedWString<detail::kFamilyMdlNameSize>;

// Correspond à `ProFamilyName` (PRO_FAMILY_NAME_SIZE) : équivalent
// générique de FamilyMdlName, au format "instance[generic]".
using FamilyName = FixedWString<detail::kFamilyNameSize>;

// ---------------------------------------------------------------------------
// ModelHandle
// ---------------------------------------------------------------------------
// Enveloppe légère et non-propriétaire autour d'un `ProMdl`. Le cycle de vie
// d'un modèle Creo est géré par la session ProTOOLKIT elle-même (retrieve /
// erase passent par des fonctions ProTOOLKIT dédiées) : ce wrapper ne fait
// donc pas de RAII sur la durée de vie du modèle. Il se contente d'apporter
// un type fort (au lieu du handle opaque brut) et un point d'extension pour
// les futures méthodes du wrapper (nom, type, unités, ...).
class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  // Handle brut, pour les appels directs aux fonctions ProTOOLKIT non (ou
  // pas encore) enveloppées par ce wrapper.
  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

} // namespace creo
