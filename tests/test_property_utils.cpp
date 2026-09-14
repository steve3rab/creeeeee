// Unit tests for PropertyUtils (windows/PropertyUtils.hpp). Unlike
// test_scope_guard.cpp, this needs the real Win32 API (this project is
// Windows-only already, see the top-level CMakeLists.txt guard): no
// mock/fake layer here, these run against the real
// GetEnvironmentVariableW/SetEnvironmentVariableW/MultiByteToWideChar/
// WideCharToMultiByte. Plain assert()-based: no test framework
// dependency, matching this project's existing style. Exit code 0 = all
// tests passed.
#include "PropertyUtils.hpp"

#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

void StringToWideRoundTripAscii() {
  std::wstring wide = PropertyUtils::stringToWideString("hello");
  assert(wide == L"hello");
  std::string back = PropertyUtils::wideStringToString(wide);
  assert(back == "hello");
  std::puts("StringToWideRoundTripAscii: OK");
}

void StringToWideRoundTripNonAscii() {
  // "café" in UTF-8: 'c','a','f' + U+00E9 (encoded as 0xC3 0xA9).
  const std::string utf8 = "caf\xC3\xA9";
  std::wstring wide = PropertyUtils::stringToWideString(utf8);
  assert(wide.size() == 4);
  assert(wide[3] == static_cast<wchar_t>(0x00E9));
  std::string back = PropertyUtils::wideStringToString(wide);
  assert(back == utf8);
  std::puts("StringToWideRoundTripNonAscii: OK");
}

void EmptyStringIsEmpty() {
  assert(PropertyUtils::stringToWideString("").empty());
  assert(PropertyUtils::charToWideString(nullptr).empty());
  assert(PropertyUtils::charToWideString("").empty());
  assert(PropertyUtils::wideStringToString(L"").empty());
  std::puts("EmptyStringIsEmpty: OK");
}

void MalformedUtf8Throws() {
  // 0xFF is never valid in any position of a UTF-8 sequence. Built via
  // string literal concatenation so the \xFF hex escape does not
  // greedily consume the following letter as more hex digits.
  const std::string invalid = "valid"
                               "\xFF"
                               "text";
  bool threw = false;
  try {
    (void)PropertyUtils::stringToWideString(invalid);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  assert(threw);
  std::puts("MalformedUtf8Throws: OK");
}

void EnvironmentStrRoundTrip() {
  const wchar_t *name = L"CREO_WRAPPER_TEST_PROPERTYUTILS_STR";
  BOOL set_ok = SetEnvironmentVariableW(name, L"hello world");
  assert(set_ok != 0);

  std::string value =
      PropertyUtils::environmentStr("CREO_WRAPPER_TEST_PROPERTYUTILS_STR");
  assert(value == "hello world");

  SetEnvironmentVariableW(name, nullptr); // unset
  std::puts("EnvironmentStrRoundTrip: OK");
}

void EnvironmentStrMissingThrows() {
  SetEnvironmentVariableW(L"CREO_WRAPPER_TEST_PROPERTYUTILS_MISSING", nullptr);
  bool threw = false;
  try {
    (void)PropertyUtils::environmentStr(
        "CREO_WRAPPER_TEST_PROPERTYUTILS_MISSING");
  } catch (const std::runtime_error &e) {
    threw = true;
    std::string what = e.what();
    assert(what.find("CREO_WRAPPER_TEST_PROPERTYUTILS_MISSING") !=
           std::string::npos);
  }
  assert(threw);
  std::puts("EnvironmentStrMissingThrows: OK");
}

void EnvironmentStrEmptyNameThrows() {
  // readEnvironment() rejects an empty name with std::invalid_argument,
  // but environmentStr() wraps every failure from it into a
  // std::runtime_error (see its own try/catch) -- the empty-name case is
  // not special-cased differently from "missing".
  bool threw = false;
  try {
    (void)PropertyUtils::environmentStr("");
  } catch (const std::runtime_error &) {
    threw = true;
  }
  assert(threw);
  std::puts("EnvironmentStrEmptyNameThrows: OK");
}

void EnvironmentPathRoundTrip() {
  const wchar_t *name = L"CREO_WRAPPER_TEST_PROPERTYUTILS_PATH";
  BOOL set_ok = SetEnvironmentVariableW(name, L"C:\\Users\\test\\Creo");
  assert(set_ok != 0);

  std::filesystem::path path = PropertyUtils::environmentPath(name);
  assert(path == std::filesystem::path(L"C:\\Users\\test\\Creo"));

  SetEnvironmentVariableW(name, nullptr); // unset
  std::puts("EnvironmentPathRoundTrip: OK");
}

void EnvironmentPathMissingThrows() {
  const wchar_t *name = L"CREO_WRAPPER_TEST_PROPERTYUTILS_PATH_MISSING";
  SetEnvironmentVariableW(name, nullptr);
  bool threw = false;
  try {
    (void)PropertyUtils::environmentPath(name);
  } catch (const std::runtime_error &e) {
    threw = true;
    std::string what = e.what();
    assert(what.find("CREO_WRAPPER_TEST_PROPERTYUTILS_PATH_MISSING") !=
           std::string::npos);
  }
  assert(threw);
  std::puts("EnvironmentPathMissingThrows: OK");
}

} // namespace

int main() {
  StringToWideRoundTripAscii();
  StringToWideRoundTripNonAscii();
  EmptyStringIsEmpty();
  MalformedUtf8Throws();
  EnvironmentStrRoundTrip();
  EnvironmentStrMissingThrows();
  EnvironmentStrEmptyNameThrows();
  EnvironmentPathRoundTrip();
  EnvironmentPathMissingThrows();
  std::puts("OK - all PropertyUtils tests passed");
  return 0;
}
