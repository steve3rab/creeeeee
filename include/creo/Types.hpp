#pragma once
// -----------------------------------------------------------------------
// Umbrella header: `#include "creo/Types.hpp"` pulls in every base type
// below in one go. New code may instead include only what it actually
// needs:
//
//   creo/Text.hpp          FixedWString<N>/FixedCharString<N> and all the
//                          text buffer aliases (Name, Line, Path, ...)
//   creo/Constants.hpp     MaxAssemLevel, ValueUnused, ValueDefault
//   creo/ObjectType.hpp    ObjectType, Boolean
//   creo/ModelHandle.hpp   ModelHandle
//   creo/ModelItem.hpp     ModelItem and its aliases (GeomItem, Feature,
//                          Dimension, ...)
//
// This split exists purely for cohesion (each header covers one concern)
// and does not change any type, name, or behavior: it is a pure
// reorganization of what used to be a single, growing Types.hpp.
// -----------------------------------------------------------------------

#include "creo/Constants.hpp"
#include "creo/ModelHandle.hpp"
#include "creo/ModelItem.hpp"
#include "creo/ObjectType.hpp"
#include "creo/Text.hpp"
