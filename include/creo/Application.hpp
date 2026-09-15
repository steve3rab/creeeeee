#pragma once
#include "creo/Error.hpp"

#include <cstddef>
#include <exception>
#include <string_view>
#include <vector>

// -----------------------------------------------------------------------
// Wrapper for PTC's two mandatory application entry points (the exact
// contract pasted by the user, matching PTC's own well-known template):
//
//   extern "C" int user_initialize(int argc, char *argv[], char *version,
//                                   char *build, wchar_t errbuf[80]);
//   extern "C" void user_terminate();
//
// user_initialize is called once by Creo while loading the application:
// returning anything other than 0/PRO_TK_NO_ERROR aborts the load, and
// errbuf (a fixed 80-wide-char buffer) is the only channel left to report
// why. user_terminate is called once on unload and returns void, with no
// channel left to report anything at all. Both are literal entry points
// Creo calls directly into: this is the single most important place in
// the whole wrapper for the "never let a C++ exception cross into a
// PTC/C call stack" rule, since there is no caller-side try/catch on the
// other side of the C linkage boundary to save you.
//
// RunUserInitialize()/RunUserTerminate() below exist so the two `extern
// "C"` functions never contain any logic themselves besides the call
// into these helpers, which do the actual catching.
// -----------------------------------------------------------------------

namespace creo {

// Parsed, safer view of user_initialize's raw argc/argv/version/build:
// built once by RunUserInitialize() and handed to the caller's init
// function, so application code never has to touch the raw C parameters
// itself. `args` mirrors argv[0..argc) verbatim (including argv[0], the
// program name, if Creo supplies one) — nothing is filtered or skipped.
struct InitializeArgs {
  std::vector<std::string_view> args;
  std::string_view version;
  std::string_view build;
};

// PRO_TK_APP_INIT_FAIL ("application failed to initialize") from the
// official ProError/ProErr enum (see src/Error.cpp's ToString table for
// the confirmed numeric value): the fallback failure code returned by
// RunUserInitialize() whenever the caught exception carries no more
// specific ProTOOLKIT error code of its own -- a better fit than the
// generic PRO_TK_GENERAL_ERROR for "user_initialize's own body failed",
// since PTC reserves this exact code for that situation.
inline constexpr ErrorCode kAppInitFailCode = static_cast<ErrorCode>(-95);

namespace detail {

// Size of PTC's own errbuf[80] out-parameter, named here instead of
// repeating the literal 80 at every call site.
inline constexpr std::size_t kErrbufSize = 80;

// Writes `message` into errbuf (buffer_size wchar_t's long), truncating
// if needed and always null-terminating within bounds. Widens each byte
// of `message` as its own code point rather than decoding it as UTF-8:
// deliberately not the codec Text.hpp/PropertyUtils.hpp use (Windows-only
// and throws on malformed input), because this runs inside a noexcept
// boundary function where throwing is not an option, and the messages it
// widens here are always this wrapper's own exception text
// (std::exception::what()), which is plain ASCII. Never fails: safe to
// call with a null buffer or a zero size (a no-op in that case).
inline void WriteErrbuf(wchar_t *errbuf, std::size_t buffer_size,
                         std::string_view message) noexcept {
  if (errbuf == nullptr || buffer_size == 0) {
    return;
  }
  std::size_t n = message.size();
  if (n > buffer_size - 1) {
    n = buffer_size - 1;
  }
  for (std::size_t i = 0; i < n; ++i) {
    errbuf[i] = static_cast<wchar_t>(static_cast<unsigned char>(message[i]));
  }
  errbuf[n] = L'\0';
}

} // namespace detail

// Adapts user_initialize's mandatory C contract to ordinary, possibly-
// throwing C++ code: parses argc/argv/version/build into an
// InitializeArgs, calls init(args), and turns any exception into a
// ProError plus a diagnostic in errbuf instead of letting it propagate
// back into Creo's C call stack. Returns 0/PRO_TK_NO_ERROR on success.
//
// A thrown creo::ProToolkitError contributes its own code() (falling
// back to kAppInitFailCode if that code is 0 -- PRO_TK_NO_ERROR is not a
// valid failure code to report back to Creo); any other std::exception,
// or a value of an unknown type, is reported as kAppInitFailCode.
//
// Usage:
//
//   extern "C" int user_initialize(int argc, char *argv[], char *version,
//                                   char *build, wchar_t errbuf[80]) {
//     return creo::RunUserInitialize(
//         argc, argv, version, build, errbuf,
//         [](const creo::InitializeArgs &args) {
//           // your real init code, may throw
//         });
//   }
//
template <typename InitFn>
int RunUserInitialize(int argc, char *argv[], char *version, char *build,
                       wchar_t errbuf[detail::kErrbufSize],
                       InitFn &&init) noexcept {
  InitializeArgs args;
  if (argv != nullptr) {
    for (int i = 0; i < argc; ++i) {
      if (argv[i] != nullptr) {
        args.args.emplace_back(argv[i]);
      }
    }
  }
  if (version != nullptr) {
    args.version = version;
  }
  if (build != nullptr) {
    args.build = build;
  }

  try {
    init(args);
    return static_cast<int>(detail::kNoError);
  } catch (const ProToolkitError &e) {
    detail::WriteErrbuf(errbuf, detail::kErrbufSize, e.what());
    ErrorCode code = e.code();
    return code != detail::kNoError ? static_cast<int>(code)
                                     : static_cast<int>(kAppInitFailCode);
  } catch (const std::exception &e) {
    detail::WriteErrbuf(errbuf, detail::kErrbufSize, e.what());
    return static_cast<int>(kAppInitFailCode);
  } catch (...) {
    detail::WriteErrbuf(errbuf, detail::kErrbufSize,
                         "unknown exception in user_initialize");
    return static_cast<int>(kAppInitFailCode);
  }
}

// Adapts user_terminate's mandatory C contract (a void function with no
// channel at all to report failure) to ordinary, possibly-throwing C++
// code: calls terminate() and silently swallows any exception, since
// there is nothing left to do with one at this point besides let it
// escape into Creo's C call stack, which the wrapper never allows.
//
// Usage:
//
//   extern "C" void user_terminate() {
//     creo::RunUserTerminate([] {
//       // your real cleanup code, may throw
//     });
//   }
//
template <typename TerminateFn>
void RunUserTerminate(TerminateFn &&terminate) noexcept {
  try {
    terminate();
  } catch (...) {
    // Nothing to report: user_terminate() returns void.
  }
}

} // namespace creo
