# creeeeee

Wrapper C++17 pour **ProTOOLKIT**, l'API C de PTC pour Creo Parametric.
Ciblé sur **Creo Parametric 10.0**.

## À propos

ProTOOLKIT expose une API C bas niveau (handles opaques, buffers de texte à
taille fixe, codes d'erreur entiers) pour développer des applications qui
s'intègrent à Creo Parametric. Ce dépôt fournit une couche C++17 au-dessus
de cette API afin de rendre son usage plus sûr et plus idiomatique :

- des types forts pour les buffers texte ProTOOLKIT (`ProName`, `ProMdlName`,
  `ProLine`, `ProPath`, `ProComment`, `ProValue`, ...) avec conversions
  vérifiées vers/depuis `std::wstring` et `std::string` ;
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
// ProMdlMdlNameGet remplace ProMdlNameGet, désormais dépréciée en Creo 10.
CREO_CHECK(ProMdlMdlNameGet(model.Raw(), name.Raw()));

std::wprintf(L"Modèle actif : %ls\n", name.ToWString().c_str());
std::printf("Modèle actif : %s\n", name.ToString().c_str());
```

`CREO_CHECK` enveloppe n'importe quel appel ProTOOLKIT retournant un
`ProError` et lève une `creo::ProToolkitError` en cas d'échec, avec le code
d'erreur natif accessible via `.code()`.

### Conversions std::string / std::wstring

`Name`, `Line`, `Path` et `ModelName` (tous basés sur `FixedWString<N>`)
peuvent être lus et construits dans les deux représentations :

```cpp
creo::Line l1(L"pièce_déformée");     // depuis un littéral wide
creo::Line l2("pièce_déformée");      // depuis une std::string en UTF-8

std::wstring w = l1.ToWString();      // wide, tel que stocké par ProTOOLKIT
std::string  s = l1.ToString();       // UTF-8
```

L'UTF-8 est utilisé comme représentation `std::string` car `wchar_t` n'a
pas la même taille selon la plateforme (UTF-16 sous Windows, UTF-32 sous
Linux/macOS) : voir `include/creo/detail/utf8.hpp` pour le détail de la
conversion, indépendante de toute bibliothèque externe.

### Types disponibles (`include/creo/types.hpp`)

Chaque type ci-dessous est un `FixedWString<N>` de la taille officielle
PTC pour Creo Parametric 10.0 (voir `detail/protoolkit_compat.hpp` pour le
détail des constantes `PRO_*_SIZE`) :

| Type C++            | Buffer ProTOOLKIT      | Taille | Usage                                   |
|----------------------|------------------------|-------:|------------------------------------------|
| `Name`               | `ProName`              |     32 | Nom générique (feature, paramètre, ...) |
| `ModelName`          | `ProMdlName`           |    180 | Nom d'un modèle (ProMdlMdlNameGet)       |
| `Line`               | `ProLine`              |     81 | Ligne de texte (messages)                |
| `Path`               | `ProPath`               |    260 | Chemin de fichier / répertoire          |
| `Comment`            | `ProComment`            |    256 | Commentaire                              |
| `Value`              | `ProValue`              |    256 | Valeur de paramètre (texte)              |
| `FeatRefKey`         | `ProFeatrefKey`         |     81 | Clé de référence de feature              |
| `ModelTypeCode`      | (`PRO_TYPE_SIZE`)       |      4 | "prt", "asm", "drw", ...                 |
| `Extension`          | (`PRO_EXTENSION_SIZE`)  |      4 | Extension de fichier générique           |
| `ModelExtension`     | (`PRO_MDLEXTENSION_SIZE`)|    32 | Extension de fichier modèle              |
| `VersionSuffix`      | (`PRO_VERSION_SIZE`)    |      4 | Numéro de version dans un nom de fichier |
| `FileMdlName`        | `ProFileMdlname`        |    216 | Nom de fichier complet "nom.ext.#"       |
| `FileName`           | `ProFileName`           |     40 | Idem, cas générique                      |
| `FamTabFieldName`    | `ProFamtabFieldname`    |    260 | Champ de table de famille                |
| `FamilyMdlName`      | `ProFamilyMdlname`      |    362 | Instance de table de famille "inst[gen]" |
| `FamilyName`         | `ProFamilyName`         |     66 | Idem, cas générique                      |

`ModelName` (180) n'est **pas** un alias de `Name` (32) : PTC réserve une
taille bien plus grande aux noms de modèles qu'aux autres noms Creo — les
confondre tronquerait silencieusement un nom de modèle trop long pour un
`Name`.

`MaxAssemLevel` (= 25, `PRO_MAX_ASSEM_LEVEL`) est aussi exposé, mais ce
n'est pas une taille de buffer : c'est le nombre maximum de niveaux
d'imbrication d'assemblage pris en charge par ProTOOLKIT.

## Contribuer

Les contributions sont les bienvenues. N'hésitez pas à ouvrir une issue ou
une pull request.

## Licence

À définir.
