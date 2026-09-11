# srcAcopier

Copie autonome et prête à l'emploi du wrapper (headers `creo/*.hpp` +
`error.cpp`), à glisser directement dans un vrai projet ProTOOLKIT ayant
accès au SDK Creo 10 et à une licence valide — sans avoir besoin du reste
de ce dépôt (CMake, exemples, README principal, etc.).

## Intégration dans votre projet

1. Copiez tout le contenu de ce dossier (`creo/` + `error.cpp`) dans votre
   projet, par exemple sous `third_party/creo_wrapper/`.
2. Ajoutez ce dossier à vos chemins d'inclusion (`-I` / `Include
   Directories`), en plus des chemins d'inclusion habituels de votre SDK
   ProTOOLKIT (`.../protoolkit/includes`).
3. Ajoutez `error.cpp` à la compilation de votre projet (même système de
   build que le reste : Visual Studio, Makefile, CMake, ...).
4. Compilez en C++17 ou plus récent.

Aucune autre configuration n'est nécessaire : les en-têtes détectent
automatiquement la présence du SDK réel via `__has_include(<ProToolkit.h>)`
(voir `creo/detail/protoolkit_compat.hpp`). Dès que votre projet a accès
aux vrais en-têtes PTC sur son chemin d'inclusion — ce qui est déjà le cas
pour tout projet ProTOOLKIT existant —, le wrapper les utilise directement
au lieu du mode « shim » (voir le README principal du dépôt pour le
détail des deux modes).

## Contenu

```
creo/
  types.hpp                    Name, ModelName, Line, Path, ModelHandle, ...
  error.hpp                    ProToolkitError + macro CREO_CHECK
  detail/protoolkit_compat.hpp Bascule SDK réel / shim
  detail/protoolkit_shim.hpp   Types de substitution (utilisés seulement
                                si le SDK réel n'est pas sur le chemin
                                d'inclusion)
  detail/utf8.hpp              Conversion UTF-8 <-> wide, sans dépendance
error.cpp                      Implémentation de ProToolkitError/ToString
```

## Important : ceci est une copie

Ce dossier est une copie manuelle de `include/creo/` et `src/error.cpp` à
la racine du dépôt — c'est volontairement le même code, pas une variante.
Si le wrapper évolue (nouveaux types, corrections), pensez à resynchroniser
ce dossier avec la source d'origine avant de le recopier dans un projet
externe.
