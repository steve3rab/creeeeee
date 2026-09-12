#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/error.hpp"

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace creo {

// ---------------------------------------------------------------------------
// Array<T>
// ---------------------------------------------------------------------------
// Enveloppe RAII autour d'un `ProArray` (ProArray.h) : le tableau dynamique
// générique de ProTOOLKIT, un simple `void*` opaque côté C, redimensionné
// par blocs de `reallocation_size` éléments. Contrairement à `ModelHandle`
// (non-propriétaire — le cycle de vie d'un ProMdl est géré par la session
// Creo), un ProArray EST explicitement alloué/libéré par l'appelant : ce
// wrapper en prend donc la propriété complète (alloue à la construction,
// libère au destructeur), avec sémantique de déplacement uniquement — pas
// de copie, ProTOOLKIT n'offrant pas de primitive de duplication et une
// copie par recopie manuelle élément par élément serait coûteuse et
// surprenante à faire passer pour un simple constructeur de copie.
//
// PTC documente qu'un ProArray peut être casté directement en `T*` pour un
// accès contigu (le handle pointe directement sur les données, pas sur une
// structure opaque séparée) : Data()/operator[] s'appuient sur cette
// garantie.
//
// T doit être trivialement copiable : ProTOOLKIT déplace les éléments par
// copie mémoire brute (realloc/memmove), sans jamais appeler de
// constructeur/destructeur C++. Un type non trivial (contenant une
// std::string, un pointeur possédé, une vtable, ...) serait corrompu ou
// fuirait dès la première réallocation.
template <typename T> class Array {
public:
  static_assert(std::is_trivially_copyable_v<T>,
                "creo::Array<T> repose sur ProArray (C), qui déplace ses "
                "éléments par copie mémoire brute (realloc/memmove) : T "
                "doit être trivialement copiable.");

  // Valeur d'index conventionnelle pour "en fin de tableau", utilisée par
  // Append(). ProTOOLKIT documente "toute valeur négative" comme
  // déclenchant cet ajout en fin de tableau (PRO_VALUE_UNUSED en est un
  // exemple, pas une valeur imposée) : -1 convient donc tout autant.
  static constexpr int kAppend = -1;

  // Alloue un tableau de `initial_count` éléments (valeur-initialisés à
  // zéro), qui croîtra par blocs de `reallocation_size` éléments.
  explicit Array(int initial_count = 0, int reallocation_size = 8)
      : handle_(nullptr) {
    CREO_CHECK(detail::ArrayAlloc(initial_count, static_cast<int>(sizeof(T)),
                                   reallocation_size, &handle_));
  }

  // Prend possession d'un ProArray déjà alloué (typiquement renvoyé par une
  // fonction ProTOOLKIT qui construit elle-même le tableau) : sera libéré
  // par ce wrapper à sa destruction, comme s'il l'avait alloué lui-même.
  static Array Adopt(detail::RawArray handle) noexcept { return Array(handle); }

  ~Array() {
    if (handle_ != nullptr) {
      detail::ArrayFree(&handle_);
    }
  }

  Array(const Array &) = delete;
  Array &operator=(const Array &) = delete;

  Array(Array &&other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  Array &operator=(Array &&other) noexcept {
    if (this != &other) {
      if (handle_ != nullptr) {
        detail::ArrayFree(&handle_);
      }
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  // Nombre d'éléments actuellement stockés.
  int Size() const {
    int size = 0;
    CREO_CHECK(detail::ArraySizeGet(handle_, &size));
    return size;
  }

  bool Empty() const { return Size() == 0; }

  // Redimensionne à exactement `new_size` éléments (les nouveaux éléments,
  // le cas échéant, sont zéro-initialisés).
  void Resize(int new_size) {
    CREO_CHECK(detail::ArraySizeSet(&handle_, new_size));
  }

  void Clear() { Resize(0); }

  // Insère `n_objects` éléments à `index` (les décale), ou les ajoute en
  // fin de tableau si `index` est négatif (voir kAppend).
  void Insert(int index, const T *values, int n_objects) {
    CREO_CHECK(detail::ArrayObjectAdd(
        &handle_, index, n_objects,
        const_cast<void *>(static_cast<const void *>(values))));
  }
  void Insert(int index, const T &value) { Insert(index, &value, 1); }

  void Append(const T &value) { Insert(kAppend, &value, 1); }
  void Append(const T *values, int n_objects) {
    Insert(kAppend, values, n_objects);
  }

  // Retire `n_objects` éléments à partir de `index` (ou des `n_objects`
  // derniers éléments si `index` est négatif).
  void Remove(int index, int n_objects = 1) {
    CREO_CHECK(detail::ArrayObjectRemove(&handle_, index, n_objects));
  }

  // Accès contigu non vérifié (comme std::vector::operator[]) : un ProArray
  // se comporte comme un T* une fois alloué.
  T *Data() noexcept { return static_cast<T *>(handle_); }
  const T *Data() const noexcept { return static_cast<const T *>(handle_); }

  T &operator[](int index) noexcept { return Data()[index]; }
  const T &operator[](int index) const noexcept { return Data()[index]; }

  // Accès vérifié (comme std::vector::at) : lève std::out_of_range plutôt
  // que de lire hors bornes.
  T &At(int index) {
    if (index < 0 || index >= Size()) {
      throw std::out_of_range("index hors limites pour creo::Array");
    }
    return Data()[index];
  }
  const T &At(int index) const {
    if (index < 0 || index >= Size()) {
      throw std::out_of_range("index hors limites pour creo::Array");
    }
    return Data()[index];
  }

  T *begin() noexcept { return Data(); }
  T *end() { return Data() + Size(); }
  const T *begin() const noexcept { return Data(); }
  const T *end() const { return Data() + Size(); }

  // Handle brut, pour les appels directs aux fonctions ProTOOLKIT qui
  // acceptent ou renvoient un ProArray non (encore) enveloppées ici.
  detail::RawArray Raw() const noexcept { return handle_; }
  detail::RawArray *RawPtr() noexcept { return &handle_; }

private:
  explicit Array(detail::RawArray handle) noexcept : handle_(handle) {}

  detail::RawArray handle_;
};

} // namespace creo
