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
//  3) PrintCurrentModelName(): retrieves the active model's name via a
//     chain of two ProTOOLKIT calls (ProMdlCurrentGet then
//     ProMdlMdlNameGet), all inside a single try/catch — illustrates how
//     CREO_CHECK short-circuits the rest of the block on the first
//     error. Requires the real SDK (CREO_TOOLKIT_ROOT, see README) to do
//     anything useful; otherwise main() prints an explanatory message.
//
// Technical note: this file only uses std::printf/std::puts (never
// std::wprintf) for output. Mixing "wide" and "narrow" calls on the same
// stdout stream is undefined behavior in C/C++ (the stream locks onto
// the first orientation used): creo::ToString() is enough to print
// everything, including the content of a wide buffer like ProName.

#include "creo/array.hpp"
#include "creo/error.hpp"
#include "creo/types.hpp"

#include <cstdio>
#include <filesystem>
#include <stdexcept>

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

  std::puts("\n--- Error handling: CREO_CHECK / ProToolkitError ---");

  // Simulates a ProTOOLKIT call that would fail with PRO_TK_BAD_INPUTS
  // (-2), without needing a real call: shows only the ProError ->
  // exception conversion mechanism (works identically for a real call,
  // e.g.: CREO_CHECK(ProMdlMdlNameGet(model.Raw(), name.Raw()));).
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

#if CREO_WRAPPER_HAS_REAL_SDK
// Retrieves the name of the model currently active in the Creo session.
// Returns false (and prints why) if no model is active or if a
// ProTOOLKIT call fails.
//
// Shows how CREO_CHECK works on ProMdlCurrentGet (output via pointer,
// hence the `&`), then on ModelHandle::Name(), which wraps both the
// handle validity check and the ProMdlMdlNameGet call (which replaces
// ProMdlNameGet, now deprecated in Creo 10). If ProMdlCurrentGet fails,
// Name() is never reached: the exception jumps straight to the catch,
// with no intermediate `if` to write by hand.
bool PrintCurrentModelName() {
  try {
    creo::detail::RawMdl raw_model = nullptr;
    CREO_CHECK(ProMdlCurrentGet(&raw_model));

    creo::ModelHandle model(raw_model);
    if (!model) {
      std::puts("No active model in the Creo session.");
      return false;
    }

    std::printf("Active model: %s\n", model.Name().ToString().c_str());
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

    std::puts("\n--- Creo session: active model name ---");
#if CREO_WRAPPER_HAS_REAL_SDK
    PrintCurrentModelName();
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
