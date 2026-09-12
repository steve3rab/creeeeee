#pragma once
#include "creo/detail/protoolkit_compat.hpp"
#include "creo/error.hpp"

#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace creo {

// ---------------------------------------------------------------------------
// Array<T>
// ---------------------------------------------------------------------------
// Conteneur RAII autour de `ProArray` (ProArray.h) : le tableau dynamique
// générique de ProTOOLKIT. Contrairement à `ModelHandle` (non-propriétaire
// — le cycle de vie d'un ProMdl est géré par la session Creo), un ProArray
// EST explicitement alloué/libéré par l'appelant : ce wrapper en prend
// donc la propriété complète (alloue à la construction, libère au
// destructeur), avec sémantique de déplacement uniquement — pas de copie,
// ProTOOLKIT n'offrant pas de primitive de duplication.
//
// Point important sur la gestion mémoire : les fonctions natives
// ProArrayObjectAdd/ProArrayObjectRemove/ProArraySizeSet déplacent les
// éléments par copie mémoire brute (memmove/realloc côté C), sans jamais
// appeler de constructeur/destructeur C++. C'est sans risque pour un type
// trivialement copiable (un int, un ProMdl, un struct C simple), mais
// corromprait un type qui ne l'est pas (ex: std::string, dont certaines
// implémentations stockent un pointeur interne vers son propre buffer —
// le déplacer par memmove laisse ce pointeur invalide).
//
// Pour que ce conteneur se comporte en C++ pour N'IMPORTE QUEL T — sans
// que l'utilisateur du wrapper ait à s'en soucier ou à se heurter à une
// restriction de type — ProArray n'est donc utilisé ici QUE comme
// fournisseur de mémoire brute (ProArrayAlloc/ProArrayFree) : toute la
// gestion du cycle de vie des éléments (construction, destruction,
// déplacement lors d'une croissance ou d'un décalage) est implémentée en
// C++ pur, exactement comme le fait std::vector au-dessus de son
// allocateur. Size()/Capacity() sont donc suivis par ce wrapper lui-même,
// pas relus depuis ProTOOLKIT à chaque appel.
template <typename T> class Array {
public:
  // Alloue un tableau de `initial_count` éléments valeur-initialisés, qui
  // croîtra par blocs d'au moins `reallocation_size` éléments. Nécessite T
  // par défaut constructible si `initial_count > 0` (comme std::vector(n))
  // — mais `Array<T> a(0, N)` compile et fonctionne pour N'IMPORTE QUEL T,
  // y compris sans constructeur par défaut : le `if constexpr` ci-dessous
  // empêche le compilateur d'exiger T() tant que ce chemin n'est pas
  // réellement emprunté avec un initial_count strictement positif (une
  // simple condition `if` à l'exécution ne suffirait pas : le corps de la
  // fonction serait quand même instancié pour un `Array<T>` donné, que la
  // branche soit prise ou non).
  explicit Array(int initial_count = 0, int reallocation_size = 8)
      : reallocation_size_(reallocation_size > 0 ? reallocation_size : 1) {
    if (initial_count > 0) {
      GrowIfNeeded(initial_count);
      try {
        ConstructDefaultRange(0, initial_count);
      } catch (...) {
        detail::ArrayFree(&handle_);
        throw;
      }
      size_ = initial_count;
    }
  }

  // Prend possession d'un ProArray déjà alloué par ProTOOLKIT lui-même
  // (typiquement renvoyé par une fonction Creo qui construit son propre
  // tableau) : sera libéré par ce wrapper à sa destruction. Un tel
  // ProArray ne peut, par construction, contenir que des données C
  // (ProTOOLKIT ne sait pas construire d'objet C++) : T doit donc être
  // trivialement copiable pour cet usage précis, même si le reste du
  // conteneur ne l'exige pas.
  static Array Adopt(detail::RawArray handle) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Array<T>::Adopt() prend possession d'un ProArray "
                  "construit par ProTOOLKIT lui-même, donc nécessairement "
                  "composé de données C : T doit être trivialement "
                  "copiable pour cet usage.");
    int size = 0;
    CREO_CHECK(detail::ArraySizeGet(handle, &size));
    return Array(handle, static_cast<T *>(handle), size, size,
                 /*reallocation_size=*/1);
  }

  ~Array() {
    DestroyRange(0, size_);
    if (handle_ != nullptr) {
      detail::ArrayFree(&handle_);
    }
  }

  Array(const Array &) = delete;
  Array &operator=(const Array &) = delete;

  Array(Array &&other) noexcept
      : handle_(other.handle_), data_(other.data_), size_(other.size_),
        capacity_(other.capacity_),
        reallocation_size_(other.reallocation_size_) {
    other.handle_ = nullptr;
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  Array &operator=(Array &&other) noexcept {
    if (this != &other) {
      DestroyRange(0, size_);
      if (handle_ != nullptr) {
        detail::ArrayFree(&handle_);
      }
      handle_ = other.handle_;
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      reallocation_size_ = other.reallocation_size_;
      other.handle_ = nullptr;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  int Size() const noexcept { return size_; }
  int Capacity() const noexcept { return capacity_; }
  bool Empty() const noexcept { return size_ == 0; }

  // Garantit une capacité d'au moins `new_capacity`, en réallouant un
  // nouveau bloc ProArray et en y déplaçant (ou copiant, si le
  // déplacement peut lever) chaque élément existant si nécessaire.
  // Garantie forte : si une exception est levée, ce tableau reste
  // inchangé.
  void Reserve(int new_capacity) {
    if (new_capacity <= capacity_) {
      return;
    }
    detail::RawArray new_handle = nullptr;
    CREO_CHECK(detail::ArrayAlloc(new_capacity, static_cast<int>(sizeof(T)),
                                   reallocation_size_, &new_handle));
    T *new_data = static_cast<T *>(new_handle);
    int constructed = 0;
    try {
      for (; constructed < size_; ++constructed) {
        ::new (static_cast<void *>(new_data + constructed))
            T(std::move_if_noexcept(data_[constructed]));
      }
    } catch (...) {
      for (int i = 0; i < constructed; ++i) {
        new_data[i].~T();
      }
      detail::ArrayFree(&new_handle);
      throw;
    }
    DestroyRange(0, size_);
    if (handle_ != nullptr) {
      detail::ArrayFree(&handle_);
    }
    handle_ = new_handle;
    data_ = new_data;
    capacity_ = new_capacity;
  }

  // Redimensionne à exactement `new_size` éléments : construit (par
  // défaut) les nouveaux si `new_size > Size()`, détruit les excédentaires
  // sinon. Nécessite T par défaut constructible pour agrandir.
  void Resize(int new_size) {
    if (new_size < 0) {
      throw std::invalid_argument(
          "new_size doit être positif pour creo::Array::Resize");
    }
    if (new_size > size_) {
      GrowIfNeeded(new_size);
      ConstructDefaultRange(size_, new_size);
      size_ = new_size;
    } else if (new_size < size_) {
      DestroyRange(new_size, size_);
      size_ = new_size;
    }
  }

  void Clear() noexcept {
    DestroyRange(0, size_);
    size_ = 0;
  }

  // Insère une copie/un déplacement de `value` à `index` (décale les
  // éléments suivants), ou en index négatif pour ajouter en fin de
  // tableau. La valeur est capturée avant toute croissance éventuelle du
  // tableau, pour rester correct même si `value` référence un élément de
  // ce même tableau (ex: arr.Insert(0, arr[3])).
  void Insert(int index, const T &value) { InsertImpl(index, value); }
  void Insert(int index, T &&value) { InsertImpl(index, std::move(value)); }

  void Append(const T &value) { AppendImpl(value); }
  void Append(T &&value) { AppendImpl(std::move(value)); }

  // Retire `n_objects` éléments à partir de `index` (ou les `n_objects`
  // derniers si `index` est négatif), en décalant le reste.
  void Remove(int index, int n_objects = 1) {
    if (n_objects <= 0) {
      throw std::invalid_argument(
          "n_objects doit être positif pour creo::Array::Remove");
    }
    int remove_at = (index < 0) ? (size_ - n_objects) : index;
    if (remove_at < 0 || remove_at + n_objects > size_) {
      throw std::out_of_range("plage hors limites pour creo::Array::Remove");
    }
    int tail_count = size_ - (remove_at + n_objects);
    for (int i = 0; i < tail_count; ++i) {
      data_[remove_at + i] =
          std::move_if_noexcept(data_[remove_at + n_objects + i]);
    }
    DestroyRange(size_ - n_objects, size_);
    size_ -= n_objects;
  }

  T *Data() noexcept { return data_; }
  const T *Data() const noexcept { return data_; }

  T &operator[](int index) noexcept { return data_[index]; }
  const T &operator[](int index) const noexcept { return data_[index]; }

  T &At(int index) {
    if (index < 0 || index >= size_) {
      throw std::out_of_range("index hors limites pour creo::Array");
    }
    return data_[index];
  }
  const T &At(int index) const {
    if (index < 0 || index >= size_) {
      throw std::out_of_range("index hors limites pour creo::Array");
    }
    return data_[index];
  }

  T *begin() noexcept { return data_; }
  T *end() noexcept { return data_ + size_; }
  const T *begin() const noexcept { return data_; }
  const T *end() const noexcept { return data_ + size_; }

  // Handle brut, pour les appels directs aux fonctions ProTOOLKIT non
  // (encore) enveloppées ici. Reflète la CAPACITÉ allouée (pas Size()) :
  // ProTOOLKIT lui-même n'a pas connaissance de la distinction que ce
  // wrapper fait entre taille logique et capacité.
  detail::RawArray Raw() const noexcept { return handle_; }

private:
  Array(detail::RawArray handle, T *data, int size, int capacity,
        int reallocation_size) noexcept
      : handle_(handle), data_(data), size_(size), capacity_(capacity),
        reallocation_size_(reallocation_size) {}

  void DestroyRange(int begin_index, int end_index) noexcept {
    for (int i = begin_index; i < end_index; ++i) {
      data_[i].~T();
    }
  }

  // Construit des éléments par défaut dans [begin_index, end_index) d'un
  // buffer déjà suffisamment dimensionné (par GrowIfNeeded). Le buffer
  // reste dans un état valide si une exception est levée en cours de
  // route (les éléments déjà construits sont détruits) ; `size_` n'est mis
  // à jour qu'après un appel réussi, par l'appelant.
  //
  // Le `if constexpr` est nécessaire, pas seulement stylistique : sans
  // lui, `T()` serait exigé à la compilation pour TOUT Array<T> dès que le
  // constructeur ou Resize() existe, même quand `initial_count`/
  // `new_size` valent 0 à l'exécution et que ce chemin n'est jamais
  // emprunté pour de vrai.
  void ConstructDefaultRange(int begin_index, int end_index) {
    if constexpr (std::is_default_constructible_v<T>) {
      int constructed = begin_index;
      try {
        for (; constructed < end_index; ++constructed) {
          ::new (static_cast<void *>(data_ + constructed)) T();
        }
      } catch (...) {
        DestroyRange(begin_index, constructed);
        throw;
      }
    } else {
      if (begin_index < end_index) {
        throw std::logic_error(
            "creo::Array<T> : impossible d'agrandir le tableau par "
            "construction par défaut, T n'a pas de constructeur par "
            "défaut accessible");
      }
    }
  }

  void GrowIfNeeded(int min_capacity) {
    if (min_capacity <= capacity_) {
      return;
    }
    int new_capacity = capacity_ > 0 ? capacity_ : reallocation_size_;
    while (new_capacity < min_capacity) {
      new_capacity += reallocation_size_;
    }
    Reserve(new_capacity);
  }

  template <typename U> void InsertImpl(int index, U &&value) {
    int insert_at = (index < 0) ? size_ : index;
    if (insert_at > size_) {
      throw std::out_of_range("index hors limites pour creo::Array::Insert");
    }
    T temp(std::forward<U>(value));
    GrowIfNeeded(size_ + 1);
    if (insert_at == size_) {
      ::new (static_cast<void *>(data_ + size_))
          T(std::move_if_noexcept(temp));
    } else {
      ::new (static_cast<void *>(data_ + size_))
          T(std::move_if_noexcept(data_[size_ - 1]));
      for (int i = size_ - 1; i > insert_at; --i) {
        data_[i] = std::move_if_noexcept(data_[i - 1]);
      }
      data_[insert_at] = std::move_if_noexcept(temp);
    }
    ++size_;
  }

  template <typename U> void AppendImpl(U &&value) {
    T temp(std::forward<U>(value));
    GrowIfNeeded(size_ + 1);
    ::new (static_cast<void *>(data_ + size_)) T(std::move_if_noexcept(temp));
    ++size_;
  }

  detail::RawArray handle_ = nullptr;
  T *data_ = nullptr;
  int size_ = 0;
  int capacity_ = 0;
  int reallocation_size_ = 8;
};

// Nombre maximum d'éléments de type T que ProTOOLKIT peut stocker dans un
// seul ProArray (voir ProArrayMaxCountGet).
template <typename T> int MaxArrayCount() {
  int max_count = 0;
  CREO_CHECK(detail::ArrayMaxCountGet(static_cast<int>(sizeof(T)), &max_count));
  return max_count;
}

} // namespace creo
