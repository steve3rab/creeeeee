#pragma once
#include "Error.hpp"
#include "ModelHandle.hpp"
#include "ObjectType.hpp"
#include "ProtoolkitCompat.hpp"
#include "Text.hpp"

#include <exception>
#include <optional>
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

class Feature : public ModelItem {
public:
  using ModelItem::ModelItem;

  void Regenerate(const ModelHandle &solid) const {
    CREO_CHECK(detail::FeatureRegenerate(
        solid.Raw(), const_cast<detail::RawModelItem *>(Raw())));
  }
};

namespace detail {

template <typename ActionFn, typename FilterFn> struct FeatureVisitContext {
  ActionFn &action;
  FilterFn &filter;
  std::exception_ptr exception;
};

template <typename ActionFn, typename FilterFn>
ProErrorCode FeatureVisitTrampoline(RawModelItem *feature, ProErrorCode status,
                                     RawAppData app_data) noexcept {
  auto *context =
      static_cast<FeatureVisitContext<ActionFn, FilterFn> *>(app_data);
  if (context->exception) {
    return static_cast<ProErrorCode>(-1);
  }
  try {
    return context->action(Feature(*feature), status);
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<ProErrorCode>(-1);
  }
}

template <typename ActionFn, typename FilterFn>
ProErrorCode FeatureFilterTrampoline(RawModelItem *feature,
                                      RawAppData app_data) noexcept {
  auto *context =
      static_cast<FeatureVisitContext<ActionFn, FilterFn> *>(app_data);
  try {
    return context->filter(Feature(*feature));
  } catch (...) {
    context->exception = std::current_exception();
    return static_cast<ProErrorCode>(-1);
  }
}

}

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

template <typename ActionFn>
ErrorCode VisitFeatures(const ModelHandle &solid, ActionFn &&action) {
  return VisitFeatures(
      solid, std::forward<ActionFn>(action),
      [](const Feature &) { return static_cast<ErrorCode>(0); });
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

}
