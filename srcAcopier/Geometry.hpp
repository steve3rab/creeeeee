#pragma once
#include <exception>
#include <type_traits>
#include <utility>

namespace creo {

template <typename RawHandleT> class GeometryHandle {
public:
  GeometryHandle() noexcept : raw_(nullptr) {}
  explicit GeometryHandle(RawHandleT raw) noexcept : raw_(raw) {}

  bool IsValid() const noexcept { return raw_ != nullptr; }
  explicit operator bool() const noexcept { return IsValid(); }

  RawHandleT Raw() const noexcept { return raw_; }

private:
  RawHandleT raw_;
};

template <typename RawHandleT>
bool operator==(const GeometryHandle<RawHandleT> &lhs,
                 const GeometryHandle<RawHandleT> &rhs) noexcept {
  return lhs.Raw() == rhs.Raw();
}
template <typename RawHandleT>
bool operator!=(const GeometryHandle<RawHandleT> &lhs,
                 const GeometryHandle<RawHandleT> &rhs) noexcept {
  return !(lhs == rhs);
}

namespace detail {

template <typename HandleT, typename ActionFn, typename FilterFn>
struct OpaqueVisitContext {
  ActionFn &action;
  FilterFn &filter;
  std::exception_ptr exception;
};

template <typename HandleT, typename ActionFn, typename FilterFn,
          typename RawErrorT, typename RawAppDataT>
RawErrorT OpaqueVisitTrampoline(HandleT item, RawErrorT status,
                                 RawAppDataT app_data) noexcept {
  auto *context = static_cast<OpaqueVisitContext<HandleT, ActionFn, FilterFn> *>(
      app_data);
  if (context->exception) {
    return static_cast<RawErrorT>(-1);
  }
  try {
    return static_cast<RawErrorT>(context->action(item, status));
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<RawErrorT>(-1);
  }
}

template <typename HandleT, typename ActionFn, typename FilterFn,
          typename RawErrorT, typename RawAppDataT>
RawErrorT OpaqueFilterTrampoline(HandleT item, RawAppDataT app_data) noexcept {
  auto *context = static_cast<OpaqueVisitContext<HandleT, ActionFn, FilterFn> *>(
      app_data);
  try {
    return static_cast<RawErrorT>(context->filter(item));
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<RawErrorT>(-1);
  }
}

}

template <typename OwnerT, typename HandleT, typename RawErrorT,
          typename RawAppDataT, typename ActionFn, typename FilterFn>
RawErrorT
VisitOpaque(RawErrorT (*raw_visit_fn)(OwnerT,
                                       RawErrorT (*)(HandleT, RawErrorT,
                                                      RawAppDataT),
                                       RawErrorT (*)(HandleT, RawAppDataT),
                                       RawAppDataT),
            OwnerT owner, ActionFn &&action, FilterFn &&filter) {
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::OpaqueVisitContext<HandleT, ActionT, FilterT> context{action, filter,
                                                                 nullptr};
  RawErrorT result = raw_visit_fn(
      owner,
      &detail::OpaqueVisitTrampoline<HandleT, ActionT, FilterT, RawErrorT,
                                      RawAppDataT>,
      &detail::OpaqueFilterTrampoline<HandleT, ActionT, FilterT, RawErrorT,
                                       RawAppDataT>,
      &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}

template <typename OwnerT, typename HandleT, typename RawErrorT,
          typename RawAppDataT, typename ActionFn>
RawErrorT
VisitOpaque(RawErrorT (*raw_visit_fn)(OwnerT,
                                       RawErrorT (*)(HandleT, RawErrorT,
                                                      RawAppDataT),
                                       RawErrorT (*)(HandleT, RawAppDataT),
                                       RawAppDataT),
            OwnerT owner, ActionFn &&action) {
  return VisitOpaque(raw_visit_fn, owner, std::forward<ActionFn>(action),
                      [](HandleT) { return static_cast<RawErrorT>(0); });
}

}
