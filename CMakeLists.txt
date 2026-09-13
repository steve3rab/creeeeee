# C++17 wrapper for ProTOOLKIT (Creo Parametric 10.0).
#
# Always compiles (with or without the PTC SDK installed, see
# include/creo/detail/protoolkit_compat.hpp). To link against a real
# Creo 10 session, set CREO_TOOLKIT_ROOT (and CREO_TOOLKIT_ARCH if
# needed): see cmake/FindProToolkit.cmake and the README for details.
#
# Requires Windows (natively or cross-compiled with MinGW-w64): the
# final target for this project is Windows only. creo::detail::ToUtf8/
# FromUtf8 (include/creo/text.hpp) call the native Win32 API
# (WideCharToMultiByte/MultiByteToWideChar) directly, with no portable
# fallback, so this is no longer a "shim mode covers every OS" situation
# like the ProTOOLKIT SDK check below.

cmake_minimum_required(VERSION 3.16)
project(creo_wrapper LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR
    "This wrapper requires Windows: creo::detail::ToUtf8/FromUtf8 "
    "(include/creo/text.hpp) call the native Win32 API "
    "(WideCharToMultiByte/MultiByteToWideChar) directly, with no portable "
    "fallback. Build on Windows, or cross-compile with a MinGW-w64 "
    "toolchain (CMAKE_TOOLCHAIN_FILE / CMAKE_SYSTEM_NAME=Windows).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ProToolkit)

add_library(creo_wrapper STATIC
  src/error.cpp
)
target_include_directories(creo_wrapper PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(ProToolkit_FOUND)
  message(STATUS "ProTOOLKIT SDK found: ${ProToolkit_INCLUDE_DIRS}")
  target_include_directories(creo_wrapper PUBLIC ${ProToolkit_INCLUDE_DIRS})
  target_link_libraries(creo_wrapper PUBLIC ${ProToolkit_LIBRARIES})
else()
  message(WARNING
    "ProTOOLKIT SDK (Creo 10) not found: building in 'shim' mode "
    "(substitute types, not linked to a real Creo session). "
    "Set CREO_TOOLKIT_ROOT to point to the protoolkit folder of your "
    "Creo 10 installation to use the real API.")
endif()

option(CREO_WRAPPER_BUILD_EXAMPLES "Build the wrapper examples" ON)
if(CREO_WRAPPER_BUILD_EXAMPLES)
  add_executable(hello_creo examples/hello_creo.cpp)
  target_link_libraries(hello_creo PRIVATE creo_wrapper)
endif()

# General-purpose Win32 utility (env var / path helpers), header-only, not
# specific to ProTOOLKIT: does not depend on creo_wrapper, and vice versa.
# Windows-only regardless of creo_wrapper's own portability: it reads
# environment variables via GetEnvironmentVariableW.
if(WIN32)
  add_library(property_utils INTERFACE)
  target_include_directories(property_utils INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/windows"
  )
endif()
