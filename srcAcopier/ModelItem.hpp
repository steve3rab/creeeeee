#pragma once
#include "Error.hpp"
#include "ModelHandle.hpp"
#include "ObjectType.hpp"
#include "ProtoolkitCompat.hpp"
#include "Text.hpp"

#include <exception>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace creo {

class ModelItem {
public:
  ModelItem() noexcept : raw_{} {}
  explicit ModelItem(const detail::RawModelItem &raw) noexcept : raw_(raw) {}

  ObjectType Type() const noexcept { return raw_.type; }
  int Id() const noexcept { return raw_.id; }
  ModelHandle Owner() const noexcept { return ModelHandle(raw_.owner); }

  Name GetName() const {
    Name name;
    CREO_CHECK(detail::ModelitemNameGet(
        const_cast<detail::RawModelItem *>(&raw_), name.Raw()));
    return name;
  }

  detail::RawModelItem *Raw() noexcept { return &raw_; }
  const detail::RawModelItem *Raw() const noexcept { return &raw_; }

private:
  detail::RawModelItem raw_;
};

inline bool operator==(const ModelItem &lhs, const ModelItem &rhs) noexcept {
  return lhs.Type() == rhs.Type() && lhs.Id() == rhs.Id() &&
         lhs.Owner() == rhs.Owner();
}
inline bool operator!=(const ModelItem &lhs, const ModelItem &rhs) noexcept {
  return !(lhs == rhs);
}

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

class Feature : public ModelItem {
public:
  using ModelItem::ModelItem;

  void Regenerate(const ModelHandle &solid) const {
    CREO_CHECK(detail::FeatureRegenerate(
        solid.Raw(), const_cast<detail::RawModelItem *>(Raw())));
  }
};

namespace detail {

template <typename ItemT, typename ActionFn, typename FilterFn>
struct ModelItemVisitContext {
  ActionFn &action;
  FilterFn &filter;
  std::exception_ptr exception;
};

template <typename ItemT, typename ActionFn, typename FilterFn>
ProErrorCode ModelItemVisitTrampoline(RawModelItem *item, ProErrorCode status,
                                       RawAppData app_data) noexcept {
  auto *context =
      static_cast<ModelItemVisitContext<ItemT, ActionFn, FilterFn> *>(
          app_data);
  if (item == nullptr) {
    return static_cast<ProErrorCode>(-1);
  }
  if (context->exception) {
    return static_cast<ProErrorCode>(-1);
  }
  try {
    return context->action(ItemT(*item), status);
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<ProErrorCode>(-1);
  }
}

template <typename ItemT, typename ActionFn, typename FilterFn>
ProErrorCode ModelItemFilterTrampoline(RawModelItem *item,
                                        RawAppData app_data) noexcept {
  auto *context =
      static_cast<ModelItemVisitContext<ItemT, ActionFn, FilterFn> *>(
          app_data);
  if (item == nullptr) {
    return static_cast<ProErrorCode>(-1);
  }
  try {
    return context->filter(ItemT(*item));
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<ProErrorCode>(-1);
  }
}

}

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

template <typename ActionFn>
ErrorCode VisitFeatures(const ModelHandle &solid, ActionFn &&action) {
  return VisitFeatures(
      solid, std::forward<ActionFn>(action),
      [](const Feature &) { return static_cast<ErrorCode>(0); });
}

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

inline std::vector<Feature> CollectFeatures(const ModelHandle &solid) {
  std::vector<Feature> features;
  VisitFeatures(solid, [&](const Feature &feature, ErrorCode) {
    features.push_back(feature);
    return static_cast<ErrorCode>(0);
  });
  return features;
}

template <typename FilterFn>
std::vector<Feature> CollectFeatures(const ModelHandle &solid,
                                      FilterFn &&filter) {
  std::vector<Feature> features;
  VisitFeatures(
      solid,
      [&](const Feature &feature, ErrorCode) {
        features.push_back(feature);
        return static_cast<ErrorCode>(0);
      },
      std::forward<FilterFn>(filter));
  return features;
}

inline std::optional<Feature> FindFeatureByName(const ModelHandle &solid,
                                                 std::wstring_view name) {
  std::optional<Feature> found;
  VisitFeatures(solid, [&](const Feature &feature, ErrorCode) {
    if (feature.GetName().View() == name) {
      found = feature;
      return static_cast<ErrorCode>(-3);
    }
    return static_cast<ErrorCode>(0);
  });
  return found;
}

}
