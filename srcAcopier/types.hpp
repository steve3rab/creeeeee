#pragma once
#include "protoolkit_compat.hpp"
#include "utf8.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

template <std::size_t N> class FixedWString {
public:
  static constexpr std::size_t kCapacity = N;

  FixedWString() noexcept { buffer_[0] = L'\0'; }

  explicit FixedWString(std::wstring_view text) { Assign(text); }

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

  void Assign(std::string_view utf8_text) {
    Assign(detail::FromUtf8(utf8_text));
  }

  std::size_t Length() const noexcept {
    std::size_t len = 0;
    while (len < kCapacity && buffer_[len] != L'\0') {
      ++len;
    }
    return len;
  }

  std::wstring ToWString() const { return std::wstring(buffer_, Length()); }

  std::string ToString() const {
    return detail::ToUtf8(std::wstring_view(buffer_, Length()));
  }

  wchar_t *Raw() noexcept { return buffer_; }
  const wchar_t *Raw() const noexcept { return buffer_; }

  operator wchar_t *() noexcept { return buffer_; }
  operator const wchar_t *() const noexcept { return buffer_; }

private:
  wchar_t buffer_[kCapacity];
};

template <std::size_t N> class FixedCharString {
public:
  static constexpr std::size_t kCapacity = N;

  FixedCharString() noexcept { buffer_[0] = '\0'; }

  explicit FixedCharString(std::string_view text) { Assign(text); }

  void Assign(std::string_view text) {
    if (text.size() >= kCapacity) {
      throw std::length_error(
          "valeur trop longue pour ce buffer ProTOOLKIT (capacité "
          "insuffisante)");
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
      buffer_[i] = text[i];
    }
    buffer_[text.size()] = '\0';
  }

  std::size_t Length() const noexcept {
    std::size_t len = 0;
    while (len < kCapacity && buffer_[len] != '\0') {
      ++len;
    }
    return len;
  }

  std::string ToString() const { return std::string(buffer_, Length()); }

  char *Raw() noexcept { return buffer_; }
  const char *Raw() const noexcept { return buffer_; }

  operator char *() noexcept { return buffer_; }
  operator const char *() const noexcept { return buffer_; }

private:
  char buffer_[kCapacity];
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

// Les constantes PRO_TYPE_SIZE, PRO_EXTENSION_SIZE et PRO_VERSION_SIZE
// n'apparaissent dans les en-têtes PTC QUE combinées à l'intérieur de
// ProMdlFileName / ProFileName (voir plus bas) : il n'existe pas de
// typedef PTC autonome "ProType"/"ProExtension"/"ProVersion". Les trois
// alias suivants sont donc des briques utilitaires propres à ce wrapper
// (pratiques pour construire/décomposer un nom de fichier morceau par
// morceau), pas la réexposition d'un type PTC existant. Les trois valent
// 4, mais ne contiennent chacune que 3 caractères utiles + le terminateur
// NULL — comme l'indique explicitement le commentaire PTC pour
// PRO_EXTENSION_SIZE ("size 3; plus NULL terminator"), et pareil pour
// PRO_TYPE_SIZE ("prt"/"asm"/"drw" : 3 lettres) et PRO_VERSION_SIZE.
using ModelTypeCode = FixedWString<detail::kTypeSize>;    // "prt"/"asm"/"drw"
using Extension = FixedWString<detail::kExtensionSize>;   // ext. générique
using VersionSuffix = FixedWString<detail::kVersionSize>; // "nom.ext.<ver>"

// Correspond à `ProMdlExtension` (taille PRO_MDLEXTENSION_SIZE = 32) :
// extension de fichier d'un modèle Creo Parametric. Celui-ci, en revanche,
// est un vrai typedef PTC.
using ModelExtension = FixedWString<detail::kMdlExtensionSize>;

// Nombre maximum de niveaux d'imbrication d'assemblage pris en charge par
// ProTOOLKIT (PRO_MAX_ASSEM_LEVEL). Ce n'est pas une taille de buffer texte,
// juste une limite numérique — exposée ici pour rester à côté des autres
// constantes ProTOOLKIT du wrapper.
inline constexpr int MaxAssemLevel = detail::kMaxAssemLevel;

// Correspond à `ProMacro` (taille PRO_MACRO_SIZE = 256). Note PTC reprise
// ici : cette taille n'est plus une limite réelle pour ProMacroLoad(), le
// typedef (et cet alias) ne sont conservés que pour compatibilité.
using Macro = FixedWString<detail::kMacroSize>;

// ---------------------------------------------------------------------------
// Types composites (noms de fichiers complets)
// ---------------------------------------------------------------------------
// PTC construit ces tailles en additionnant les tailles atomiques
// ci-dessus (voir detail/protoolkit_compat.hpp pour les formules) : elles
// couvrent un nom de fichier complet "nom.ext.#" ou une instance de table
// de famille "instance[generic]".

// Correspond à `ProMdlFileName` (PRO_FILE_MDLNAME_SIZE) : nom de fichier
// complet d'un modèle Creo Parametric, au format "nom.ext.#".
using MdlFileName = FixedWString<detail::kFileMdlNameSize>;

// Correspond à `ProFileName` (PRO_FILE_NAME_SIZE) : nom de fichier complet
// générique, au format "nom.ext.#".
using FileName = FixedWString<detail::kFileNameSize>;

// Correspond à `ProFamtabClmDesc` (PRO_FAMTAB_FIELDNAME_SIZE) : description
// de colonne d'une table de famille (même taille qu'un ProPath).
using FamTabColumnDesc = FixedWString<detail::kFamTabFieldNameSize>;

// Correspond à `ProFamilyMdlName` (PRO_FAMILY_MDLNAME_SIZE) : nom d'une
// instance de table de famille pour un modèle, au format
// "instance[generic]".
using FamilyMdlName = FixedWString<detail::kFamilyMdlNameSize>;

// Correspond à `ProFamilyName` (PRO_FAMILY_NAME_SIZE) : équivalent
// générique de FamilyMdlName, au format "instance[generic]".
using FamilyName = FixedWString<detail::kFamilyNameSize>;

// Correspond à `ProDisplayModelName` : nom d'affichage d'un modèle. PTC lui
// donne la même taille qu'à ProFamilyMdlName (PRO_FAMILY_MDLNAME_SIZE).
using DisplayModelName = FixedWString<detail::kFamilyMdlNameSize>;

// ---------------------------------------------------------------------------
// Buffers en char (non wide) : ProCharName, ProCharPath, ProCharLine et les
// types "menu" ci-dessous sont, contrairement à Name/Line/Path, des
// `char[N]` côté C — voir FixedCharString ci-dessus.
// ---------------------------------------------------------------------------

// Correspond à `ProCharName` (PRO_NAME_SIZE) : variante char de Name.
using CharName = FixedCharString<detail::kNameSize>;

// Correspond à `ProCharPath` (PRO_PATH_SIZE) : variante char de Path.
using CharPath = FixedCharString<detail::kPathSize>;

// Correspond à `ProCharLine` (PRO_LINE_SIZE) : "message constant" côté PTC,
// variante char de Line.
using CharLine = FixedCharString<detail::kLineSize>;

// Correspond à `ProMenuName` (PRO_NAME_SIZE) : nom d'un menu ProTOOLKIT.
using MenuName = FixedCharString<detail::kNameSize>;

// Correspond à `ProMenufileName` (PRO_NAME_SIZE) : nom de fichier menu
// (.mnu).
using MenuFileName = FixedCharString<detail::kNameSize>;

// Correspond à `ProMenubuttonName` (PRO_NAME_SIZE) : nom d'un bouton de
// menu.
using MenuButtonName = FixedCharString<detail::kNameSize>;

class ModelHandle {
public:
  ModelHandle() noexcept : handle_(nullptr) {}
  explicit ModelHandle(detail::RawMdl handle) noexcept : handle_(handle) {}

  bool IsValid() const noexcept { return handle_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

} // namespace creo
