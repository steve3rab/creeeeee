#pragma once
#include "error.hpp"
#include "protoolkit_compat.hpp"

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace creo {

template <typename T> class Array {
public:
  static_assert(std::is_trivially_copyable_v<T>);

  static constexpr int kAppend = -1;

  explicit Array(int initial_count = 0, int reallocation_size = 8)
      : handle_(nullptr) {
    CREO_CHECK(detail::ArrayAlloc(initial_count, static_cast<int>(sizeof(T)),
                                   reallocation_size, &handle_));
  }

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

  int Size() const {
    int size = 0;
    CREO_CHECK(detail::ArraySizeGet(handle_, &size));
    return size;
  }

  bool Empty() const { return Size() == 0; }

  void Resize(int new_size) {
    CREO_CHECK(detail::ArraySizeSet(&handle_, new_size));
  }

  void Clear() { Resize(0); }

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

  void Remove(int index, int n_objects = 1) {
    CREO_CHECK(detail::ArrayObjectRemove(&handle_, index, n_objects));
  }

  T *Data() noexcept { return static_cast<T *>(handle_); }
  const T *Data() const noexcept { return static_cast<const T *>(handle_); }

  T &operator[](int index) noexcept { return Data()[index]; }
  const T &operator[](int index) const noexcept { return Data()[index]; }

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

  detail::RawArray Raw() const noexcept { return handle_; }
  detail::RawArray *RawPtr() noexcept { return &handle_; }

private:
  explicit Array(detail::RawArray handle) noexcept : handle_(handle) {}

  detail::RawArray handle_;
};

}
