#pragma once
#include "Error.hpp"

#include <cstddef>
#include <exception>
#include <string_view>
#include <vector>

namespace creo {

struct InitializeArgs {
  std::vector<std::string_view> args;
  std::string_view version;
  std::string_view build;
};

inline constexpr ErrorCode kAppInitFailCode = static_cast<ErrorCode>(-95);

namespace detail {

inline constexpr std::size_t kErrbufSize = 80;

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

template <typename TerminateFn>
void RunUserTerminate(TerminateFn &&terminate) noexcept {
  try {
    terminate();
  } catch (...) {
  }
}

} // namespace creo
