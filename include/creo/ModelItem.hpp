#pragma once
#include "creo/detail/ProtoolkitCompat.hpp"
#include "creo/Error.hpp"
#include "creo/ModelHandle.hpp"
#include "creo/ObjectType.hpp"
#include "creo/Text.hpp"

#include <exception>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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
  //
  // ProModelitemNameGet's own reference page (pasted verbatim by the
  // user) documents `Type()` as only valid for a specific list of
  // ProType values -- PRO_EDGE, PRO_SURFACE, PRO_FEATURE, PRO_CSYS,
  // PRO_AXIS, PRO_POINT, PRO_QUILT, PRO_CURVE, PRO_LAYER, PRO_DIMENSION,
  // PRO_REF_DIMENSION, PRO_NOTE, PRO_GTOL, PRO_SURF_FIN,
  // PRO_SYMBOL_INSTANCE, PRO_SET_DATUM_TAG, PRO_SIMP_REP,
  // PRO_EXPLD_STATE, PRO_ANNOTATION_ELEM, PRO_COMBINED_STATE -- any other
  // type returns PRO_TK_BAD_INPUTS (thrown here as a ProToolkitError,
  // like any other CREO_CHECK failure). Interesting in its own right:
  // several of those (Edge, Surface, Csys, Axis, Point, Quilt, Curve) are
  // the same PTC types confirmed elsewhere (ProUtilVisit.c, see
  // creo/Geometry.hpp) as being visited through their own OPAQUE-HANDLE
  // functions (ProSolidCsysVisit & co.) rather than as a `pro_model_item`
  // -- this does not contradict that: PTC evidently also lets a caller
  // identify one by the same generic (type, id, owner) triple as any
  // other database object for a lookup like this, on top of whatever
  // specialized handle its own geometry APIs use elsewhere. Not a reason
  // to add a `Csys`/`Axis`/... alias here, though: this class already
  // covers any `Type()`, aliased or not, and inventing an alias for the
  // *name* of PTC's own opaque handle type would risk colliding with it.
  //
  // GetName() throws on PRO_TK_E_NOT_FOUND ("the specified item does not
  // have a name", per the same reference page) exactly like any other
  // failure; see TryGetName() below for a std::optional-returning
  // alternative that treats that one specific code as "no name" rather
  // than an error.
  Name GetName() const {
    Name name;
    CREO_CHECK(detail::ModelitemNameGet(
        const_cast<detail::RawModelItem *>(&raw_), name.Raw()));
    return name;
  }

  // Same as GetName(), but returns std::nullopt instead of throwing when
  // ProModelitemNameGet reports PRO_TK_E_NOT_FOUND ("the specified item
  // does not have a name") -- a legitimate, documented outcome for some
  // item types, not a failure worth an exception, matching the same
  // Get()/TryGet() split already established for ModelHandle::GetCurrent()/
  // TryGetCurrent() and GetActive()/TryGetActive() (ModelHandle.hpp).
  // Any other failure (including PRO_TK_BAD_INPUTS for a `Type()` outside
  // the list documented on GetName() above) still throws: an invalid
  // item is a caller bug, not the same kind of "nothing there" outcome.
  std::optional<Name> TryGetName() const {
    Name name;
    detail::ProErrorCode err = detail::ModelitemNameGet(
        const_cast<detail::RawModelItem *>(&raw_), name.Raw());
    if (err == static_cast<ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
      return std::nullopt;
    }
    CREO_CHECK(err);
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

// PTC gives `pro_model_item` one typedef name per kind of database
// object; this wrapper mirrors that with one alias per name, all sharing
// the same ModelItem implementation (see the class comment above) —
// except Feature, which is a real derived class (see below) rather than
// a bare alias, since it alone has a per-type method so far. Declared
// here (rather than at the end of the file, where PTC's own header lists
// them) so that the visit functions further down — several of which
// visit one of these aliases rather than a Feature — can name them.
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
// the VisitXxx() entry point once the raw PTC visit call itself has
// returned. Parameterized on `ItemT` (Feature, or any of the plain
// ModelItem aliases such as ExpldState/Note/ProcStep/SimpRep/GeomItem)
// so the same engine below serves every "modelitem-style" visit function
// confirmed from PTC's own ProUtilVisit.c sample utility, not just
// ProSolidFeatVisit.
template <typename ItemT, typename ActionFn, typename FilterFn>
struct ModelItemVisitContext {
  ActionFn &action;
  FilterFn &filter;
  std::exception_ptr exception;
};

// Matches ProFeatureVisitAction's signature exactly (RawModelItem is
// pro_model_item, the same struct every ItemT below wraps; ProErrorCode
// is ProError; RawAppData is ProAppData) — and therefore also matches
// every other "modelitem-style" visit action confirmed from
// ProUtilVisit.c, since they all share that one struct shape.
template <typename ItemT, typename ActionFn, typename FilterFn>
ProErrorCode ModelItemVisitTrampoline(RawModelItem *item, ProErrorCode status,
                                       RawAppData app_data) noexcept {
  auto *context =
      static_cast<ModelItemVisitContext<ItemT, ActionFn, FilterFn> *>(
          app_data);
  if (item == nullptr) {
    // Defensive: every raw PTC visit function this engine serves is
    // documented to pass the address of a real item here, but a null
    // item would otherwise be dereferenced below (undefined behavior) --
    // fail loudly instead of trusting that unconditionally.
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
  if (context->exception) {
    // The filter already failed for this item (see below): PTC's own
    // control flow leaves no way to skip straight to termination from
    // there, so this call was unavoidable — do not also run the user's
    // action, just propagate the stop.
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
  try {
    return context->action(ItemT(*item), status);
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
}

// Matches ProFeatureFilterAction's signature exactly (see above).
template <typename ItemT, typename ActionFn, typename FilterFn>
ProErrorCode ModelItemFilterTrampoline(RawModelItem *item,
                                        RawAppData app_data) noexcept {
  auto *context =
      static_cast<ModelItemVisitContext<ItemT, ActionFn, FilterFn> *>(
          app_data);
  if (item == nullptr) {
    // Defensive: see ModelItemVisitTrampoline's identical guard above.
    return static_cast<ProErrorCode>(-1); // PRO_TK_GENERAL_ERROR
  }
  try {
    return context->filter(ItemT(*item));
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
//
// Throws std::logic_error if `solid` itself is invalid (null), before
// even attempting the raw PTC call — same guard and rationale as
// ModelHandle::Name()/Type()/etc. (ModelHandle.hpp): a caller bug (an
// invalid handle) is a different kind of problem than "the visit found
// nothing" above, so it is not folded into the raw-code contract.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitFeatures(const ModelHandle &solid, ActionFn &&action,
                         FilterFn &&filter) {
  if (!solid.IsValid()) {
    throw std::logic_error(
        "creo::VisitFeatures() called with an invalid (null) ModelHandle");
  }
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::ModelItemVisitContext<Feature, ActionT, FilterT> context{
      action, filter, nullptr};
  ErrorCode result = detail::SolidFeatVisit(
      solid.Raw(), &detail::ModelItemVisitTrampoline<Feature, ActionT, FilterT>,
      &detail::ModelItemFilterTrampoline<Feature, ActionT, FilterT>,
      &context);
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

// ---------------------------------------------------------------------------
// VisitExpldStates() / VisitNotes() / VisitProcSteps() / VisitSimpReps() /
// VisitGeomitems(): four more visit functions confirmed from PTC's own
// ProUtilVisit.c sample utility (ProUtilCollectExpldStates(),
// ProUtilVisitNotes(), ProUtilVisitProcsteps(), ProUtilVisitSimpreps(),
// ProUtilVisitGeomitems() — names paraphrased, the actual raw calls are
// ProSolidExpldstateVisit/ProMdlNoteVisit/ProProcstepVisit/
// ProSolidSimprepVisit/ProFeatureGeomitemVisit). All five share
// VisitFeatures()'s exact contract (status/return codes, filter
// PRO_TK_CONTINUE convention, exception-safety) and are built on the same
// detail::ModelItemVisitContext engine above — only the underlying raw
// PTC call and the visited item type differ. See VisitFeatures() above
// for the full contract documentation, not repeated per function here.
// ---------------------------------------------------------------------------

// Wraps ProSolidExpldstateVisit: visits the exploded states of `assembly`.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitExpldStates(const ModelHandle &assembly, ActionFn &&action,
                            FilterFn &&filter) {
  if (!assembly.IsValid()) {
    throw std::logic_error(
        "creo::VisitExpldStates() called with an invalid (null) "
        "ModelHandle");
  }
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::ModelItemVisitContext<ExpldState, ActionT, FilterT> context{
      action, filter, nullptr};
  ErrorCode result = detail::SolidExpldstateVisit(
      assembly.Raw(),
      &detail::ModelItemVisitTrampoline<ExpldState, ActionT, FilterT>,
      &detail::ModelItemFilterTrampoline<ExpldState, ActionT, FilterT>,
      &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}
template <typename ActionFn>
ErrorCode VisitExpldStates(const ModelHandle &assembly, ActionFn &&action) {
  return VisitExpldStates(
      assembly, std::forward<ActionFn>(action),
      [](const ExpldState &) { return static_cast<ErrorCode>(0); });
}

// Wraps ProMdlNoteVisit: visits the notes of `model`.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitNotes(const ModelHandle &model, ActionFn &&action,
                      FilterFn &&filter) {
  if (!model.IsValid()) {
    throw std::logic_error(
        "creo::VisitNotes() called with an invalid (null) ModelHandle");
  }
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::ModelItemVisitContext<Note, ActionT, FilterT> context{
      action, filter, nullptr};
  ErrorCode result = detail::MdlNoteVisit(
      model.Raw(), &detail::ModelItemVisitTrampoline<Note, ActionT, FilterT>,
      &detail::ModelItemFilterTrampoline<Note, ActionT, FilterT>, &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}
template <typename ActionFn>
ErrorCode VisitNotes(const ModelHandle &model, ActionFn &&action) {
  return VisitNotes(model, std::forward<ActionFn>(action),
                     [](const Note &) { return static_cast<ErrorCode>(0); });
}

// Wraps ProProcstepVisit: visits the process steps of `solid`.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitProcSteps(const ModelHandle &solid, ActionFn &&action,
                          FilterFn &&filter) {
  if (!solid.IsValid()) {
    throw std::logic_error(
        "creo::VisitProcSteps() called with an invalid (null) ModelHandle");
  }
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::ModelItemVisitContext<ProcStep, ActionT, FilterT> context{
      action, filter, nullptr};
  ErrorCode result = detail::ProcstepVisit(
      solid.Raw(),
      &detail::ModelItemVisitTrampoline<ProcStep, ActionT, FilterT>,
      &detail::ModelItemFilterTrampoline<ProcStep, ActionT, FilterT>,
      &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}
template <typename ActionFn>
ErrorCode VisitProcSteps(const ModelHandle &solid, ActionFn &&action) {
  return VisitProcSteps(
      solid, std::forward<ActionFn>(action),
      [](const ProcStep &) { return static_cast<ErrorCode>(0); });
}

// Wraps ProSolidSimprepVisit: visits the simplified representations of
// `solid`. The raw PTC function takes filter before action (see
// detail::SolidSimprepVisit in ProtoolkitCompat.hpp, which already
// reorders them back) — nothing special needed at this level.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitSimpReps(const ModelHandle &solid, ActionFn &&action,
                         FilterFn &&filter) {
  if (!solid.IsValid()) {
    throw std::logic_error(
        "creo::VisitSimpReps() called with an invalid (null) ModelHandle");
  }
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::ModelItemVisitContext<SimpRep, ActionT, FilterT> context{
      action, filter, nullptr};
  ErrorCode result = detail::SolidSimprepVisit(
      solid.Raw(),
      &detail::ModelItemVisitTrampoline<SimpRep, ActionT, FilterT>,
      &detail::ModelItemFilterTrampoline<SimpRep, ActionT, FilterT>,
      &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}
template <typename ActionFn>
ErrorCode VisitSimpReps(const ModelHandle &solid, ActionFn &&action) {
  return VisitSimpReps(
      solid, std::forward<ActionFn>(action),
      [](const SimpRep &) { return static_cast<ErrorCode>(0); });
}

// Wraps ProFeatureGeomitemVisit: visits the geometry items of kind
// `item_type` (e.g. PRO_SURFACE, PRO_EDGE) owned by `feature`. Unlike the
// four visit functions above, this one is scoped to a Feature rather than
// a whole ModelHandle, and takes the extra `item_type` selector PTC's raw
// call requires.
template <typename ActionFn, typename FilterFn>
ErrorCode VisitGeomitems(const Feature &feature, ObjectType item_type,
                          ActionFn &&action, FilterFn &&filter) {
  using ActionT = std::remove_reference_t<ActionFn>;
  using FilterT = std::remove_reference_t<FilterFn>;
  detail::ModelItemVisitContext<GeomItem, ActionT, FilterT> context{
      action, filter, nullptr};
  ErrorCode result = detail::FeatureGeomitemVisit(
      const_cast<detail::RawModelItem *>(feature.Raw()), item_type,
      &detail::ModelItemVisitTrampoline<GeomItem, ActionT, FilterT>,
      &detail::ModelItemFilterTrampoline<GeomItem, ActionT, FilterT>,
      &context);
  if (context.exception) {
    std::rethrow_exception(context.exception);
  }
  return result;
}
template <typename ActionFn>
ErrorCode VisitGeomitems(const Feature &feature, ObjectType item_type,
                          ActionFn &&action) {
  return VisitGeomitems(
      feature, item_type, std::forward<ActionFn>(action),
      [](const GeomItem &) { return static_cast<ErrorCode>(0); });
}

// ---------------------------------------------------------------------------
// CollectFeatures() / FindFeatureByName(): convenience wrappers built on
// VisitFeatures(), mirroring PTC's own ProUtilCollectSolidFeatures()/
// ProUtilCollectSolidFeaturesWithFilter()/ProUtilFindFeatureByName()
// (PTC's ProUtilVisit.c sample utility, pasted verbatim by the user) --
// no new PTC binding needed for either, both are plain C++ built on top
// of the already-wrapped VisitFeatures()/ModelItem::GetName().
// ---------------------------------------------------------------------------

// Collects every feature of `solid` into a std::vector<Feature>. Unlike
// PTC's own ProUtilCollectSolidFeatures(), there is no ProArray to
// allocate/free by hand: VisitFeatures() already owns the visit itself,
// a plain std::vector is enough on this side. PRO_TK_E_NOT_FOUND (no
// feature exists) is not an error here, it is simply an empty vector --
// this wrapper's own action always returns PRO_TK_NO_ERROR, so
// VisitFeatures() only ever stops "early" by finding nothing to visit
// at all, never by this function's own choice.
inline std::vector<Feature> CollectFeatures(const ModelHandle &solid) {
  std::vector<Feature> features;
  VisitFeatures(solid, [&](const Feature &feature, ErrorCode) {
    features.push_back(feature);
    return static_cast<ErrorCode>(0); // PRO_TK_NO_ERROR: keep visiting
  });
  return features;
}

// Same as above, but only collects features `filter` selects (mirrors
// ProUtilCollectSolidFeaturesWithFilter()): `filter` follows
// VisitFeatures()'s own filter contract (PRO_TK_CONTINUE skips the
// item; anything else collects it).
template <typename FilterFn>
std::vector<Feature> CollectFeatures(const ModelHandle &solid,
                                      FilterFn &&filter) {
  std::vector<Feature> features;
  VisitFeatures(
      solid,
      [&](const Feature &feature, ErrorCode) {
        features.push_back(feature);
        return static_cast<ErrorCode>(0); // PRO_TK_NO_ERROR
      },
      std::forward<FilterFn>(filter));
  return features;
}

// Returns the first feature of `solid` whose name (GetName(),
// ProModelitemNameGet) matches `name`, or std::nullopt if none does.
// Stops visiting as soon as a match is found (PRO_TK_USER_ABORT, PTC's
// own established code for exactly this "found what I was looking for"
// case, per ProUtilFindFeatureByName()) rather than visiting every
// remaining feature once the answer is already known.
inline std::optional<Feature> FindFeatureByName(const ModelHandle &solid,
                                                 std::wstring_view name) {
  std::optional<Feature> found;
  VisitFeatures(solid, [&](const Feature &feature, ErrorCode) {
    if (feature.GetName().View() == name) {
      found = feature;
      return static_cast<ErrorCode>(-3); // PRO_TK_USER_ABORT: stop, found it
    }
    return static_cast<ErrorCode>(0); // PRO_TK_NO_ERROR: keep looking
  });
  return found;
}

} // namespace creo
