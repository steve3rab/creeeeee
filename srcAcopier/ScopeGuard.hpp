#pragma once

#include <type_traits>
#include <utility>

namespace creo {

template <typename F> class ScopeGuard {
public:
  explicit ScopeGuard(F f) noexcept(std::is_nothrow_move_constructible_v<F>)
      : f_(std::move(f)) {}

  ScopeGuard(ScopeGuard &&other) noexcept(
      std::is_nothrow_move_constructible_v<F>)
      : f_(std::move(other.f_)), active_(other.active_) {
    other.active_ = false;
  }

  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
  ScopeGuard &operator=(ScopeGuard &&) = delete;

  ~ScopeGuard() {
    if (active_) {
      f_();
    }
  }

  void Dismiss() noexcept { active_ = false; }

private:
  F f_;
  bool active_ = true;
};

template <typename F> ScopeGuard<std::decay_t<F>> Defer(F &&f) {
  return ScopeGuard<std::decay_t<F>>(std::forward<F>(f));
}

}
