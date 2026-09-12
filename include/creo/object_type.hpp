#pragma once
#include "creo/detail/protoolkit_compat.hpp"

namespace creo {

// ---------------------------------------------------------------------------
// ObjectType
// ---------------------------------------------------------------------------
// Corresponds to `ProType` (struct pro_obj_types, ProObjects.h): the
// broad Creo database object type — NOT just models. Models
// (part/assembly/drawing/manufacturing/...) are only a small part of it,
// alongside features, curves, simulation entities, mesh entities,
// animation entities, etc. Re-exposed as-is (direct alias, no RAII
// wrapper: it is a plain tagged integer).
//
// A few "model" values for reference: PRO_ASSEMBLY (1), PRO_PART (2),
// PRO_DRAWING (4), PRO_MFG (37), PRO_SUB_ASSEMBLY (34), PRO_DWGFORM (33),
// PRO_LAYOUT (19), PRO_REPORT (105), PRO_MARKUP (116), PRO_DIAGRAM (121).
using ObjectType = detail::RawObjectType;

// ---------------------------------------------------------------------------
// MdlType
// ---------------------------------------------------------------------------
// Corresponds to `ProMdlType` (ProMdl.h): a narrower classification than
// ObjectType/ProType, covering only the "model-shaped" object types that
// ProMdlTypeGet can return (assembly, part, drawing, 3D/2D section,
// layout, dwgform, mfg, report, markup, diagram, ce_solid, ce_drawing,
// drw_solid). Each PRO_MDL_* enumerator reuses the exact same integer
// value as the corresponding PRO_* constant of ObjectType (e.g.
// PRO_MDL_ASSEMBLY == PRO_ASSEMBLY) — that equivalence comes directly
// from PTC's own enum definition, not something this wrapper assumes.
// See ModelHandle::Type() for the convenience method built on this.
using MdlType = detail::RawMdlType;

// ---------------------------------------------------------------------------
// Boolean
// ---------------------------------------------------------------------------
// Corresponds to `ProBoolean`/`ProBool` (ProToolkit.h, enum ProBooleans:
// PRO_B_FALSE = 0, PRO_B_TRUE = 1): the ProTOOLKIT boolean, a type
// distinct from C++ bool even though its two values numerically coincide
// with false/true. Many ProTOOLKIT functions take or return precisely
// this type (not a C++ bool): Boolean is re-exposed as-is (direct alias,
// like ObjectType) to stay interoperable with them without a risky
// implicit conversion.
using Boolean = detail::RawBoolean;

// Explicit conversions with C++ bool: avoid writing `x == creo::Boolean{}`
// (hard to read) or casting by hand on every call. ToBool() tests
// `!= PRO_B_FALSE` rather than `== PRO_B_TRUE` out of defensive caution:
// nothing guarantees that a ProTOOLKIT function will only ever return one
// of the two documented values for a Boolean.
constexpr bool ToBool(Boolean value) noexcept {
  return value != detail::kBooleanFalse;
}
constexpr Boolean ToProBoolean(bool value) noexcept {
  return value ? detail::kBooleanTrue : detail::kBooleanFalse;
}

} // namespace creo
