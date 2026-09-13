#pragma once
#include <exception>
#include <type_traits>
#include <utility>

namespace creo {

// ---------------------------------------------------------------------------
// GeometryHandle<RawHandleT>
// ---------------------------------------------------------------------------
// A handful of PTC ProTOOLKIT types confirmed from its own ProUtilVisit.c
// sample utility (pasted verbatim by the user) — Csys, Axis, Quilt,
// Surface, Contour, Edge — are, unlike ProModelitem (see creo::ModelItem),
// plain OPAQUE POINTER handles: the visit functions that walk them
// (ProSolidCsysVisit, ProSolidAxisVisit, ProSolidQuiltVisit,
// ProQuiltSurfaceVisit, ProSurfaceContourVisit, ProContourEdgeVisit) pass
// the handle to their action/filter callbacks BY VALUE, not by pointer —
// confirmed from the sample's own `(void*)&p_object` idiom, which only
// makes sense if `p_object` (the handle) already fits in a pointer-sized
// value. This wrapper does not know, and was never given, PTC's own
// declared type name for any of the six (e.g. whatever the real header
// spells "Csys" as) or which header declares it, so it cannot define
// `using Csys = ...;` the way ModelItem.hpp does for Feature/Note/etc.
// without guessing — something this project has consistently avoided
// elsewhere (see the exclusions documented in ModelItem.hpp/README).
//
// GeometryHandle<RawHandleT> is the part that needs no such guess: a
// thin, generic, non-owning wrapper around any opaque pointer handle,
// for application code that already has the real type from its own PTC
// headers, e.g.:
//
//   using Csys = creo::GeometryHandle<ProCsys>;   // your own real ProCsys
//
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

// ---------------------------------------------------------------------------
// VisitOpaque(): lambda-friendly wrapper for the "opaque-handle" family of
// PTC visit functions above — the by-value counterpart to
// creo::VisitFeatures() (ModelItem.hpp), which instead covers the
// "modelitem-style" family (Feature, Note, ExpldState, ProcStep, SimpRep,
// GeomItem — items passed by pointer to a pro_model_item-shaped struct).
//
// Rather than hard-coding six functions bound to guessed PTC type/header
// names, VisitOpaque() takes the real raw PTC visit function itself as its
// first argument (e.g. `::ProSolidCsysVisit`, from your own project's real
// ProTOOLKIT headers) and deduces every type it needs — the owner, the
// handle, the raw error code, ProAppData — directly from that function
// pointer's actual declared signature. Nothing here is a project-side
// guess: if the real function's signature does not match the confirmed
// "owner, action, filter, app_data" shape (action taking the handle by
// value plus a status, filter taking just the handle), template argument
// deduction fails and the call does not compile — loudly, rather than
// silently doing the wrong thing, exactly like the rest of this wrapper's
// stance on unconfirmed PTC shapes.
//
// Usage (in an application that has the real Creo SDK headers):
//
//   ProError result = creo::VisitOpaque(
//       ::ProSolidCsysVisit, solid.Raw(),
//       [](ProCsys csys, ProError status) {
//         ...
//         return PRO_TK_NO_ERROR;
//       });
//
// `action(HandleT, RawErrorT status) -> RawErrorT` / `filter(HandleT) ->
// RawErrorT` follow VisitFeatures()'s exact contract: PRO_TK_CONTINUE from
// `filter` skips the item; any other value from `action` stops the visit
// and becomes VisitOpaque()'s own return value. A C++ exception thrown
// from either is never let to unwind through ProTOOLKIT's C stack frames:
// it is stashed (see detail::OpaqueVisitContext below) and rethrown here
// once the raw call has returned, exactly like VisitFeatures().
// ---------------------------------------------------------------------------

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
    return static_cast<RawErrorT>(-1); // PRO_TK_GENERAL_ERROR
  }
  try {
    return static_cast<RawErrorT>(context->action(item, status));
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<RawErrorT>(-1); // PRO_TK_GENERAL_ERROR
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
    return static_cast<RawErrorT>(-1); // PRO_TK_GENERAL_ERROR
  }
}

} // namespace detail

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

// Overload with no filter: `action` is called for every item, with status
// always PRO_TK_NO_ERROR (0 on every ProError-shaped enum this wrapper has
// ever seen, PTC's own or the shim's) — no filter ran to produce anything
// else.
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

} // namespace creo
