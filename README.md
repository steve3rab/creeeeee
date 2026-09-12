# creeeeee

C++17 wrapper for **ProTOOLKIT**, PTC's C API for Creo Parametric.
Targets **Creo Parametric 10.0**.

## About

ProTOOLKIT exposes a low-level C API (opaque handles, fixed-size text
buffers, integer error codes) for building applications that integrate
with Creo Parametric. This repository provides a C++17 layer on top of
that API to make it safer and more idiomatic to use:

- strong types for ProTOOLKIT text buffers (`ProName`, `ProMdlName`,
  `ProLine`, `ProPath`, `ProComment`, `ProValue`, ...) with checked
  conversions to/from `std::wstring` and `std::string`;
- a typed model handle (`ProMdl`);
- C++ exceptions (`creo::ProToolkitError`) instead of `ProError` codes to
  check manually after every call.

The project starts with these base building blocks (types + error
handling); more wrappers (features, parameters, geometry, UI...) will be
added over time.

### Important prerequisites

The ProTOOLKIT SDK is **proprietary** (shipped by PTC with Creo) and is
not included in this repository. To build against the real Creo API, you
need a Creo Parametric 10.0 installation with the ProTOOLKIT SDK.

Without this SDK, the project still builds: `include/creo/detail/`
automatically falls back to a "shim" mode (substitute types, see the
comments in `protoolkit_compat.hpp` and `protoolkit_shim.hpp`) which lets
you develop and test the wrapper's logic away from a Creo workstation. An
executable built in this mode obviously cannot drive a real Creo session.

## Repository layout

```
include/creo/
  types.hpp                       Umbrella header: includes the five below
  text.hpp                        FixedWString/FixedCharString + all text aliases
  constants.hpp                   MaxAssemLevel, ValueUnused, ValueDefault
  object_type.hpp                 ObjectType, Boolean
  model_handle.hpp                ModelHandle
  model_item.hpp                  ModelItem, Feature, and the rest of the aliases
  array.hpp                       Array<T>: RAII around ProArray
  error.hpp                       ProToolkitError + CREO_CHECK macro
  detail/protoolkit_compat.hpp    Real SDK / shim switch
  detail/protoolkit_shim.hpp      Substitute types (no SDK)
src/
  error.cpp
examples/
  hello_creo.cpp                  Tour of types/errors/Array + active model name
cmake/
  FindProToolkit.cmake            Locates the installed ProTOOLKIT SDK
srcAcopier/
  types.hpp, text.hpp,            Flat version (no subfolders, no
  constants.hpp,                  comments) to drop into a real
  object_type.hpp,                ProTOOLKIT project (SDK + license
  model_handle.hpp,                available): CREO_WRAPPER_HAS_REAL_SDK
  model_item.hpp, array.hpp,      is hardcoded to 1 there (no shim
  error.hpp,                      mode, the real SDK is required to
  protoolkit_compat.hpp,          build). Same file split as
  utf8.hpp, error.cpp             include/creo/ above.
```

Existing code that does `#include "creo/types.hpp"` keeps working exactly
as before: it is now a thin umbrella pulling in `text.hpp`/
`constants.hpp`/`object_type.hpp`/`model_handle.hpp`/`model_item.hpp`.
New code may include only the specific header(s) it needs instead — this
split is a pure reorganization for cohesion (each header covers one
concern), it changes no type, name, or behavior.

## Building

```bash
# Without the Creo SDK (shim mode, for developing/testing the wrapper):
cmake -S . -B build
cmake --build build

# With the Creo 10 SDK (for a build usable in a Creo session):
cmake -S . -B build \
  -DCREO_TOOLKIT_ROOT="/path/to/Creo 10.0.0.0/Common Files" \
  -DCREO_TOOLKIT_ARCH=<sdk_arch_folder_name>
cmake --build build
```

`CREO_TOOLKIT_ROOT` and `CREO_TOOLKIT_ARCH` can also be supplied as
environment variables. See `cmake/FindProToolkit.cmake` for the paths it
searches.

## Usage

See `examples/hello_creo.cpp` for a complete, commented example — it runs
even without the SDK (shim mode) since it first shows the types and error
handling independently of any Creo session, before the part that actually
requires the SDK (retrieving the active model):

```bash
cmake -S . -B build && cmake --build build
./build/hello_creo
```

Excerpt of the part that requires a real Creo session:

```cpp
#include "creo/error.hpp"
#include "creo/types.hpp"

creo::detail::RawMdl raw_model = nullptr;
CREO_CHECK(ProMdlCurrentGet(&raw_model));

creo::ModelHandle model(raw_model);
// model.Name() wraps CREO_CHECK + ProMdlMdlNameGet (which replaces
// ProMdlNameGet, now deprecated in Creo 10) and throws std::logic_error
// if the handle is invalid, without even attempting the ProTOOLKIT call.
creo::ModelName name = model.Name();

std::printf("Active model: %s\n", name.ToString().c_str());
```

Note: never call both `std::wprintf` and `std::printf` on `stdout` in the
same program — once a C stream is oriented by the first call ("wide" or
"narrow"), using the other orientation afterward is undefined behavior.
`ToString()` (UTF-8) is enough to print everything with `printf`,
including the content of a wide buffer like `ProName`.

`CREO_CHECK` wraps any ProTOOLKIT call returning a `ProError` and throws
a `creo::ProToolkitError` on failure, with the native error code
accessible via `.code()`. The exception's message includes the code's
symbolic label (`creo::ToString`), which covers the entire official
Creo 10 `ProError`/`ProErr` enum (`PRO_TK_BAD_INPUTS`, `PRO_TK_NO_LICENSE`,
...) — not just the raw number.

### std::string / std::wstring conversions

`Name`, `Line`, `Path` and `ModelName` (all based on `FixedWString<N>`)
can be read from and constructed from both representations:

```cpp
creo::Line l1(L"deformed_part");     // from a wide literal
creo::Line l2("deformed_part");      // from a UTF-8 std::string

std::wstring w = l1.ToWString();     // wide, as stored by ProTOOLKIT
std::string  s = l1.ToString();      // UTF-8
```

UTF-8 is used as the `std::string` representation because `wchar_t` is
not the same size across platforms (UTF-16 on Windows, UTF-32 on
Linux/macOS): see `include/creo/detail/utf8.hpp` for the conversion
details, with no external library dependency. The conversion strictly
validates its input in both directions (truncated/malformed UTF-8
sequences, overlong encodings, isolated UTF-16 surrogates, code points
outside the Unicode range): anything invalid is replaced with the
`U+FFFD` replacement character rather than being silently let through or
crashing the conversion — useful since this text may come from an
external model file.

Every text type exposes `kCapacity` (the buffer's total size, including
the terminator) and `kMaxLength = kCapacity - 1` (the number of
characters that can actually be stored). It is `kMaxLength`, not
`kCapacity`, that bounds the size accepted by `Assign()`/the constructor.

### ProTOOLKIT input/output functions (Get / Set)

Many ProTOOLKIT functions share the same C shape —
`ProError Xxx(ProName option, ProPath option_value)` — whether the buffer
is used as input (`...Set`) or output (`...Get`); nothing in the type
says which, it is a PTC documentation convention. The wrapper is used the
same way in both cases, only the way the object is constructed changes:

```cpp
#include "creo/error.hpp"
#include "creo/types.hpp"

// --- Set: the buffer is already filled before the call (input) ---
creo::Name option("pro_line_font");
creo::Path option_value("solid");
CREO_CHECK(ProConfigoptSet(option, option_value));

// --- Get: the buffer is empty before the call, filled by ProTOOLKIT (output) ---
creo::Name option2("pro_line_font");
creo::Path option_value2;                    // empty, to be filled
CREO_CHECK(ProConfigoptionGet(option2, option_value2));

std::string value = option_value2.ToString(); // or .ToWString()
```

And to go from a `std::string`/`std::wstring` back to a `ProPath` (for
example for a new `...Set` call with a computed value):

```cpp
std::string new_value = "hidden";

creo::Path p1(new_value);        // builds a new Path
option_value.Assign(new_value);  // or reuses an existing Path

CREO_CHECK(ProConfigoptSet(option, option_value));
```

`Assign()` (like the constructor) checks the target buffer's capacity
(`Path` = 260 characters) and throws `std::length_error` rather than
silently truncating an overly long value.

### Comparisons, `View()` and interoperability

All text types (`Name`, `Line`, `Path`, `ModelName`, `CharName`, ...) are
directly comparable, both to each other and against a C++ string:

```cpp
creo::Name a(L"gear_01");
creo::ModelName b(L"gear_01");   // different capacity, comparison OK

if (a == b) { /* ... */ }
if (a == L"gear_01") { /* ... */ }                  // wide literal
if (a == std::wstring_view(L"gear_01")) { }         // wstring_view / wstring

creo::CharName cn("MENU_A");
if (cn == "MENU_A") { /* ... */ }                   // char literal
```

These operators compare **content** (via `View()`, below), never the
buffer's address — a point worth making explicit: every type also
exposes an implicit `operator wchar_t*()`/`operator char*()`, needed for
direct interop with ProTOOLKIT C functions
(`ProConfigoptSet(option, option_value)`). Without a dedicated
`const wchar_t*`/`const char*` overload in addition to the
`wstring_view`/`string_view` ones, comparing against a literal would be
ambiguous for the compiler (two implicit conversions of the same rank:
to a pointer, or to a view); these overloads exist precisely to resolve
that ambiguity in favor of the content-based comparison.

`View()` returns a `std::wstring_view`/`std::string_view` over the buffer
with no copy (unlike `ToWString()`/`ToString()`, which allocate a new one
on every call):

```cpp
std::wstring_view v = path.View();
if (v.substr(v.size() - 4) == L".prt") { /* ... */ }
```

(`std::wstring_view::ends_with` is C++20; this wrapper targets C++17.)

`Path` also interfaces with `std::filesystem::path`:

```cpp
#include "creo/types.hpp"

std::filesystem::path fs = creo::ToFilesystemPath(path);
creo::Path p = creo::PathFromFilesystem(fs / "subdirectory" / "part.prt");
```

`creo::ValueUnused` corresponds to `PRO_VALUE_UNUSED` (= -1), the
"value/index not used" sentinel accepted by many ProTOOLKIT functions
(e.g. any negative index passed to `ProArrayObjectAdd` appends at the end
of the array — `ValueUnused` is one example of that, not the only value
that triggers this behavior). `creo::ValueDefault` corresponds to
`PRO_VALUE_DEFAULT` (= -5), the "default value" sentinel — distinct from
`PRO_VALUE_UNUSED` despite the similar names, do not confuse the two in a
ProTOOLKIT call. In real-SDK mode, both directly reuse the PTC macros.

### Boolean

`creo::Boolean` corresponds to `ProBoolean`/`ProBool` (`ProToolkit.h`,
enum `ProBooleans`: `PRO_B_FALSE = 0`, `PRO_B_TRUE = 1`) — the ProTOOLKIT
boolean, a type distinct from C++ `bool` even though its two values
numerically coincide with `false`/`true`. Many ProTOOLKIT functions take
or return precisely this type, never a C++ `bool`:

```cpp
creo::Boolean flag = creo::ToProBoolean(true);
// ... CREO_CHECK(SomeProtoolkitFunction(..., flag));

bool value = creo::ToBool(flag);
```

`ToBool()` tests `!= PRO_B_FALSE` rather than `== PRO_B_TRUE`, out of
defensive caution against a value that would be neither of the two
documented ones.

### ModelHandle

Beyond `IsValid()`/`Raw()`, `ModelHandle` exposes a convenience method for
the most common case:

```cpp
creo::ModelHandle model(raw_model);
creo::ModelName name = model.Name();  // CREO_CHECK(ProMdlMdlNameGet(...)) built in
```

`Name()` throws `std::logic_error` (not a `ProToolkitError`) if the
handle is invalid (null): the error is detected before even attempting
the ProTOOLKIT call, with a clearer message than a generic
`PRO_TK_BAD_INPUTS` coming back from the SDK.

Two `ModelHandle` are comparable by equality — they refer to the same
model if and only if they carry the same underlying ProTOOLKIT handle
(not just the same name, since two distinct models can share a generic
name):

```cpp
if (model1 == model2) { /* same model */ }
```

### ModelItem

`creo::ModelItem` corresponds to `pro_model_item` (`ProObjects.h`): a
plain 3-field value struct `{type, id, owner}` that PTC gives roughly
thirty different typedef names — `ProGeomitem`, `ProFeature`,
`ProDimension`, `ProNote`, `ProLayer`, `ProGtol`, `ProSolidBody`, ... —
one per kind of database object, even though they are bit-for-bit
identical at the C level: a `(type, id, owner)` triple is how ProTOOLKIT
identifies any database object within a model. This wrapper mirrors that
with a single `ModelItem` class and one alias per PTC name, all sharing
the same implementation:

```cpp
creo::GeomItem
creo::ExtObj
creo::Feature
creo::ProcStep
creo::SimpRep
creo::ExpldState
creo::Layer
creo::Dimension
creo::DtlNote
creo::DtlSymInst
creo::Gtol
creo::CompDisp
creo::DwgTable
creo::Note
creo::AnnotationElem
creo::Annotation
creo::AnnotationPlane
creo::Symbol
creo::SurfFinish
creo::MechItem
creo::MaterialItem
creo::CombState
creo::LayerState
creo::ApprnState
creo::SolidBody
creo::Ply
creo::Table
```

```cpp
// Type()/Id()/Owner() are plain field reads: they work on any ModelItem
// value, including one built by hand (e.g. in a test), with no
// ProTOOLKIT call and no real Creo session needed.
creo::Feature feat(raw_feature);   // raw_feature: detail::RawModelItem
creo::ObjectType t = feat.Type();
int id = feat.Id();
creo::ModelHandle owner = feat.Owner();

// GetName() does need a real session (ProModelitemNameGet).
creo::Name name = feat.GetName();
```

`GetName()` returns a `Name` (`ProName`, 32 characters) — **not** a
`ModelName` (`ProMdlName`, 180 characters): `ProModelitemNameGet` names a
database object (a feature, a dimension, an explosion state, ...), which
falls under "any other Creo Parametric name", while `ModelName`/
`ProMdlName` is reserved specifically for the name of a whole `ProMdl`
(see `ModelHandle::Name()` above) — the two are easy to conflate since
both are ultimately "the name of a Pro-something". The method is named
`GetName()`, not `Name()`: a member function named exactly like the
`creo::Name` type it returns does not compile (it shadows the type
within the class, including in its own return-type position).

Two `ModelItem` are comparable by equality — they designate the same
database object if their type, id, and owning model all match, matching
how ProTOOLKIT itself identifies a database object:

```cpp
if (feat1 == feat2) { /* same feature */ }
```

`Feature` is the one exception to "all 27 names are plain aliases of
`ModelItem`": it is a real derived class (`class Feature : public
ModelItem`), because it has one feature-specific operation —
`Regenerate()` (`ProFeatureRegenerate`) — that would not make sense on a
`Layer`, a `Note`, or a `SolidBody`. It adds no data member of its own,
so it stays exactly the same size/layout as `ModelItem`, with no virtual
dispatch: this is a compile-time-only distinction (the compiler tells a
`Feature` apart from any other alias), not a runtime one.

```cpp
creo::Feature feat(raw_feature);
feat.Regenerate(feat.Owner());  // CREO_CHECK(ProFeatureRegenerate(...))
```

> **Caveat**: `ProFeatureRegenerate`'s exact signature
> (`ProError ProFeatureRegenerate(ProSolid solid, ProFeature *feature)`)
> was given as a description of the API's usual shape, not copied from a
> real header — unlike the constants/enums elsewhere in this wrapper,
> confirmed from pasted PTC header content. `Regenerate()` also assumes
> `ProSolid` is interchangeable with `ProMdl` (as described), rather than
> introducing a separate `Solid` type. If a real SDK's `ProSolid` turns
> out to be a distinct, incompatible type, the real-SDK build will fail
> to compile right at this trampoline (`detail::FeatureRegenerate` in
> `detail/protoolkit_compat.hpp`) — loudly, not silently wrong — and the
> fix is local to that one line.

Only `Feature` has been promoted to a real class so far, on the strength
of this one concrete per-type operation. The other 26 aliases
(`GeomItem`, `Dimension`, `Layer`, `Note`, ...) stay plain aliases of
`ModelItem` until a similarly concrete need for each shows up — turning
all of them into distinct classes speculatively, before any of them has
actual per-type behavior, was considered and deliberately deferred.

### Array&lt;T&gt;

`creo::Array<T>` (`include/creo/array.hpp`) wraps `ProArray`
(`ProArray.h`): ProTOOLKIT's generic dynamic array. Unlike `ModelHandle`
(non-owning — Creo manages a model's lifetime), a `ProArray` is
explicitly allocated/freed by the caller: `Array<T>` therefore takes full
RAII ownership of it (allocates on construction, frees in the
destructor).

```cpp
#include "creo/array.hpp"

creo::Array<int> values(0, 8); // empty, grows in blocks of 8 elements
values.Append(10);
values.Append(20);
values.Insert(1, 15);           // -> 10, 15, 20
values.Remove(0);               // -> 15, 20

for (int v : values) { /* ... */ }   // standard iteration (begin()/end())
int v = values[0];                    // unchecked access, like std::vector
int w = values.At(0);                 // checked access, throws std::out_of_range

// Take ownership of a ProArray already allocated by another ProTOOLKIT
// function (instead of allocating a new one):
creo::Array<ProFeature> feats = creo::Array<ProFeature>::Adopt(raw_pro_array);
```

**`T` can be any C++ type** (`std::string`, a class with a
destructor/owned members, ...), not just a trivially copyable one — see
below for why, and why that required an implementation different from
what a plain call to the native ProTOOLKIT functions would give.

Key points:
- **Movable, not copyable**: ProTOOLKIT offers no duplication primitive;
  a deep, element-by-element copy would be expensive and surprising to
  pass off as a plain copy constructor.
- **Genuinely C++ memory management, not C**: the native functions
  `ProArrayObjectAdd`/`ProArrayObjectRemove`/`ProArraySizeSet` move
  elements by raw memory copy (`memmove`/`realloc` on the C side), never
  calling a C++ constructor/destructor — safe for a trivially copyable
  type, but would corrupt one that isn't (`std::string` can store an
  internal pointer into its own buffer; moving it via `memmove` invalidates
  it). `Array<T>` therefore only uses `ProArray` as a raw memory provider
  (`ProArrayAlloc`/`ProArrayFree`): all element lifetime management
  (construction, destruction, moving on growth or on a shift) is
  implemented in pure C++ — exactly like `std::vector` on top of its
  allocator — with the strong exception guarantee on `Reserve()`
  (preferring a copy over a move when the latter could throw, via
  `std::move_if_noexcept`, as the standard library itself does). Result:
  no type restriction visible to the wrapper's user.
- `Insert(index, ...)`: a negative `index` is equivalent to inserting at
  the end of the array (same bounds as `Append`).
- `Adopt()`, on the other hand, is specifically restricted to a trivially
  copyable `T` (checked via `static_assert`): a `ProArray` built by
  ProTOOLKIT itself can only ever hold C data, never already-constructed
  C++ objects.

In shim mode (no SDK), `ProArrayAlloc`/`ProArrayFree` (and
`ProArraySizeGet`, used by `Adopt()`) are reimplemented functionally
rather than being plain substitute types: unlike `ProMdl`/`ProError`,
`Array<T>` has real behavior to exercise to be testable without Creo
installed. The shim also reproduces
`ProArraySizeSet`/`ProArrayObjectAdd`/`ProArrayObjectRemove` for fidelity
to `ProArray.h`, even though `Array<T>` no longer uses them (see above).

### ObjectType

`creo::ObjectType` corresponds to `ProType` (`pro_obj_types`,
`ProObjects.h`): the Creo database object type **in the broad sense**,
not just models. Models in the everyday sense
(part/assembly/drawing/manufacturing/...) are only a small part of it,
alongside features, curves, simulation entities, mesh entities,
animation entities, etc. — several hundred values in total.

```cpp
creo::ObjectType t = creo::ObjectType::PRO_PART;
if (t == creo::ObjectType::PRO_ASSEMBLY) { /* ... */ }
```

A few "model" values for reference: `PRO_ASSEMBLY` (1), `PRO_PART` (2),
`PRO_DRAWING` (4), `PRO_MFG` (37), `PRO_SUB_ASSEMBLY` (34), `PRO_DWGFORM`
(33), `PRO_LAYOUT` (19), `PRO_REPORT` (105), `PRO_MARKUP` (116),
`PRO_DIAGRAM` (121).

One value from the real enum has been omitted: `PRO_TYPE_UNUSED` (defined
on the PTC side as `= PRO_VALUE_UNUSED`), because `PRO_VALUE_UNUSED` had
not been provided and its value was not to be guessed. A build with the
real SDK gets it normally via the PTC header; only shim mode (no SDK)
does not offer it.

### Available types (`include/creo/types.hpp`)

Two families of fixed-size text buffers, depending on what PTC uses on
the C side (see `detail/protoolkit_compat.hpp`/`protoolkit_shim.hpp` for
the details of the `PRO_*_SIZE` constants):

**Wide buffers (`wchar_t[N]`, `FixedWString<N>` template)** — most
ProTOOLKIT text types since Pro/ENGINEER Wildfire:

| C++ type             | ProTOOLKIT buffer      | Size | Usage                                    |
|----------------------|------------------------|-----:|-------------------------------------------|
| `Name`               | `ProName`              |   32 | Generic name (feature, parameter, ...)   |
| `ModelName`          | `ProMdlName`           |  180 | A model's name (ProMdlMdlNameGet)         |
| `Line`               | `ProLine`              |   81 | Line of text (messages)                   |
| `Path`               | `ProPath`              |  260 | File / directory path                     |
| `Comment`            | `ProComment`           |  256 | Comment                                   |
| `Value`              | (`PRO_VALUE_SIZE`)     |  256 | Parameter value (text)                    |
| `FeatRefKey`         | (`PRO_FEATREF_KEY_SIZE`)|  81 | Feature reference key                    |
| `ModelExtension`     | `ProMdlExtension`      |   32 | A model's file extension                  |
| `Macro`              | `ProMacro`             |  256 | Macro (size kept for PTC compat.)         |
| `MdlFileName`        | `ProMdlFileName`       |  216 | Full file name "name.ext.#"               |
| `FileName`           | `ProFileName`          |   40 | Same, generic case                        |
| `FamTabColumnDesc`   | `ProFamtabClmDesc`     |  260 | Family table column description           |
| `FamilyMdlName`      | `ProFamilyMdlName`     |  362 | Family table instance "inst[gen]"         |
| `FamilyName`         | `ProFamilyName`        |   66 | Same, generic case                        |
| `DisplayModelName`   | `ProDisplayModelName`  |  362 | A model's display name                    |
| `ModelTypeCode`      | *(no PTC typedef)*     |    4 | "prt"/"asm"/"drw" — internal building block|
| `Extension`          | *(no PTC typedef)*     |    4 | Generic extension — internal building block|
| `VersionSuffix`      | *(no PTC typedef)*     |    4 | Version suffix — internal building block   |

`ModelTypeCode`/`Extension`/`VersionSuffix` have no standalone PTC
equivalent: `PRO_TYPE_SIZE`, `PRO_EXTENSION_SIZE` and `PRO_VERSION_SIZE`
only ever appear in PTC headers combined inside
`ProMdlFileName`/`ProFileName`. These are utility building blocks of the
wrapper, not the re-exposure of a PTC type.

**Narrow buffers (`char[N]`, `FixedCharString<N>` template)**:

| C++ type          | ProTOOLKIT buffer      | Size | Usage                            |
|--------------------|------------------------|-----:|-----------------------------------|
| `CharName`         | `ProCharName`          |   32 | Char variant of `Name`            |
| `CharPath`         | `ProCharPath`          |  260 | Char variant of `Path`            |
| `CharLine`         | `ProCharLine`          |   81 | Char variant of `Line` (messages) |
| `MenuName`         | `ProMenuName`          |   32 | Menu name                         |
| `MenuFileName`     | `ProMenufileName`      |   32 | Menu file name (.mnu)             |
| `MenuButtonName`   | `ProMenubuttonName`    |   32 | Menu button name                  |

`ModelName` (180) is **not** an alias of `Name` (32): PTC reserves a much
larger size for model names than for other Creo names — confusing them
would silently truncate a model name that is too long for a `Name`.

`MaxAssemLevel` (= 25, `PRO_MAX_ASSEM_LEVEL`) is also exposed, but it is
not a buffer size: it is the maximum number of assembly nesting levels
supported by ProTOOLKIT. `ValueUnused`/`ValueDefault`
(`PRO_VALUE_UNUSED`/`PRO_VALUE_DEFAULT`) and `Boolean`
(`ProBoolean`/`ProBool`) are documented above, see "Comparisons, `View()`
and interoperability" and "Boolean".

## Contributing

Contributions are welcome. Feel free to open an issue or a pull request.

## License

To be determined.
