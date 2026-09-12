#pragma once

#include <filesystem>
#include <string>

class PropertyUtils final {
  public:
    PropertyUtils() = delete;

    static std::wstring charToWideString(const char* text);
    static std::wstring stringToWideString(const std::string& text);
    static std::string wideStringToString(const std::wstring& text);

    static std::string environmentStr(const char* name);
    static std::filesystem::path environmentPath(const wchar_t* name);

  private:
    // UTF-8 <-> wide (UTF-16) conversions, kept private: the UTF-8
    // assumption on the narrow side is specific to how this class uses
    // them, not a general contract this header wants to expose. Unlike
    // creo::detail::ToUtf8/FromUtf8 (creo/detail/utf8.hpp), which
    // substitute U+FFFD for malformed input coming from a model file,
    // these validate strictly and throw: a malformed environment
    // variable name or value is a configuration error worth surfacing
    // loudly, not silently patching up.
    static std::wstring utf8ToWide(const char* text);
    static std::string wideToUtf8(const std::wstring& text);
};
