# Wrapper C++17 pour ProTOOLKIT (Creo Parametric 10.0).
#
# Compile toujours (avec ou sans le SDK PTC installé, cf.
# include/creo/detail/protoolkit_compat.hpp). Pour lier une vraie session
# Creo 10, renseignez CREO_TOOLKIT_ROOT (et CREO_TOOLKIT_ARCH si besoin) :
# voir cmake/FindProToolkit.cmake et le README pour le détail.

cmake_minimum_required(VERSION 3.16)
project(creo_wrapper LANGUAGES CXX)

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
  message(STATUS "SDK ProTOOLKIT trouvé : ${ProToolkit_INCLUDE_DIRS}")
  target_include_directories(creo_wrapper PUBLIC ${ProToolkit_INCLUDE_DIRS})
  target_link_libraries(creo_wrapper PUBLIC ${ProToolkit_LIBRARIES})
else()
  message(WARNING
    "SDK ProTOOLKIT (Creo 10) introuvable : compilation en mode 'shim' "
    "(types de substitution, non lié à une session Creo réelle). "
    "Définissez CREO_TOOLKIT_ROOT pour pointer vers le dossier protoolkit "
    "de votre installation Creo 10 et bénéficier de la vraie API.")
endif()

option(CREO_WRAPPER_BUILD_EXAMPLES "Compiler les exemples du wrapper" ON)
if(CREO_WRAPPER_BUILD_EXAMPLES)
  add_executable(hello_creo examples/hello_creo.cpp)
  target_link_libraries(hello_creo PRIVATE creo_wrapper)
endif()
