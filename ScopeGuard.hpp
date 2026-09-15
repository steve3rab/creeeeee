#pragma once
// -----------------------------------------------------------------------
// Generic RAII "run this on scope exit" utility.
//
// ProTOOLKIT is a C API with many begin/end and set/restore pairs (e.g.
// suspend regeneration then resume it, disable display then re-enable
// it, ProUtilXxx setup/teardown calls, ...): there is no destructor to
// hook the restore step to automatically, so every caller ends up
// hand-writing the same try/catch-and-restore boilerplate, and it is
// easy to forget on one of several early-return paths. ScopeGuard/Defer
// fixes that generically, independently of any specific ProTOOLKIT
// function: wrap the restore step once, right next to the step that
// needs it, and it runs no matter how the scope is exited (normal
// return, early return, or an exception).
// -----------------------------------------------------------------------

#include <type_traits>
#include <utility>

namespace creo {

// Runs `f` when the guard is destroyed, unless Dismiss() was called
// first. Move-only: ownership of "run this on exit" transfers, it is
// never shared. Prefer creo::Defer(...) (below) over naming this type
// directly, so the callable's type does not need to be spelled out.
//
// `f` must not throw: like any destructor, ~ScopeGuard() is implicitly
// noexcept, so a throwing `f` calls std::terminate() rather than
// letting the exception escape (the same rule the standard library
// itself follows for deleters, comparators, hash functors, ...).
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

  // Cancels the pending call: use when the cleanup turns out to be
  // unnecessary (e.g. an operation that started as "risky" completed
  // and committed successfully, so its rollback must not run).
  void Dismiss() noexcept { active_ = false; }

private:
  F f_;
  bool active_ = true;
};

// Deduces F so callers never spell out ScopeGuard<...> themselves:
//
//   detail::MdlRegenModeSet(model.Raw(), kRegenModeManual);
//   auto restore_regen_mode = creo::Defer([&] {
//     detail::MdlRegenModeSet(model.Raw(), previous_mode);
//   });
//   // ... code that may return early or throw ...
//   // previous_mode is restored no matter which path is taken.
template <typename F> ScopeGuard<std::decay_t<F>> Defer(F &&f) {
  return ScopeGuard<std::decay_t<F>>(std::forward<F>(f));
}

} // namespace creo
