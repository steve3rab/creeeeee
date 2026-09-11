# creeeeee

Wrapper C++17 pour **ProTOOLKIT**, l'API C de PTC pour Creo Parametric.
Ciblé sur **Creo Parametric 10.0**.

## À propos

ProTOOLKIT expose une API C bas niveau (handles opaques, buffers de texte à
taille fixe, codes d'erreur entiers) pour développer des applications qui
s'intègrent à Creo Parametric. Ce dépôt fournit une couche C++17 au-dessus
de cette API afin de rendre son usage plus sûr et plus idiomatique :

- des types forts pour les buffers texte ProTOOLKIT (`ProName`, `ProLine`,
  `ProPath`) avec conversions vérifiées vers/depuis `std::wstring` ;
- un handle de modèle typé (`ProMdl`) ;
- des exceptions C++ (`creo::ProToolkitError`) à la place des codes
  `ProError` à vérifier manuellement après chaque appel.

Le projet démarre par ces briques de base (types + gestion d'erreurs) ;
d'autres wrappers (features, paramètres, géométrie, UI...) viendront s'y
ajouter au fur et à mesure.

### Prérequis importants

Le SDK ProTOOLKIT est **propriétaire** (livré par PTC avec Creo) et n'est
pas inclus dans ce dépôt. Pour compiler avec la vraie API Creo, il vous
faut une installation de Creo Parametric 10.0 disposant du SDK ProTOOLKIT.

Sans ce SDK, le projet compile quand même : `include/creo/detail/` bascule
automatiquement sur un mode « shim » (types de substitution, cf.
commentaires dans `protoolkit_compat.hpp` et `protoolkit_shim.hpp`) qui
permet de développer et de tester la logique du wrapper hors poste Creo.
Un exécutable compilé dans ce mode ne peut évidemment pas piloter une
session Creo réelle.

## Structure du dépôt

```
include/creo/
  types.hpp                       Types de base : Name, Line, Path, ModelHandle
  error.hpp                       ProToolkitError + macro CREO_CHECK
  detail/protoolkit_compat.hpp    Bascule SDK réel / shim
  detail/protoolkit_shim.hpp      Types de substitution (sans SDK)
src/
  error.cpp
examples/
  hello_creo.cpp                  Exemple minimal (nom du modèle actif)
cmake/
  FindProToolkit.cmake            Localise le SDK ProTOOLKIT installé
```

## Compilation

```bash
# Sans le SDK Creo (mode shim, pour développer/tester le wrapper) :
cmake -S . -B build
cmake --build build

# Avec le SDK Creo 10 (pour un build utilisable dans une session Creo) :
cmake -S . -B build \
  -DCREO_TOOLKIT_ROOT="/chemin/vers/Creo 10.0.0.0/Common Files" \
  -DCREO_TOOLKIT_ARCH=<nom_du_dossier_arch_du_sdk>
cmake --build build
```

`CREO_TOOLKIT_ROOT` et `CREO_TOOLKIT_ARCH` peuvent aussi être fournis en
variables d'environnement. Voir `cmake/FindProToolkit.cmake` pour le détail
des chemins recherchés.

## Utilisation

```cpp
#include "creo/error.hpp"
#include "creo/types.hpp"

creo::detail::RawMdl raw_model = nullptr;
CREO_CHECK(ProMdlCurrentGet(&raw_model));

creo::ModelHandle model(raw_model);
creo::ModelName name;
CREO_CHECK(ProMdlNameGet(model.Raw(), name.Raw()));

std::wprintf(L"Modèle actif : %ls\n", name.ToWString().c_str());
```

`CREO_CHECK` enveloppe n'importe quel appel ProTOOLKIT retournant un
`ProError` et lève une `creo::ProToolkitError` en cas d'échec, avec le code
d'erreur natif accessible via `.code()`.

## Contribuer

Les contributions sont les bienvenues. N'hésitez pas à ouvrir une issue ou
une pull request.

## Licence

À définir.
