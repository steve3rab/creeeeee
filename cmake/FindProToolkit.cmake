#[[
FindProToolkit.cmake

Module de recherche pour le SDK ProTOOLKIT de Creo Parametric 10.0. Ce SDK
est fourni par PTC avec l'installation de Creo et n'est PAS redistribué
dans ce dépôt (licence propriétaire) : ce module se contente de le
localiser sur la machine qui compile.

Variables d'entrée (à définir avant `find_package(ProToolkit)`, soit en
cache CMake `-DCREO_TOOLKIT_ROOT=...`, soit en variable d'environnement) :

  CREO_TOOLKIT_ROOT
      Racine de l'installation du SDK Creo (ex. sous Windows :
      "C:/Program Files/PTC/Creo 10.0.0.0/Common Files"). Le module cherche
      ensuite les en-têtes sous "<racine>/protoolkit/includes" et la
      bibliothèque sous "<racine>/protoolkit/<arch>/obj".

  CREO_TOOLKIT_ARCH
      Nom du sous-répertoire d'architecture livré par PTC pour la
      bibliothèque ProTOOLKIT (varie selon la plateforme et la version,
      ex. x86e_win64 sous Windows 64 bits). Ce module ne devine
      volontairement pas cette valeur : vérifiez-la dans votre installation
      locale du SDK sous "<racine>/protoolkit/".

Variables de sortie :

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
