// Unit tests for creo::RunUserInitialize/RunUserTerminate/InitializeArgs
// (creo/Application.hpp), using doctest (vendored: tests/doctest.h, MIT
// license). Each TEST_CASE below is its own independently reported test
// -- doctest supplies main() itself (DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN),
// nothing hand-rolled here.
//
// Like test_error.cpp, this needs no live Creo session and no real SDK
// at all: RunUserInitialize()/RunUserTerminate() only call the supplied
// init/terminate functor and catch whatever it throws -- no ProTOOLKIT
// call happens inside either of them. Builds and runs identically in
// shim mode (no SDK on the include path, the default here) and in
// real-SDK mode.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "creo/Application.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

// Converts an errbuf written by creo::detail::WriteErrbuf() back to a
// narrow string, undoing the byte-per-codepoint widening (the reverse of
// what WriteErrbuf itself does) so tests can compare it like plain text.
std::string NarrowErrbuf(const wchar_t *errbuf) {
  std::string result;
  for (std::size_t i = 0; errbuf[i] != L'\0'; ++i) {
    result += static_cast<char>(errbuf[i]);
  }
  return result;
}

// Builds a `char*[]` view over a fixed set of C-string literals, mimicking
// the raw argv Creo passes to user_initialize.
struct FakeArgv {
  char a0[8] = "creo";
  char a1[16] = "-batch";
  char *argv[2] = {a0, a1};
  int argc = 2;
};

} // namespace

// Mirrors the documented real-world usage pattern verbatim (see
// Application.hpp's file-level comment and the two functions'
// docstrings): CREO_APP_EXPORT rather than a bare extern "C", so this
// also confirms the macro itself compiles to a well-formed function
// definition, not just that RunUserInitialize/RunUserTerminate work when
// called directly. `g_last_args_seen`/`g_terminate_called` are only
// observable from within this translation unit; nothing beyond compiling
// and calling these two functions is meaningful outside a real Creo
// process (there's no way to check they're actually exported from a DLL
// without one).
namespace {
int g_last_argc_seen = -1;
bool g_terminate_called = false;
} // namespace

CREO_APP_EXPORT int user_initialize(int argc, char *argv[], char *version,
                                     char *build, wchar_t errbuf[80]) {
  return creo::RunUserInitialize(
      argc, argv, version, build, errbuf,
      [](const creo::InitializeArgs &args) {
        g_last_argc_seen = static_cast<int>(args.args.size());
      });
}

CREO_APP_EXPORT void user_terminate() {
  creo::RunUserTerminate([] { g_terminate_called = true; });
}

TEST_CASE("The documented CREO_APP_EXPORT usage pattern compiles and "
          "works end to end") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;

  int result = user_initialize(fake.argc, fake.argv,
                                const_cast<char *>("J1"),
                                const_cast<char *>("1"), errbuf);
  CHECK(result == 0);
  CHECK(g_last_argc_seen == 2);

  user_terminate();
  CHECK(g_terminate_called);
}

TEST_CASE("RunUserInitialize returns 0 and leaves errbuf untouched on "
          "success") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  errbuf[0] = L'X';
  FakeArgv fake;
  bool called = false;

  int result = creo::RunUserInitialize(
      fake.argc, fake.argv, const_cast<char *>("J1"), const_cast<char *>("1"),
      errbuf, [&](const creo::InitializeArgs &) { called = true; });

  CHECK(result == 0);
  CHECK(called);
  CHECK(errbuf[0] == L'X');
}

TEST_CASE("RunUserInitialize parses argc/argv/version/build into "
          "InitializeArgs") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;
  creo::InitializeArgs seen;

  creo::RunUserInitialize(fake.argc, fake.argv, const_cast<char *>("J1"),
                           const_cast<char *>("42"), errbuf,
                           [&](const creo::InitializeArgs &args) {
                             seen = args;
                           });

  REQUIRE(seen.args.size() == 2);
  CHECK(seen.args[0] == "creo");
  CHECK(seen.args[1] == "-batch");
  CHECK(seen.version == "J1");
  CHECK(seen.build == "42");
}

TEST_CASE("RunUserInitialize tolerates null argv/version/build") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  bool called = false;

  int result = creo::RunUserInitialize(
      0, nullptr, nullptr, nullptr, errbuf,
      [&](const creo::InitializeArgs &args) {
        called = true;
        CHECK(args.args.empty());
        CHECK(args.version.empty());
        CHECK(args.build.empty());
      });

  CHECK(result == 0);
  CHECK(called);
}

TEST_CASE("RunUserInitialize skips null entries inside a non-null argv") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  char a0[] = "creo";
  char *argv[2] = {a0, nullptr};
  creo::InitializeArgs seen;

  creo::RunUserInitialize(2, argv, nullptr, nullptr, errbuf,
                           [&](const creo::InitializeArgs &args) {
                             seen = args;
                           });

  REQUIRE(seen.args.size() == 1);
  CHECK(seen.args[0] == "creo");
}

TEST_CASE("RunUserInitialize reports a thrown ProToolkitError's own code "
          "and message in errbuf") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;
  creo::ErrorCode failing = static_cast<creo::ErrorCode>(-2); // BAD_INPUTS

  int result = creo::RunUserInitialize(
      fake.argc, fake.argv, nullptr, nullptr, errbuf,
      [&](const creo::InitializeArgs &) {
        throw creo::ProToolkitError(failing, "some_context");
      });

  CHECK(result == -2);
  std::string message = NarrowErrbuf(errbuf);
  CHECK(message.find("PRO_TK_BAD_INPUTS") != std::string::npos);
  CHECK(message.find("some_context") != std::string::npos);
}

TEST_CASE("RunUserInitialize falls back to kAppInitFailCode for a "
          "ProToolkitError carrying code 0") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;

  int result = creo::RunUserInitialize(
      fake.argc, fake.argv, nullptr, nullptr, errbuf,
      [&](const creo::InitializeArgs &) {
        throw creo::ProToolkitError(static_cast<creo::ErrorCode>(0), "");
      });

  CHECK(result == static_cast<int>(creo::kAppInitFailCode));
}

TEST_CASE("RunUserInitialize reports a generic std::exception as "
          "kAppInitFailCode with its what() in errbuf") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;

  int result = creo::RunUserInitialize(
      fake.argc, fake.argv, nullptr, nullptr, errbuf,
      [&](const creo::InitializeArgs &) {
        throw std::runtime_error("bad config file");
      });

  CHECK(result == static_cast<int>(creo::kAppInitFailCode));
  CHECK(NarrowErrbuf(errbuf).find("bad config file") != std::string::npos);
}

TEST_CASE("RunUserInitialize reports a non-std::exception value as "
          "kAppInitFailCode") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;

  int result = creo::RunUserInitialize(
      fake.argc, fake.argv, nullptr, nullptr, errbuf,
      [&](const creo::InitializeArgs &) { throw 42; });

  CHECK(result == static_cast<int>(creo::kAppInitFailCode));
  CHECK(NarrowErrbuf(errbuf).find("unknown exception") != std::string::npos);
}

TEST_CASE("RunUserInitialize truncates a message longer than errbuf "
          "without overrunning it") {
  wchar_t errbuf[creo::detail::kErrbufSize];
  FakeArgv fake;
  std::string huge(500, 'z');

  creo::RunUserInitialize(fake.argc, fake.argv, nullptr, nullptr, errbuf,
                           [&](const creo::InitializeArgs &) {
                             throw std::runtime_error(huge);
                           });

  std::string message = NarrowErrbuf(errbuf);
  CHECK(message.size() == creo::detail::kErrbufSize - 1);
  CHECK(errbuf[creo::detail::kErrbufSize - 1] == L'\0');
}

TEST_CASE("RunUserInitialize tolerates a null errbuf") {
  FakeArgv fake;
  CHECK_NOTHROW(creo::RunUserInitialize(
      fake.argc, fake.argv, nullptr, nullptr, nullptr,
      [&](const creo::InitializeArgs &) { throw std::runtime_error("x"); }));
}

TEST_CASE("RunUserTerminate calls terminate() on the normal path") {
  bool called = false;
  creo::RunUserTerminate([&] { called = true; });
  CHECK(called);
}

TEST_CASE("RunUserTerminate silently swallows an exception") {
  CHECK_NOTHROW(creo::RunUserTerminate([] {
    throw std::runtime_error("cleanup failed");
  }));
}
