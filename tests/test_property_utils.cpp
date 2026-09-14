// Unit tests for PropertyUtils (windows/PropertyUtils.hpp), using
// doctest (vendored: tests/doctest.h, MIT license, see its own header
// for the full notice). Each TEST_CASE below is its own independently
// reported test -- doctest supplies main() itself
// (DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN), nothing hand-rolled here.
//
// Windows-only like the library itself (only registered with CTest
// under WIN32, see CMakeLists.txt): no mock/fake layer, these run
// against the real GetEnvironmentVariableW/SetEnvironmentVariableW/
// MultiByteToWideChar/WideCharToMultiByte.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "PropertyUtils.hpp"

#include <stdexcept>
#include <string>

TEST_CASE("stringToWideString/wideStringToString round-trip ASCII") {
  std::wstring wide = PropertyUtils::stringToWideString("hello");
  CHECK(wide == L"hello");
  CHECK(PropertyUtils::wideStringToString(wide) == "hello");
}

TEST_CASE("stringToWideString/wideStringToString round-trip non-ASCII") {
  // "café" in UTF-8: 'c','a','f' + U+00E9 (encoded as 0xC3 0xA9).
  const std::string utf8 = "caf\xC3\xA9";
  std::wstring wide = PropertyUtils::stringToWideString(utf8);
  REQUIRE(wide.size() == 4);
  CHECK(wide[3] == static_cast<wchar_t>(0x00E9));
  CHECK(PropertyUtils::wideStringToString(wide) == utf8);
}

TEST_CASE("empty strings stay empty") {
  CHECK(PropertyUtils::stringToWideString("").empty());
  CHECK(PropertyUtils::charToWideString(nullptr).empty());
  CHECK(PropertyUtils::charToWideString("").empty());
  CHECK(PropertyUtils::wideStringToString(L"").empty());
}

TEST_CASE("malformed UTF-8 throws std::invalid_argument") {
  // 0xFF is never valid in any position of a UTF-8 sequence. Built via
  // string literal concatenation so the \xFF hex escape does not
  // greedily consume the following letter as more hex digits.
  const std::string invalid = "valid"
                               "\xFF"
                               "text";
  CHECK_THROWS_AS(PropertyUtils::stringToWideString(invalid),
                  std::invalid_argument);
}

TEST_CASE("environmentStr() round-trips through the real environment") {
  const wchar_t *name = L"CREO_WRAPPER_TEST_PROPERTYUTILS_STR";
  REQUIRE(SetEnvironmentVariableW(name, L"hello world") != 0);
  CHECK(PropertyUtils::environmentStr("CREO_WRAPPER_TEST_PROPERTYUTILS_STR") ==
        "hello world");
  SetEnvironmentVariableW(name, nullptr); // unset
}

TEST_CASE("environmentStr() throws with the variable name when missing") {
  SetEnvironmentVariableW(L"CREO_WRAPPER_TEST_PROPERTYUTILS_MISSING", nullptr);
  try {
    (void)PropertyUtils::environmentStr(
        "CREO_WRAPPER_TEST_PROPERTYUTILS_MISSING");
    FAIL("expected std::runtime_error");
  } catch (const std::runtime_error &e) {
    CHECK(std::string(e.what()).find(
              "CREO_WRAPPER_TEST_PROPERTYUTILS_MISSING") != std::string::npos);
  }
}

TEST_CASE("environmentStr() throws std::runtime_error on an empty name") {
  // readEnvironment() rejects an empty name with std::invalid_argument,
  // but environmentStr() wraps every failure from it into a
  // std::runtime_error (see its own try/catch) -- the empty-name case is
  // not special-cased differently from "missing".
  CHECK_THROWS_AS(PropertyUtils::environmentStr(""), std::runtime_error);
}

TEST_CASE("environmentPath() round-trips through the real environment") {
  const wchar_t *name = L"CREO_WRAPPER_TEST_PROPERTYUTILS_PATH";
  REQUIRE(SetEnvironmentVariableW(name, L"C:\\Users\\test\\Creo") != 0);
  CHECK(PropertyUtils::environmentPath(name) ==
        std::filesystem::path(L"C:\\Users\\test\\Creo"));
  SetEnvironmentVariableW(name, nullptr); // unset
}

TEST_CASE("environmentPath() throws with the variable name when missing") {
  const wchar_t *name = L"CREO_WRAPPER_TEST_PROPERTYUTILS_PATH_MISSING";
  SetEnvironmentVariableW(name, nullptr);
  try {
    (void)PropertyUtils::environmentPath(name);
    FAIL("expected std::runtime_error");
  } catch (const std::runtime_error &e) {
    CHECK(std::string(e.what()).find(
              "CREO_WRAPPER_TEST_PROPERTYUTILS_PATH_MISSING") !=
          std::string::npos);
  }
}
