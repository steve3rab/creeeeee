// Example usage of the C++17 wrapper for ProTOOLKIT.
//
// Two parts, meant to be read in this order:
//
//  1) DemonstrateTypes(): a tour of the wrapper's types (Name, Line, Path,
//     CharName, error handling) and their conversions. Does not depend
//     on any real ProTOOLKIT function: runs identically in real-SDK mode
//     and in shim mode, so you can run this executable directly
//     (`./hello_creo`) even without Creo installed.
//
//  2) DemonstrateArray(): a tour of creo::Array<T> (RAII around
//     ProArray). Like DemonstrateTypes(), works identically in real-SDK
//     mode and in shim mode (the shim genuinely reimplements a
//     ProArray's behavior, not just its shape).
//
//  3) PrintCurrentModelName(): retrieves the active model's name and type
//     via ModelHandle::GetCurrent()/Name()/Type() (themselves wrapping
//     ProMdlCurrentGet/ProMdlMdlnameGet/ProMdlTypeGet), all inside a
//     single try/catch — illustrates how CREO_CHECK short-circuits the
//     rest of the block on the first error. Requires the real SDK
//     (CREO_TOOLKIT_ROOT, see README) to do anything useful; otherwise
//     main() prints an explanatory message.
//
//  4) PrintSessionModelInfo(): tours the ProAssembly.h-declared session
//     functions — ModelHandle::GetActive(), Extension(), DirectoryPath(),
//     WindowId(), Display(), and the static ModelHandle::List() — each
//     wrapping its own ProTOOLKIT call (ProMdlActiveGet,
//     ProMdlExtensionGet, ProMdlDirectoryPathGet, ProMdlWindowGet,
//     ProMdlDisplay, ProSessionMdlList respectively), plus
//     TryGetCurrent()/TryGetActive() (the std::optional-returning
//     alternative to GetCurrent()/GetActive() for callers who do not
//     consider "nothing current/active" exceptional). Same real-SDK-only
//     caveat as PrintCurrentModelName().
//
//  5) DemonstrateScopeGuard(): creo::Defer()/ScopeGuard, a generic RAII
//     "run this on scope exit" utility — not specific to any ProTOOLKIT
//     function, so (like DemonstrateTypes()/DemonstrateArray()) it runs
//     identically in shim mode and in real-SDK mode.
//
//  6) PrintFeatures(): tours creo::VisitFeatures(), a lambda-friendly
//     wrapper around ProSolidFeatVisit (PTC's "visit function" pattern
//     for a solid's features) — pass ordinary capturing lambdas as the
//     action/filter instead of routing through a raw C function pointer
//     and a void* ProAppData by hand — plus CollectFeatures()/
//     FindFeatureByName(), convenience wrappers built on top of it (no
//     ProTOOLKIT call of their own), mirroring PTC's own
//     ProUtilCollectSolidFeatures()/ProUtilFindFeatureByName() sample
//     utilities. Requires the real SDK (an actual solid to visit) to do
//     anything useful; otherwise it just shows the "no current model"
//     path.
//
//  7) PrintExpldStates(): one representative of the four other
//     modelitem-shaped visit functions confirmed alongside
//     ProSolidFeatVisit (ProSolidExpldstateVisit here; also
//     VisitNotes()/VisitProcSteps()/VisitSimpReps()/VisitGeomitems(), not
//     each demonstrated to keep this tour short) — see ModelItem.hpp.
//     Not shown here at all: creo::VisitOpaque()/GeometryHandle<T>
//     (creo/Geometry.hpp), the by-value-handle counterpart covering
//     Csys/Axis/Quilt/Surface/Contour/Edge — it needs the real PTC type
//     names from your own project's headers, which this example does not
//     have (see Geometry.hpp's own comment for why).
//
// Technical note: this file only uses std::printf/std::puts (never
// std::wprintf) for output. Mixing "wide" and "narrow" calls on the same
// stdout stream is undefined behavior in C/C++ (the stream locks onto
// the first orientation used): creo::ToString() is enough to print
// everything, including the content of a wide buffer like ProName.

#include "creo/Array.hpp"
#include "creo/Error.hpp"
#include "creo/ScopeGuard.hpp"
#include "creo/Types.hpp"

#include <cstdio>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>

namespace {

void DemonstrateTypes() {
  std::puts("--- Text types: Name / Line / Path ---");

  // Construction from a wide literal (L"...") or directly from a UTF-8
  // std::string: both constructors exist on every type.
  creo::Name part_name(L"gear_01");
  creo::Line message(std::string("part checked: OK"));
  creo::Path file_path("/home/user/creo/project/gear_01.prt");

  std::printf("Name: %s (wide length = %zu)\n", part_name.ToString().c_str(),
              part_name.ToWString().size());
  std::printf("Line: %s\n", message.ToString().c_str());
  std::printf("Path: %s\n", file_path.ToString().c_str());

  // Capacity (PRO_NAME_SIZE = 32 for Name) is checked: no silent
  // truncation like in C, an exception is thrown instead.
  try {
    creo::Name too_long(std::wstring(50, L'x'));
  } catch (const std::length_error &e) {
    std::printf("Capacity exceeded (expected): %s\n", e.what());
  }

  // Char variant (FixedCharString), for ProTOOLKIT buffers that are not
  // wide (ProCharName, ProMenuName, ProMenubuttonName, ...).
  creo::CharName menu_entry("EDIT_FEATURE");
  std::printf("CharName: %s\n", menu_entry.ToString().c_str());

  std::puts("\n--- Comparisons, View() and std::filesystem interop ---");

  // Content-based comparison, including against a wide literal: see the
  // README ("Comparisons, View() and interoperability") for the overload
  // resolution ambiguity these operators avoid.
  creo::Name same_part(L"gear_01");
  std::printf("part_name == same_part: %s\n",
              part_name == same_part ? "yes" : "no");
  std::printf("part_name == L\"gear_01\": %s\n",
              part_name == L"gear_01" ? "yes" : "no");

  // View(): copy-free access to the content (unlike ToWString(), which
  // allocates a new std::wstring on every call). ends_with() is C++20:
  // this wrapper targets C++17, so we compare via substr/compare.
  std::wstring_view view = file_path.View();
  constexpr std::wstring_view kExt = L".prt";
  bool ends_with_prt =
      view.size() >= kExt.size() &&
      view.compare(view.size() - kExt.size(), kExt.size(), kExt) == 0;
  std::printf("file_path ends with .prt: %s\n",
              ends_with_prt ? "yes" : "no");

  // Path <-> std::filesystem::path.
  std::filesystem::path fs_path = creo::ToFilesystemPath(file_path);
  creo::Path round_trip = creo::PathFromFilesystem(fs_path / ".." / "other.prt");
  std::printf("Round-trip filesystem::path: %s\n",
              round_trip.ToString().c_str());

  std::puts("\n--- Boolean (ProBoolean/ProBool) ---");

  // ProBoolean is a type distinct from C++ bool on the ProTOOLKIT side
  // (even though its two values, PRO_B_FALSE/PRO_B_TRUE, numerically
  // coincide with false/true): ToBool()/ToProBoolean() avoid writing the
  // conversion by hand on every call to a function that expects one.
  creo::Boolean flag = creo::ToProBoolean(true);
  std::printf("ToBool(flag): %s\n", creo::ToBool(flag) ? "true" : "false");
  std::printf("ValueUnused = %d, ValueDefault = %d\n", creo::ValueUnused,
              creo::ValueDefault);

  std::puts("\n--- ModelItem (ProGeomitem/ProFeature/ProDimension/...) ---");

  // ProGeomitem, ProFeature, ProDimension, ProNote, ProLayer, ... are all,
  // structurally, the exact same 3-field struct {type, id, owner} — PTC
  // only distinguishes them by typedef name. creo::ModelItem wraps that
  // struct once; GeomItem/Dimension/... are plain aliases of it, same as
  // the C side. Type()/Id()/Owner() are plain field reads, so this works
  // without a real Creo session (unlike GetName(), which needs one).
  creo::detail::RawModelItem raw_item{};
  raw_item.type = creo::ObjectType::PRO_FEATURE;
  raw_item.id = 42;
  creo::Feature feature(raw_item);
  std::printf("Feature: type = %d, id = %d, owner is valid = %s\n",
              static_cast<int>(feature.Type()), feature.Id(),
              feature.Owner().IsValid() ? "yes" : "no");

  // Unlike the other aliases, Feature is a real derived class of
  // ModelItem, not a bare alias: it has one feature-specific method,
  // Regenerate() (ProFeatureRegenerate), which would not make sense on a
  // Layer or a Note. It still needs a real session to do anything useful.
  try {
    feature.Regenerate(feature.Owner());
  } catch (const creo::ProToolkitError &e) {
    std::printf("Feature::Regenerate() failed (expected, no session): %s\n",
                e.what());
  }

  std::puts("\n--- Error handling: CREO_CHECK / ProToolkitError ---");

  // Simulates a ProTOOLKIT call that would fail with PRO_TK_BAD_INPUTS
  // (-2), without needing a real call: shows only the ProError ->
  // exception conversion mechanism (works identically for a real call,
  // e.g.: CREO_CHECK(ProMdlMdlnameGet(model.Raw(), name.Raw()));).
  try {
    CREO_CHECK(static_cast<creo::ErrorCode>(-2));
  } catch (const creo::ProToolkitError &e) {
    std::printf("Caught error: %s (raw code = %d)\n", e.what(),
                static_cast<int>(e.code()));
  }
}

void DemonstrateArray() {
  std::puts("\n--- creo::Array<T> (RAII around ProArray) ---");

  creo::Array<int> values(0, 4); // empty, grows in blocks of 4
  for (int i = 1; i <= 5; ++i) {
    values.Append(i * 10);
  }
  std::printf("Size after 5 Append: %d\n", values.Size());

  values.Insert(1, 999); // shifts the rest
  std::printf("After Insert(1, 999): ");
  for (int v : values) {
    std::printf("%d ", v);
  }
  std::putchar('\n');

  values.Remove(1); // removes the element just inserted
  std::printf("After Remove(1): ");
  for (int v : values) {
    std::printf("%d ", v);
  }
  std::putchar('\n');

  try {
    values.At(100);
  } catch (const std::out_of_range &e) {
    std::printf("At(100) out of range (expected): %s\n", e.what());
  }
}

void DemonstrateScopeGuard() {
  std::puts("\n--- creo::ScopeGuard / Defer (generic RAII cleanup) ---");

  // Runs on normal scope exit.
  {
    auto guard = creo::Defer([] { std::puts("  cleanup #1 ran (normal exit)"); });
  }

  // Also runs when the scope exits via an exception -- exactly the case
  // hand-written try/catch-and-restore code around a ProTOOLKIT
  // begin/end pair is prone to getting wrong on an early-return path.
  try {
    auto guard = creo::Defer([] { std::puts("  cleanup #2 ran (via exception)"); });
    throw std::runtime_error("simulated failure mid-scope");
  } catch (const std::runtime_error &e) {
    std::printf("  caught: %s\n", e.what());
  }

  // Dismiss(): cancel the pending call once it turns out to be
  // unnecessary (e.g. the risky step it was guarding committed OK).
  {
    auto guard = creo::Defer([] { std::puts("  cleanup #3 (should NOT print)"); });
    guard.Dismiss();
  }
  std::puts("  Dismiss()'d guard produced no output, as expected");
}

#if CREO_WRAPPER_HAS_REAL_SDK
// Retrieves the name of the model currently active in the Creo session.
// Returns false (and prints why) if no model is active or if a
// ProTOOLKIT call fails.
//
// ModelHandle::GetCurrent() wraps ProMdlCurrentGet directly: it throws
// rather than returning null when there is no current model, so unlike
// PrintCurrentModelName()'s previous version there is no separate
// `if (!model)` branch to write — a missing model and a ProMdlMdlnameGet
// failure both land in the same catch below, exactly like a chained
// CREO_CHECK would.
bool PrintCurrentModelName() {
  try {
    creo::ModelHandle model = creo::ModelHandle::GetCurrent();
    std::printf("Active model: %s (type = %d)\n",
                model.Name().ToString().c_str(),
                static_cast<int>(model.Type()));
    return true;

  } catch (const creo::ProToolkitError &e) {
    std::printf("Could not retrieve the active model: %s\n", e.what());

    // e.code() allows differentiated handling if needed, e.g.:
    if (e.code() == static_cast<creo::ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
      std::puts("(no model is currently loaded in Creo)");
    }
    return false;
  }
}

// Tours the six ProAssembly.h wrappers added alongside GetCurrent().
// Each call is independent (one try/catch per call) so that one missing
// piece of session state (e.g. no active model) does not prevent seeing
// the others succeed or fail on their own terms.
void PrintSessionModelInfo() {
  // TryGetCurrent()/TryGetActive(): the std::optional-returning
  // alternative to GetCurrent()/GetActive(), for callers who treat "no
  // current/active model" as a normal case to check for rather than an
  // exceptional one -- no try/catch needed here.
  if (std::optional<creo::ModelHandle> current =
          creo::ModelHandle::TryGetCurrent()) {
    std::printf("TryGetCurrent() -> a model is current: %s\n",
                current->Name().ToString().c_str());
  } else {
    std::puts("TryGetCurrent() -> std::nullopt (no current model)");
  }

  try {
    creo::ModelHandle active = creo::ModelHandle::GetActive();
    std::printf("Active model (ProMdlActiveGet): %s\n",
                active.Name().ToString().c_str());

    std::printf("  Extension: %s\n", active.Extension().ToString().c_str());
    std::printf("  Directory: %s\n", active.DirectoryPath().ToString().c_str());
    std::printf("  Window id: %d\n", active.WindowId());

    active.Display();
    std::puts("  Display() sent to Creo.");

  } catch (const creo::ProToolkitError &e) {
    std::printf("Could not inspect the active model: %s\n", e.what());
  }

  try {
    creo::Array<creo::ModelHandle> parts =
        creo::ModelHandle::List(creo::MdlType::PRO_MDL_PART);
    std::printf("Parts loaded in session (ProSessionMdlList): %d\n",
                parts.Size());
  } catch (const creo::ProToolkitError &e) {
    std::printf("Could not list session models: %s\n", e.what());
  }
}

// Visits every feature of the active model, printing name and id, and
// counts how many are of type PRO_FEATURE (the vast majority in
// practice) via the filter argument. VisitFeatures() is not thrown as a
// ProToolkitError on "nothing found"/an early stop (see its own comment
// in ModelItem.hpp): its return code is inspected directly here instead.
void PrintFeatures() {
  std::optional<creo::ModelHandle> active = creo::ModelHandle::TryGetActive();
  if (!active.has_value()) {
    std::puts("No active model to visit features on.");
    return;
  }

  int considered = 0;
  creo::ErrorCode result = creo::VisitFeatures(
      *active,
      [&](const creo::Feature &feature, creo::ErrorCode /*status*/) {
        ++considered;
        std::printf("  feature id = %d\n", feature.Id());
        return static_cast<creo::ErrorCode>(0); // PRO_TK_NO_ERROR: continue
      },
      [](const creo::Feature &feature) {
        if (feature.Type() != creo::ObjectType::PRO_FEATURE) {
          return static_cast<creo::ErrorCode>(-7); // PRO_TK_CONTINUE: skip
        }
        return static_cast<creo::ErrorCode>(0); // PRO_TK_NO_ERROR: visit it
      });

  if (result == static_cast<creo::ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
    std::puts("No features found on the active model.");
  } else if (result != static_cast<creo::ErrorCode>(0)) {
    std::printf("Visit stopped early with code %d after %d feature(s)\n",
                static_cast<int>(result), considered);
  } else {
    std::printf("Visited %d feature(s)\n", considered);
  }

  // CollectFeatures()/FindFeatureByName(): convenience wrappers built on
  // VisitFeatures(), needing no ProTOOLKIT call of their own.
  std::vector<creo::Feature> features = creo::CollectFeatures(*active);
  std::printf("CollectFeatures() -> %zu feature(s)\n", features.size());

  if (!features.empty()) {
    creo::Name first_name = features.front().GetName();
    if (std::optional<creo::Feature> found =
            creo::FindFeatureByName(*active, first_name.View())) {
      std::printf("FindFeatureByName(%s) -> id = %d\n",
                  first_name.ToString().c_str(), found->Id());
    }
  }
}

// One representative of the four other modelitem-shaped visit functions
// confirmed alongside VisitFeatures() from ProUtilVisit.c (VisitNotes(),
// VisitProcSteps(), VisitSimpReps(), VisitGeomitems() all follow the same
// shape/contract, just not each demonstrated here to keep this tour
// short). Note: there is no VisitOpaque()/GeometryHandle<> demo here —
// unlike the functions above, that family (Csys/Axis/Quilt/Surface/
// Contour/Edge, see creo/Geometry.hpp) needs the *real* PTC type names
// from your own project's headers, which this example does not have.
void PrintExpldStates() {
  std::optional<creo::ModelHandle> active = creo::ModelHandle::TryGetActive();
  if (!active.has_value()) {
    std::puts("No active model to visit exploded states on.");
    return;
  }

  int count = 0;
  creo::ErrorCode result =
      creo::VisitExpldStates(*active, [&](const creo::ExpldState &, creo::ErrorCode) {
        ++count;
        return static_cast<creo::ErrorCode>(0); // PRO_TK_NO_ERROR: continue
      });

  if (result == static_cast<creo::ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
    std::puts("No exploded states found on the active model.");
  } else {
    std::printf("Visited %d exploded state(s)\n", count);
  }
}
#endif

} // namespace

int main() {
  // Safety net: each section already has its own try/catch for expected
  // errors (ProToolkitError, std::out_of_range, ...), but a truly
  // unexpected exception (e.g. std::bad_alloc) should not reach
  // std::terminate() without an actionable message.
  try {
    DemonstrateTypes();
    DemonstrateArray();
    DemonstrateScopeGuard();

    std::puts("\n--- Creo session: active model name ---");
#if CREO_WRAPPER_HAS_REAL_SDK
    PrintCurrentModelName();

    std::puts("\n--- Creo session: ProAssembly.h wrappers ---");
    PrintSessionModelInfo();

    std::puts("\n--- Creo session: VisitFeatures() ---");
    PrintFeatures();

    std::puts("\n--- Creo session: VisitExpldStates() ---");
    PrintExpldStates();
#else
    std::puts(
        "ProTOOLKIT SDK not found: this part was built in 'shim' mode. "
        "Set CREO_TOOLKIT_ROOT (see README) and rebuild to run it in a "
        "real Creo 10 session.");
#endif
  } catch (const std::exception &e) {
    std::fprintf(stderr, "Unexpected error: %s\n", e.what());
    return 1;
  }
  return 0;
}
