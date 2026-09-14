// Unit tests for creo::ErrorCode/ProToolkitError/ToString()/CREO_CHECK
// (creo/Error.hpp + src/Error.cpp), using doctest (vendored:
// tests/doctest.h, MIT license). Each TEST_CASE below is its own
// independently reported test -- doctest supplies main() itself
// (DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN), nothing hand-rolled here.
//
// Unlike test_property_utils.cpp, this needs no live Creo session and
// no real SDK at all: ToString() is a pure code->label lookup and
// ProToolkitError/CREO_CHECK only build/throw a C++ exception from
// whatever ErrorCode the caller already has -- no ProTOOLKIT call
// happens inside any of them. Builds and runs identically in shim mode
// (no SDK on the include path, the default here) and in real-SDK mode
// (see the README's "Testing/integrating one file at a time" section):
// same behavior either way, since ToString() compares the numeric value
// of `code`, not the enumerator name, and the numeric values are
// identical in both modes.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "creo/Error.hpp"

#include <string>

namespace {
constexpr creo::ErrorCode kNoError = static_cast<creo::ErrorCode>(0);
constexpr creo::ErrorCode kGeneralError = static_cast<creo::ErrorCode>(-1);
constexpr creo::ErrorCode kBadInputs = static_cast<creo::ErrorCode>(-2);
constexpr creo::ErrorCode kContinue = static_cast<creo::ErrorCode>(-7);
constexpr creo::ErrorCode kDllLoadError = static_cast<creo::ErrorCode>(-71);
constexpr creo::ErrorCode kAppJlinkNotAllowed =
    static_cast<creo::ErrorCode>(-100);
// -72..-87 are reserved/unused in the official ProError/ProErr enum (no
// gap for this wrapper to fill): a safe "code this wrapper does not
// know" value, alongside a value wildly outside the whole enum's range.
constexpr creo::ErrorCode kReservedGap = static_cast<creo::ErrorCode>(-80);
constexpr creo::ErrorCode kWildlyOutOfRange =
    static_cast<creo::ErrorCode>(12345);
} // namespace

TEST_CASE("ToString() maps known codes to their PTC label") {
  CHECK(creo::ToString(kNoError) == "PRO_TK_NO_ERROR");
  CHECK(creo::ToString(kGeneralError) == "PRO_TK_GENERAL_ERROR");
  CHECK(creo::ToString(kBadInputs) == "PRO_TK_BAD_INPUTS");
  CHECK(creo::ToString(kContinue) == "PRO_TK_CONTINUE");
  CHECK(creo::ToString(kDllLoadError) == "PRO_TK_DLL_LOAD_ERROR");
  CHECK(creo::ToString(kAppJlinkNotAllowed) == "PRO_TK_APP_JLINK_NOT_ALLOWED");
}

TEST_CASE("ToString() falls back to the raw number for an unknown code") {
  CHECK(creo::ToString(kReservedGap) == "code #-80 (unknown to creo::ToString)");
  CHECK(creo::ToString(kWildlyOutOfRange) ==
        "code #12345 (unknown to creo::ToString)");
}

TEST_CASE("ProToolkitError::code() returns the code it was built with") {
  creo::ProToolkitError error(kBadInputs, "");
  CHECK(error.code() == kBadInputs);
}

TEST_CASE("ProToolkitError's message includes the label and the context") {
  creo::ProToolkitError error(kBadInputs, "ProMdlMdlnameGet(...)");
  std::string what = error.what();
  CHECK(what == "ProTOOLKIT error: PRO_TK_BAD_INPUTS (ProMdlMdlnameGet(...))");
}

TEST_CASE("ProToolkitError's message omits the parens for an empty context") {
  creo::ProToolkitError error(kBadInputs, "");
  std::string what = error.what();
  CHECK(what == "ProTOOLKIT error: PRO_TK_BAD_INPUTS");
  CHECK(what.find('(') == std::string::npos);
}

TEST_CASE("ThrowIfError() does not throw on PRO_TK_NO_ERROR") {
  CHECK_NOTHROW(creo::ThrowIfError(kNoError));
}

TEST_CASE("ThrowIfError() throws ProToolkitError with the given code on "
          "failure") {
  try {
    creo::ThrowIfError(kBadInputs, "some_context");
    FAIL("expected creo::ProToolkitError");
  } catch (const creo::ProToolkitError &e) {
    CHECK(e.code() == kBadInputs);
    std::string what = e.what();
    CHECK(what.find("PRO_TK_BAD_INPUTS") != std::string::npos);
    CHECK(what.find("some_context") != std::string::npos);
  }
}

TEST_CASE("CREO_CHECK() does not throw on success and throws with the "
          "expression text on failure") {
  CHECK_NOTHROW(CREO_CHECK(kNoError));

  try {
    CREO_CHECK(kBadInputs);
    FAIL("expected creo::ProToolkitError");
  } catch (const creo::ProToolkitError &e) {
    CHECK(e.code() == kBadInputs);
    std::string what = e.what();
    CHECK(what.find("PRO_TK_BAD_INPUTS") != std::string::npos);
    CHECK(what.find("kBadInputs") != std::string::npos);
  }
}
