#pragma once
#include "error.hpp"
#include "protoolkit_compat.hpp"

#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace creo {

template <typename T> class Array {
public:
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

  static Array Adopt(detail::RawArray handle) {
    static_assert(std::is_trivially_copyable_v<T>);
    int size = 0;
    CREO_CHECK(detail::ArraySizeGet(handle, &size));
    return Array(handle, static_cast<T *>(handle), size, size, 1);
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

  void Insert(int index, const T &value) { InsertImpl(index, value); }
  void Insert(int index, T &&value) { InsertImpl(index, std::move(value)); }

  void Append(const T &value) { AppendImpl(value); }
  void Append(T &&value) { AppendImpl(std::move(value)); }

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

template <typename T> int MaxArrayCount() {
  int max_count = 0;
  CREO_CHECK(detail::ArrayMaxCountGet(static_cast<int>(sizeof(T)), &max_count));
  return max_count;
}

}
