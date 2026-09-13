#pragma once
#include "creo/detail/ProtoolkitCompat.hpp"
#include "creo/Error.hpp"
#include "creo/ModelHandle.hpp"
#include "creo/ObjectType.hpp"
#include "creo/Text.hpp"

#include <exception>
#include <type_traits>
#include <utility>

namespace creo {

// ---------------------------------------------------------------------------
// ModelItem
// ---------------------------------------------------------------------------
// Corresponds to `pro_model_item` (ProObjects.h): a plain 3-field value
// struct {type, id, owner} that PTC gives roughly thirty different
// typedef names — ProGeomitem, ProFeature, ProDimension, ProNote,
// ProLayer, ... (see the aliases below) — one per kind of database
// object, even though they are bit-for-bit identical at the C level: a
// (type, id, owner) triple is how ProTOOLKIT identifies any database
// object within a model. Wrapped as a single class, exactly like PTC
// treats it as a single struct.
class ModelItem {
public:
  ModelItem() noexcept : raw_{} {}
  explicit ModelItem(const detail::RawModelItem &raw) noexcept : raw_(raw) {}

  ObjectType Type() const noexcept { return raw_.type; }
  int Id() const noexcept { return raw_.id; }
  ModelHandle Owner() const noexcept { return ModelHandle(raw_.owner); }

  // The item's name (ProModelitemNameGet). Returns a `Name` (ProName, 32
  // characters) — NOT a `ModelName` (ProMdlName, 180 characters):
  // ProModelitemNameGet names a database object (a feature, a dimension,
  // an explosion state, ...), which falls under "any other Creo
  // Parametric name" (see creo::Name), while ModelName/ProMdlName is
  // reserved specifically for the name of a whole ProMdl (see
  // ModelHandle::Name()). The two are easy to conflate since both are
  // ultimately "the name of a Pro-something", but mixing them up would
  // either truncate a wide name into too small a buffer, or waste space
  // — see the same warning on ModelName above.
  //
  // Named GetName(), not Name(): a member function named exactly like
  // the `creo::Name` type it returns does not compile (it shadows the
  // type within the class, including in its own return-type position).
  //
  // const_cast: ProModelitemNameGet takes a non-const `ProModelitem*` (a
  // pure "Get" function; this is a C API not being const-correct, not an
  // actual mutation of the item through that pointer).
  Name GetName() const {
    Name name;
    CREO_CHECK(detail::ModelitemNameGet(
        const_cast<detail::RawModelItem *>(&raw_), name.Raw()));
    return name;
  }

  // Raw struct, for direct calls to ProTOOLKIT functions not (yet)
  // wrapped by this library (most of them take a `ProModelitem*`).
  detail::RawModelItem *Raw() noexcept { return &raw_; }
  const detail::RawModelItem *Raw() const noexcept { return &raw_; }

private:
  detail::RawModelItem raw_;
};

// Two ModelItem are equal if they designate the same database object:
// same type, same id, same owning model — matching how ProTOOLKIT itself
// identifies a database object (a (type, id, owner) triple, not object
// identity of the C struct value).
inline bool operator==(const ModelItem &lhs, const ModelItem &rhs) noexcept {
  return lhs.Type() == rhs.Type() && lhs.Id() == rhs.Id() &&
         lhs.Owner() == rhs.Owner();
}
inline bool operator!=(const ModelItem &lhs, const ModelItem &rhs) noexcept {
  return !(lhs == rhs);
}

// ---------------------------------------------------------------------------
// Feature
// ---------------------------------------------------------------------------
// Unlike the other aliases below, `Feature` is a real derived class, not
// a bare `using Feature = ModelItem;`: it has genuine feature-specific
// behavior (Regenerate(), wrapping ProFeatureRegenerate) that would make
// no sense on a Layer, a Note, or a SolidBody. Adding no data member of
// its own — only a method — it stays exactly the same size/layout as
// ModelItem, with no virtual dispatch: this is a compile-time-only
// distinction (the compiler now tells a Feature apart from a Layer),
// not a runtime one. `using ModelItem::ModelItem;` inherits both of the
// base's constructors unchanged.
class Feature : public ModelItem {
public:
  using ModelItem::ModelItem;

  // Regenerates this feature (ProFeatureRegenerate). `solid` is the
  // part/assembly that owns it — matches Owner(), but PTC's API takes it
  // as a separate argument rather than reading it off the feature, so
  // this mirrors that rather than silently substituting Owner().
  //
  // const_cast: see ModelItem::GetName() above — same reasoning, a C
  // "Get"/"do a thing" API taking a non-const pointer for an operation
  // that does not mutate the (type, id, owner) triple itself.
  void Regenerate(const ModelHandle &solid) const {
    CREO_CHECK(detail::FeatureRegenerate(
        solid.Raw(), const_cast<detail::RawModelItem *>(Raw())));
  }
};

// ---------------------------------------------------------------------------
// VisitFeatures(): lambda-friendly wrapper around ProSolidFeatVisit
// ---------------------------------------------------------------------------
// PTC's "visit function" pattern (per its own "Visit Functions"
// documentation) is a pair of raw C callbacks — an "action" called once
// per item, and an optional "filter" called first to decide whether to
// call the action at all — communicating through a `ProAppData` (void*)
// the caller must thread through by hand. A raw C function pointer
// cannot capture state, so wrapping this by hand for every call site
// means routing everything through that void* yourself. VisitFeatures()
// does that once: pass ordinary lambdas (captures included), it takes
// care of the ProAppData plumbing.
namespace detail {

// Holds the two user callables plus a slot for a C++ exception that
// escaped one of them mid-visit: ProTOOLKIT's C stack frames are not
// exception-aware, so an exception must never unwind through them (that
// is undefined behavior) — it is stashed here instead, and rethrown by
// VisitFeatures() once ProSolidFeatVisit itself has returned.
template <typename ActionFn, typename FilterFn> struct FeatureVisitContext {
  ActionFn &action;
  FilterFn &filter;
  std::exception_ptr exception;
};

// Matches ProFeatureVisitAction's signature exactly (RawModelItem is
// ProFeature; ProErrorCode is ProError; RawAppData is ProAppData).
template <typename ActionFn, typename FilterFn>
ProErrorCode FeatureVisitTrampoline(RawModelItem *feature, ProErrorCode status,
                                     RawAppData app_data) noexcept {
  auto *context =
      static_cast<FeatureVisitContext<ActionFn, FilterFn> *>(app_data);
  if (context->exception) {
    // The filter already failed for this item (see below): PTC's own
    // control flow leaves no way to skip straight to termination from
    // there, so this call was unavoidable — do not also run the user's
    // action, just propagate the stop.
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
  try {
    return context->action(Feature(*feature), status);
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
}

// Matches ProFeatureFilterAction's signature exactly.
template <typename ActionFn, typename FilterFn>
ProErrorCode FeatureFilterTrampoline(RawModelItem *feature,
                                      RawAppData app_data) noexcept {
  auto *context =
      static_cast<FeatureVisitContext<ActionFn, FilterFn> *>(app_data);
  try {
    return context->filter(Feature(*feature));
  } catch (...) {
    context->exception = std::current_exception();
    // Anything other than PRO_TK_CONTINUE calls the action next (PTC's
    // own contract): the action trampoline above checks `exception` and
    // stops immediately without invoking the user's action for it.
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
}

} // namespace detail

// `action(Feature, ErrorCode status) -> ErrorCode`: called once per
// visited feature, `status` being whatever `filter` returned for it
// (PRO_TK_NO_ERROR if no filter was given). Same return contract as the
// raw ProFeatureVisitAction: PRO_TK_NO_ERROR continues visiting; any
// other value stops the visit and becomes VisitFeatures()'s own return
// value.
//
// `filter(Feature) -> ErrorCode`: called before `action` for each
// feature. Returning `PRO_TK_CONTINUE` (-7) skips the item (`action` is
// not called for it); any other value calls `action` with that value as
// `status`.
//
// Returns the raw code PTC's own documentation assigns to
// ProSolidFeatVisit: PRO_TK_NO_ERROR (every feature visited normally),
// PRO_TK_E_NOT_FOUND (no feature exists on `solid`), or whatever `action`
// returned to stop early. Deliberately not thrown as a ProToolkitError,
// unlike most of this wrapper: an empty result or an early stop is often
// exactly what the caller's own action/filter intended, not a failure —
// inspect the returned code yourself.
//
// A C++ exception thrown from `action` or `filter` is never let to
// unwind through ProTOOLKIT's C stack frames (see the trampolines
// above): it is rethrown here once control is back on the C++ side, in
// place of returning a code at all.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitFeatures(const ModelHandle &solid, ActionFn &&action,
                         FilterFn &&filter) {
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::FeatureVisitContext<ActionT, FilterT> context{action, filter,
                                                          nullptr};
  ErrorCode result = detail::SolidFeatVisit(
      solid.Raw(), &detail::FeatureVisitTrampoline<ActionT, FilterT>,
      &detail::FeatureFilterTrampoline<ActionT, FilterT>, &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}

// Overload with no filter: `action` is called for every feature, with
// status always PRO_TK_NO_ERROR (no filter ran to produce anything
// else).
template <typename ActionFn>
ErrorCode VisitFeatures(const ModelHandle &solid, ActionFn &&action) {
  return VisitFeatures(
      solid, std::forward<ActionFn>(action),
      [](const Feature &) { return static_cast<ErrorCode>(0); }); // PRO_TK_NO_ERROR
}

// PTC gives `pro_model_item` one typedef name per kind of database
// object; this wrapper mirrors that with one alias per name, all sharing
// the same ModelItem implementation (see the class comment above) —
// except Feature, which is a real derived class (see above) rather than
// a bare alias, since it alone has a per-type method so far.
using GeomItem = ModelItem;
using ExtObj = ModelItem;
using ProcStep = ModelItem;
using SimpRep = ModelItem;
using ExpldState = ModelItem;
using Layer = ModelItem;
using Dimension = ModelItem;
using DtlNote = ModelItem;
using DtlSymInst = ModelItem;
using Gtol = ModelItem;
using CompDisp = ModelItem;
using DwgTable = ModelItem;
using Note = ModelItem;
using AnnotationElem = ModelItem;
using Annotation = ModelItem;
using AnnotationPlane = ModelItem;
using Symbol = ModelItem;
using SurfFinish = ModelItem;
using MechItem = ModelItem;
using MaterialItem = ModelItem;
using CombState = ModelItem;
using LayerState = ModelItem;
using ApprnState = ModelItem;
using SolidBody = ModelItem;
using Ply = ModelItem;
using Table = ModelItem;

} // namespace creo
