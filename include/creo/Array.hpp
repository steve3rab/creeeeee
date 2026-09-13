#pragma once
#include "creo/detail/ProtoolkitCompat.hpp"
#include "creo/Error.hpp"

#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace creo {

// ---------------------------------------------------------------------------
// Array<T>
// ---------------------------------------------------------------------------
// RAII container around `ProArray` (ProArray.h): ProTOOLKIT's generic
// dynamic array. Unlike `ModelHandle` (non-owning — the lifetime of a
// ProMdl is managed by the Creo session), a ProArray IS explicitly
// allocated/freed by the caller: this wrapper therefore takes full
// ownership of it (allocates on construction, frees in the destructor),
// with move-only semantics — no copy, since ProTOOLKIT offers no
// duplication primitive.
//
// Important note on memory management: the native functions
// ProArrayObjectAdd/ProArrayObjectRemove/ProArraySizeSet move elements by
// raw memory copy (memmove/realloc on the C side), never calling a C++
// constructor/destructor. That's safe for a trivially copyable type (an
// int, a ProMdl, a plain C struct), but would corrupt one that isn't
// (e.g. std::string, some implementations of which store an internal
// pointer into their own buffer — moving it via memmove leaves that
// pointer dangling).
//
// For this container to behave like C++ for ANY T — without the wrapper's
// user having to worry about it or hit a type restriction — ProArray is
// therefore used here ONLY as a raw memory provider
// (ProArrayAlloc/ProArrayFree): all element lifetime management
// (construction, destruction, moving on growth or on a shift) is
// implemented in pure C++, exactly as std::vector does on top of its
// allocator. Size()/Capacity() are thus tracked by this wrapper itself,
// not re-read from ProTOOLKIT on every call.
template <typename T> class Array {
public:
  // Allocates an array of `initial_count` value-initialized elements,
  // which will grow in blocks of at least `reallocation_size` elements.
  // Requires T to be default-constructible if `initial_count > 0` (like
  // std::vector(n)) — but `Array<T> a(0, N)` compiles and works for ANY
  // T, including one with no default constructor: the `if constexpr`
  // below stops the compiler from requiring T() as long as this path is
  // not actually taken with a strictly positive initial_count (a plain
  // runtime `if` would not be enough here: the function body would still
  // get instantiated for a given `Array<T>` whether or not the branch is
  // taken).
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

  // Takes ownership of a ProArray already allocated by ProTOOLKIT itself
  // (typically returned by a Creo function that builds its own array):
  // it will be freed by this wrapper on destruction. Such a ProArray can,
  // by construction, only ever contain C data (ProTOOLKIT cannot
  // construct a C++ object): T must therefore be trivially copyable for
  // this specific use, even though the rest of the container does not
  // require it.
  static Array Adopt(detail::RawArray handle) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Array<T>::Adopt() takes ownership of a ProArray built "
                  "by ProTOOLKIT itself, so it necessarily holds C data: "
                  "T must be trivially copyable for this use.");
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

  // Guarantees a capacity of at least `new_capacity`, by reallocating a
  // new ProArray block and moving (or copying, if the move could throw)
  // each existing element into it as needed. Strong guarantee: if an
  // exception is thrown, this array is left unchanged.
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

  // Resizes to exactly `new_size` elements: default-constructs the new
  // ones if `new_size > Size()`, destroys the extra ones otherwise.
  // Requires T to be default-constructible to grow.
  void Resize(int new_size) {
    if (new_size < 0) {
      throw std::invalid_argument(
          "new_size must be positive for creo::Array::Resize");
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

  // Inserts a copy/move of `value` at `index` (shifting later elements),
  // or at the end of the array for a negative index. The value is
  // captured before any potential growth of the array, to remain correct
  // even if `value` references an element of this same array (e.g.
  // arr.Insert(0, arr[3])).
  void Insert(int index, const T &value) { InsertImpl(index, value); }
  void Insert(int index, T &&value) { InsertImpl(index, std::move(value)); }

  void Append(const T &value) { AppendImpl(value); }
  void Append(T &&value) { AppendImpl(std::move(value)); }

  // Removes `n_objects` elements starting at `index` (or the last
  // `n_objects` if `index` is negative), shifting the rest down.
  void Remove(int index, int n_objects = 1) {
    if (n_objects <= 0) {
      throw std::invalid_argument(
          "n_objects must be positive for creo::Array::Remove");
    }
    int remove_at = (index < 0) ? (size_ - n_objects) : index;
    if (remove_at < 0 || remove_at + n_objects > size_) {
      throw std::out_of_range("range out of bounds for creo::Array::Remove");
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
      throw std::out_of_range("index out of range for creo::Array");
    }
    return data_[index];
  }
  const T &At(int index) const {
    if (index < 0 || index >= size_) {
      throw std::out_of_range("index out of range for creo::Array");
    }
    return data_[index];
  }

  T *begin() noexcept { return data_; }
  T *end() noexcept { return data_ + size_; }
  const T *begin() const noexcept { return data_; }
  const T *end() const noexcept { return data_ + size_; }

  // Raw handle, for direct calls to ProTOOLKIT functions not (yet)
  // wrapped here. Reflects the allocated CAPACITY (not Size()):
  // ProTOOLKIT itself has no notion of the distinction this wrapper
  // makes between logical size and capacity.
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

  // Default-constructs elements in [begin_index, end_index) of a buffer
  // already sized large enough (by GrowIfNeeded). The buffer is left in a
  // valid state if an exception is thrown along the way (elements
  // already constructed are destroyed); `size_` is only updated after a
  // successful call, by the caller.
  //
  // The `if constexpr` is necessary, not just stylistic: without it,
  // `T()` would be required at compile time for ANY Array<T> as soon as
  // the constructor or Resize() exists, even when `initial_count`/
  // `new_size` are 0 at runtime and this path is never actually taken.
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
            "creo::Array<T>: cannot grow the array via default "
            "construction, T has no accessible default constructor");
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
      throw std::out_of_range("index out of range for creo::Array::Insert");
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

// Maximum number of elements of type T that ProTOOLKIT can store in a
// single ProArray (see ProArrayMaxCountGet).
template <typename T> int MaxArrayCount() {
  int max_count = 0;
  CREO_CHECK(detail::ArrayMaxCountGet(static_cast<int>(sizeof(T)), &max_count));
  return max_count;
}

} // namespace creo
