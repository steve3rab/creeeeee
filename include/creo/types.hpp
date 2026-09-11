#pragma once
#include "creo/detail/protoolkit_compat.hpp"

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
// conversions sûres vers/depuis std::wstring et des vérifications de
// capacité absentes du C brut.
template <std::size_t N> class FixedWString {
public:
  // Capacité totale du buffer, terminateur nul inclus (correspond à
  // PRO_NAME_SIZE / PRO_LINE_SIZE / PRO_PATH_SIZE selon le type instancié).
  static constexpr std::size_t kCapacity = N;

  FixedWString() noexcept { buffer_[0] = L'\0'; }

  // Construction à partir d'une chaîne C++. Lève std::length_error si la
  // chaîne ne tient pas dans le buffer : contrairement au C, on ne tronque
  // jamais silencieusement une valeur trop longue.
  explicit FixedWString(std::wstring_view text) { Assign(text); }

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

  // Longueur effective de la chaîne (hors terminateur), utile après qu'une
  // fonction ProTOOLKIT a rempli le buffer via Raw().
  std::size_t Length() const noexcept {
    std::size_t len = 0;
    while (len < kCapacity && buffer_[len] != L'\0') {
      ++len;
    }
    return len;
  }

  std::wstring ToWString() const { return std::wstring(buffer_, Length()); }

  // Accès au buffer brut, pour passer directement aux fonctions C
  // ProTOOLKIT, ex : ProMdlNameGet(model, name.Raw());
  wchar_t *Raw() noexcept { return buffer_; }
  const wchar_t *Raw() const noexcept { return buffer_; }

  // Permet de passer l'objet directement là où un `wchar_t*` /
  // `const wchar_t*` est attendu, sans appel explicite à Raw().
  operator wchar_t *() noexcept { return buffer_; }
  operator const wchar_t *() const noexcept { return buffer_; }

private:
  wchar_t buffer_[kCapacity];
};

// Correspond à `ProName` (taille PRO_NAME_SIZE) : nom court générique
// (feature, paramètre, repère, ...).
using Name = FixedWString<detail::kNameSize>;

// Alias sémantique pour le cas d'usage "nom de modèle" (ce que renvoie par
// exemple ProMdlNameGet). Il ne s'agit pas d'un type ProTOOLKIT distinct au
// sens C — PTC réutilise ProName — mais nommer l'alias explicitement évite
// toute ambiguïté dans les signatures du wrapper.
using ModelName = Name;

// Correspond à `ProLine` (taille PRO_LINE_SIZE) : une ligne de texte, telle
// qu'utilisée par les fonctions de message ProTOOLKIT (ProMessageDisplay,
// ProUILabelTextSet, ...). Attention : malgré son nom, ce n'est PAS un
// segment géométrique dans l'API ProTOOLKIT — `ProLine` y désigne bien un
// buffer de texte, pas une primitive de courbe.
using Line = FixedWString<detail::kLineSize>;

// Correspond à `ProPath` (taille PRO_PATH_SIZE) : chemin de fichier ou de
// répertoire.
using Path = FixedWString<detail::kPathSize>;

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
