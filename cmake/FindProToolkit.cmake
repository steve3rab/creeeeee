#[[
FindProToolkit.cmake

Find module for the Creo Parametric 10.0 ProTOOLKIT SDK. This SDK is
shipped by PTC with the Creo installation and is NOT redistributed in
this repository (proprietary license): this module only locates it on
the machine doing the build.

Input variables (set before `find_package(ProToolkit)`, either as a
CMake cache variable `-DCREO_TOOLKIT_ROOT=...`, or as an environment
variable):

  CREO_TOOLKIT_ROOT
      Root of the Creo SDK installation (e.g. on Windows:
      "C:/Program Files/PTC/Creo 10.0.0.0/Common Files"). The module then
      looks for the headers under "<root>/protoolkit/includes" and the
      library under "<root>/protoolkit/<arch>/obj".

  CREO_TOOLKIT_ARCH
      Name of the architecture subdirectory shipped by PTC for the
      ProTOOLKIT library (varies by platform and version, e.g.
      x86e_win64 on 64-bit Windows). This module deliberately does not
      guess this value: check it against your local SDK installation
      under "<root>/protoolkit/".

Output variables:

  ProToolkit_FOUND
  ProToolkit_INCLUDE_DIRS
  ProToolkit_LIBRARIES
]]

if(NOT CREO_TOOLKIT_ROOT AND DEFINED ENV{CREO_TOOLKIT_ROOT})
  set(CREO_TOOLKIT_ROOT "$ENV{CREO_TOOLKIT_ROOT}")
endif()

if(NOT CREO_TOOLKIT_ARCH AND DEFINED ENV{CREO_TOOLKIT_ARCH})
  set(CREO_TOOLKIT_ARCH "$ENV{CREO_TOOLKIT_ARCH}")
endif()

find_path(ProToolkit_INCLUDE_DIR
  NAMES ProToolkit.h
  PATHS "${CREO_TOOLKIT_ROOT}/protoolkit/includes"
)

find_library(ProToolkit_LIBRARY
  NAMES protoolkit libprotoolkit
  PATHS "${CREO_TOOLKIT_ROOT}/protoolkit/${CREO_TOOLKIT_ARCH}/obj"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ProToolkit
  REQUIRED_VARS ProToolkit_INCLUDE_DIR ProToolkit_LIBRARY
)

if(ProToolkit_FOUND)
  set(ProToolkit_INCLUDE_DIRS "${ProToolkit_INCLUDE_DIR}")
  set(ProToolkit_LIBRARIES "${ProToolkit_LIBRARY}")
endif()

mark_as_advanced(ProToolkit_INCLUDE_DIR ProToolkit_LIBRARY)
