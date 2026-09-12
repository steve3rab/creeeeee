#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/detail/utf8.hpp"
#include "creo/error.hpp"

#include <cstddef>
#include <filesystem>
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
  static_assert(N > 0, "FixedWString requiert une capacité non nulle");

  // Capacité totale du buffer, terminateur nul inclus (correspond à
  // PRO_NAME_SIZE / PRO_LINE_SIZE / PRO_PATH_SIZE selon le type instancié).
  static constexpr std::size_t kCapacity = N;

  // Longueur maximale utilisable, terminateur nul exclu. C'est cette
  // valeur, et non kCapacity, qui borne la taille d'une chaîne acceptée
  // par Assign()/le constructeur — kCapacity inclut le terminateur.
  static constexpr std::size_t kMaxLength = N - 1;

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
    if (text.size() > kMaxLength) {
      throw std::length_error(
          "valeur trop longue pour ce buffer ProTOOLKIT (" +
          std::to_string(text.size()) + " caractères, capacité max " +
          std::to_string(kMaxLength) + ")");
    }
    if (!text.empty()) {
      std::char_traits<wchar_t>::copy(buffer_, text.data(), text.size());
    }
    buffer_[text.size()] = L'\0';
  }

  // Équivalent de Assign() pour une chaîne UTF-8.
  void Assign(std::string_view utf8_text) {
    Assign(detail::FromUtf8(utf8_text));
  }

  // Longueur effective de la chaîne (hors terminateur), utile après qu'une
  // fonction ProTOOLKIT a rempli le buffer via Raw(). Recherche le
  // terminateur via char_traits::find (s'appuie sur wmemchr), plus rapide
  // qu'une boucle manuelle sur les buffers de grande taille (ProPath,
  // ProComment, ...). Si le buffer n'est pas terminé par un nul dans ses
  // kCapacity éléments (buffer corrompu ou mal rempli par l'appelant),
  // retombe sur kCapacity plutôt que de risquer une lecture hors bornes.
  std::size_t Length() const noexcept {
    const wchar_t *end =
        std::char_traits<wchar_t>::find(buffer_, kCapacity, L'\0');
    return end != nullptr ? static_cast<std::size_t>(end - buffer_)
                           : kCapacity;
  }

  // Vue sur le contenu actuel, sans copie. Utile pour comparer ou passer
  // la valeur à une API attendant un std::wstring_view sans payer le prix
  // d'une allocation (ToWString() en construit une nouvelle à chaque
  // appel).
  std::wstring_view View() const noexcept {
    return std::wstring_view(buffer_, Length());
  }

  // Récupère le contenu du buffer tel quel (wide), typiquement après un
  // appel ProTOOLKIT du type CREO_CHECK(ProMdlMdlNameGet(model, name.Raw()));
  std::wstring ToWString() const { return std::wstring(View()); }

  // Récupère le contenu du buffer converti en UTF-8. Le choix de l'UTF-8
  // comme représentation std::string est délibéré : la taille de wchar_t
  // (donc son encodage implicite) diffère entre Windows (UTF-16) et
  // Linux/macOS (UTF-32), l'UTF-8 reste la seule représentation stable des
  // deux côtés (voir creo/detail/utf8.hpp).
  std::string ToString() const { return detail::ToUtf8(View()); }

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

// Comparaisons par contenu, entre deux FixedWString (même capacité ou
// non — comparer un Name et un ModelName a un sens) ou contre un
// std::wstring_view (donc aussi un std::wstring, converti implicitement).
// Sans ces opérateurs, comparer deux valeurs obligerait à passer par
// .ToWString() des deux côtés à chaque fois.
//
// Les surcharges en `const wchar_t*` ne sont PAS redondantes avec celles
// en std::wstring_view : FixedWString a par ailleurs un operator
// wchar_t*() implicite (nécessaire pour l'interop directe avec les
// fonctions C ProTOOLKIT). Sans une surcharge exacte en const wchar_t*,
// comparer contre un littéral (`name == L"texte"`) serait AMBIGU pour le
// compilateur entre convertir `name` en pointeur puis comparer les
// pointeurs (operator== intégré, qui compare des ADRESSES — le mauvais
// résultat, silencieusement) et convertir le littéral en wstring_view
// (notre operator==, qui compare le contenu) : les deux conversions ont
// le même rang. La surcharge exacte lève l'ambiguïté en faveur de la
// bonne comparaison, par contenu.
template <std::size_t N, std::size_t M>
bool operator==(const FixedWString<N> &lhs,
                const FixedWString<M> &rhs) noexcept {
  return lhs.View() == rhs.View();
}
template <std::size_t N, std::size_t M>
bool operator!=(const FixedWString<N> &lhs,
                const FixedWString<M> &rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator==(const FixedWString<N> &lhs, std::wstring_view rhs) noexcept {
  return lhs.View() == rhs;
}
template <std::size_t N>
bool operator==(std::wstring_view lhs, const FixedWString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedWString<N> &lhs, std::wstring_view rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(std::wstring_view lhs, const FixedWString<N> &rhs) noexcept {
  return !(rhs == lhs);
}
template <std::size_t N>
bool operator==(const FixedWString<N> &lhs, const wchar_t *rhs) noexcept {
  return lhs.View() == std::wstring_view(rhs);
}
template <std::size_t N>
bool operator==(const wchar_t *lhs, const FixedWString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedWString<N> &lhs, const wchar_t *rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(const wchar_t *lhs, const FixedWString<N> &rhs) noexcept {
  return !(rhs == lhs);
}

// ---------------------------------------------------------------------------
// FixedCharString<N>
// ---------------------------------------------------------------------------
// Certains buffers ProTOOLKIT ne sont PAS en wchar_t : ProCharName,
// ProCharPath, ProCharLine ainsi que les types "menu" (ProMenuName,
// ProMenufileName, ProMenubuttonName) sont des `char[N]` côté C. Comme les
// deux côtés (buffer et conversion std::string) sont déjà en char, il n'y a
// ici aucune histoire d'encodage à gérer — contrairement à FixedWString,
// une simple copie suffit.
template <std::size_t N> class FixedCharString {
public:
  static_assert(N > 0, "FixedCharString requiert une capacité non nulle");

  static constexpr std::size_t kCapacity = N;
  static constexpr std::size_t kMaxLength = N - 1;

  FixedCharString() noexcept { buffer_[0] = '\0'; }

  explicit FixedCharString(std::string_view text) { Assign(text); }

  void Assign(std::string_view text) {
    if (text.size() > kMaxLength) {
      throw std::length_error(
          "valeur trop longue pour ce buffer ProTOOLKIT (" +
          std::to_string(text.size()) + " caractères, capacité max " +
          std::to_string(kMaxLength) + ")");
    }
    if (!text.empty()) {
      std::char_traits<char>::copy(buffer_, text.data(), text.size());
    }
    buffer_[text.size()] = '\0';
  }

  std::size_t Length() const noexcept {
    const char *end = std::char_traits<char>::find(buffer_, kCapacity, '\0');
    return end != nullptr ? static_cast<std::size_t>(end - buffer_)
                           : kCapacity;
  }

  std::string_view View() const noexcept {
    return std::string_view(buffer_, Length());
  }

  std::string ToString() const { return std::string(View()); }

  char *Raw() noexcept { return buffer_; }
  const char *Raw() const noexcept { return buffer_; }

  operator char *() noexcept { return buffer_; }
  operator const char *() const noexcept { return buffer_; }

private:
  char buffer_[kCapacity];
};

template <std::size_t N, std::size_t M>
bool operator==(const FixedCharString<N> &lhs,
                const FixedCharString<M> &rhs) noexcept {
  return lhs.View() == rhs.View();
}
template <std::size_t N, std::size_t M>
bool operator!=(const FixedCharString<N> &lhs,
                const FixedCharString<M> &rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator==(const FixedCharString<N> &lhs,
                std::string_view rhs) noexcept {
  return lhs.View() == rhs;
}
template <std::size_t N>
bool operator==(std::string_view lhs,
                const FixedCharString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedCharString<N> &lhs,
                std::string_view rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(std::string_view lhs,
                const FixedCharString<N> &rhs) noexcept {
  return !(rhs == lhs);
}
// Surcharges exactes en const char* : même raison que pour FixedWString
// et const wchar_t* ci-dessus (lève l'ambiguïté avec l'operator char*()
// implicite de FixedCharString face à un littéral "texte").
template <std::size_t N>
bool operator==(const FixedCharString<N> &lhs, const char *rhs) noexcept {
  return lhs.View() == std::string_view(rhs);
}
template <std::size_t N>
bool operator==(const char *lhs, const FixedCharString<N> &rhs) noexcept {
  return rhs == lhs;
}
template <std::size_t N>
bool operator!=(const FixedCharString<N> &lhs, const char *rhs) noexcept {
  return !(lhs == rhs);
}
template <std::size_t N>
bool operator!=(const char *lhs, const FixedCharString<N> &rhs) noexcept {
  return !(rhs == lhs);
}

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

// Conversions entre Path et std::filesystem::path : Path représente
// spécifiquement un chemin (contrairement à Name/Line/Comment/...), cette
// interop n'a donc de sens que pour cet alias précis, pas pour le modèle
// générique FixedWString<N>.
inline std::filesystem::path ToFilesystemPath(const Path &path) {
  return std::filesystem::path(path.View());
}
inline Path PathFromFilesystem(const std::filesystem::path &fs_path) {
  return Path(fs_path.wstring());
}

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

// Correspond à `PRO_VALUE_UNUSED` (= -1) : sentinelle "valeur non
// utilisée" que de nombreuses fonctions ProTOOLKIT acceptent en lieu et
// place d'un index ou d'une valeur explicite (par ex. ProArrayObjectAdd :
// tout index négatif ajoute en fin de tableau — PRO_VALUE_UNUSED en est un
// exemple, pas la seule valeur qui déclenche ce comportement).
inline constexpr int ValueUnused = detail::kValueUnused;

// Correspond à `PRO_VALUE_DEFAULT` (= -5) : sentinelle "valeur par
// défaut", distincte de PRO_VALUE_UNUSED malgré la proximité des noms — ne
// pas les confondre dans un appel ProTOOLKIT.
inline constexpr int ValueDefault = detail::kValueDefault;

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

// ---------------------------------------------------------------------------
// ObjectType
// ---------------------------------------------------------------------------
// Correspond à `ProType` (struct pro_obj_types, ProObjects.h) : le type
// d'objet de base de données Creo au sens large — PAS uniquement les
// modèles. Les modèles (part/assemblage/dessin/manufacturing/...) n'en
// sont qu'une petite partie, au même titre que les features, courbes,
// entités de simulation, de maillage, d'animation, etc. Réexposé tel quel
// (alias direct, pas de wrapper RAII : c'est un simple entier étiqueté).
//
// Quelques valeurs "modèle" pour repère : PRO_ASSEMBLY (1), PRO_PART (2),
// PRO_DRAWING (4), PRO_MFG (37), PRO_SUB_ASSEMBLY (34), PRO_DWGFORM (33),
// PRO_LAYOUT (19), PRO_REPORT (105), PRO_MARKUP (116), PRO_DIAGRAM (121).
using ObjectType = detail::RawObjectType;

// ---------------------------------------------------------------------------
// Boolean
// ---------------------------------------------------------------------------
// Correspond à `ProBoolean`/`ProBool` (ProToolkit.h, enum ProBooleans :
// PRO_B_FALSE = 0, PRO_B_TRUE = 1) : le booléen ProTOOLKIT, un type
// distinct du bool C++ bien que ses deux valeurs coïncident numériquement
// avec false/true. De nombreuses fonctions ProTOOLKIT prennent ou
// renvoient précisément ce type (pas un bool C++) : Boolean est réexposé
// tel quel (alias direct, comme ObjectType) pour rester interopérable avec
// elles sans conversion implicite hasardeuse.
using Boolean = detail::RawBoolean;

// Conversions explicites avec le bool C++ : évitent d'écrire
// `x == creo::Boolean{}` (peu lisible) ou de caster à la main à chaque
// appel. ToBool() teste `!= PRO_B_FALSE` plutôt que `== PRO_B_TRUE` par
// prudence défensive : rien ne garantit qu'une fonction ProTOOLKIT ne
// renvoie jamais qu'une des deux valeurs documentées pour un Boolean.
constexpr bool ToBool(Boolean value) noexcept {
  return value != detail::kBooleanFalse;
}
constexpr Boolean ToProBoolean(bool value) noexcept {
  return value ? detail::kBooleanTrue : detail::kBooleanFalse;
}

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

  // Nom du modèle (ProMdlMdlNameGet, qui remplace en Creo 10 l'ancienne
  // ProMdlNameGet dépréciée). Convenience method : évite de réécrire à
  // chaque fois le couple CREO_CHECK + ModelName + .Raw() déjà montré dans
  // examples/hello_creo.cpp. Lève std::logic_error si le handle est
  // invalide (nul), avant même de tenter l'appel ProTOOLKIT — message
  // plus clair qu'un PRO_TK_BAD_INPUTS générique remonté depuis le SDK.
  ModelName Name() const {
    if (!IsValid()) {
      throw std::logic_error(
          "creo::ModelHandle::Name() appelé sur un handle invalide (nul)");
    }
    ModelName name;
    CREO_CHECK(detail::MdlMdlNameGet(handle_, name.Raw()));
    return name;
  }

  // Handle brut, pour les appels directs aux fonctions ProTOOLKIT non (ou
  // pas encore) enveloppées par ce wrapper.
  detail::RawMdl Raw() const noexcept { return handle_; }

private:
  detail::RawMdl handle_;
};

// Deux ModelHandle sont égaux s'ils désignent le même modèle (même handle
// ProTOOLKIT sous-jacent) — pas s'ils ont le même nom, deux modèles
// distincts pouvant partager un nom générique.
inline bool operator==(const ModelHandle &lhs, const ModelHandle &rhs) noexcept {
  return lhs.Raw() == rhs.Raw();
}
inline bool operator!=(const ModelHandle &lhs, const ModelHandle &rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace creo
