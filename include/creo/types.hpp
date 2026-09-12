#pragma once
// -----------------------------------------------------------------------
// Umbrella header, kept for backward compatibility: existing code doing
// `#include "creo/types.hpp"` keeps pulling in every base type exactly as
// before. New code may instead include only what it actually needs:
//
//   creo/text.hpp          FixedWString<N>/FixedCharString<N> and all the
//                          text buffer aliases (Name, Line, Path, ...)
//   creo/constants.hpp     MaxAssemLevel, ValueUnused, ValueDefault
//   creo/object_type.hpp   ObjectType, Boolean
//   creo/model_handle.hpp  ModelHandle
//   creo/model_item.hpp    ModelItem and its aliases (GeomItem, Feature,
//                          Dimension, ...)
//
// This split exists purely for cohesion (each header covers one concern)
// and does not change any type, name, or behavior: it is a pure
// reorganization of what used to be a single, growing types.hpp.
// -----------------------------------------------------------------------

#include "creo/constants.hpp"
#include "creo/model_handle.hpp"
#include "creo/model_item.hpp"
#include "creo/object_type.hpp"
#include "creo/text.hpp"
