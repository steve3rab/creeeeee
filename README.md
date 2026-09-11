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
  hello_creo.cpp                  Tour des types/erreurs + nom du modèle actif
cmake/
  FindProToolkit.cmake            Localise le SDK ProTOOLKIT installé
srcAcopier/
  types.hpp, error.hpp,           Mêmes fichiers que ci-dessus mais à plat
  protoolkit_compat.hpp,          (aucun sous-dossier, mêmes noms de
  protoolkit_shim.hpp,            fichiers) et avec des #include relatifs
  utf8.hpp, error.cpp             (sans préfixe "creo/") : à copier tel
                                   quel dans un vrai projet ProTOOLKIT (SDK
                                   + licence disponibles), sans avoir
                                   besoin de configurer de chemin
                                   d'inclusion supplémentaire.
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

Voir `examples/hello_creo.cpp` pour un exemple complet et commenté — il
s'exécute même sans le SDK (mode shim) puisqu'il montre d'abord les types
et la gestion d'erreurs indépendamment de toute session Creo, avant la
partie qui nécessite réellement le SDK (récupération du modèle actif) :

```bash
cmake -S . -B build && cmake --build build
./build/hello_creo
```

Extrait de la partie qui nécessite une vraie session Creo :

```cpp
#include "creo/error.hpp"
#include "creo/types.hpp"

creo::detail::RawMdl raw_model = nullptr;
CREO_CHECK(ProMdlCurrentGet(&raw_model));

creo::ModelHandle model(raw_model);
creo::ModelName name;
// ProMdlMdlNameGet remplace ProMdlNameGet, désormais dépréciée en Creo 10.
CREO_CHECK(ProMdlMdlNameGet(model.Raw(), name.Raw()));

std::printf("Modèle actif : %s\n", name.ToString().c_str());
```

Note : n'appelez jamais `std::wprintf` et `std::printf` sur `stdout` dans
le même programme — une fois orienté par le premier appel ("wide" ou
"narrow"), un flux C a un comportement indéfini si l'autre orientation est
utilisée ensuite. `ToString()` (UTF-8) suffit pour tout afficher avec
`printf`, y compris le contenu d'un buffer wide comme `ProName`.

`CREO_CHECK` enveloppe n'importe quel appel ProTOOLKIT retournant un
`ProError` et lève une `creo::ProToolkitError` en cas d'échec, avec le code
d'erreur natif accessible via `.code()`. Le message de l'exception inclut
le libellé symbolique du code (`creo::ToString`), qui couvre l'intégralité
de l'énum `ProError`/`ProErr` officielle de Creo 10 (`PRO_TK_BAD_INPUTS`,
`PRO_TK_NO_LICENSE`, ...) — pas seulement le numéro brut.

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

### Fonctions ProTOOLKIT en entrée/sortie (Get / Set)

Beaucoup de fonctions ProTOOLKIT partagent la même forme en C —
`ProError Xxx(ProName option, ProPath option_value)` — que le buffer serve
d'entrée (`...Set`) ou de sortie (`...Get`) ; rien dans le type ne le dit,
c'est une convention documentaire PTC. Le wrapper s'utilise identiquement
dans les deux cas, seule la façon de construire l'objet change :

```cpp
#include "creo/error.hpp"
#include "creo/types.hpp"

// --- Set : le buffer est déjà rempli avant l'appel (entrée) ---
creo::Name option("pro_line_font");
creo::Path option_value("solid");
CREO_CHECK(ProConfigoptSet(option, option_value));

// --- Get : le buffer est vide avant l'appel, rempli par ProTOOLKIT (sortie) ---
creo::Name option2("pro_line_font");
creo::Path option_value2;                    // vide, à remplir
CREO_CHECK(ProConfigoptionGet(option2, option_value2));

std::string value = option_value2.ToString(); // ou .ToWString()
```

Et pour repartir d'un `std::string`/`std::wstring` vers un `ProPath` (par
exemple pour un nouvel appel `...Set` avec une valeur calculée) :

```cpp
std::string new_value = "hidden";

creo::Path p1(new_value);        // construit un nouveau Path
option_value.Assign(new_value);  // ou réutilise un Path existant

CREO_CHECK(ProConfigoptSet(option, option_value));
```

`Assign()` (comme le constructeur) vérifie la capacité du buffer visé
(`Path` = 260 caractères) et lève `std::length_error` plutôt que de
tronquer silencieusement une valeur trop longue.

### Types disponibles (`include/creo/types.hpp`)

Deux familles de buffers texte à taille fixe, selon ce que PTC utilise
côté C (voir `detail/protoolkit_compat.hpp`/`protoolkit_shim.hpp` pour le
détail des constantes `PRO_*_SIZE`) :

**Buffers wide (`wchar_t[N]`, gabarit `FixedWString<N>`)** — la majorité
des types texte ProTOOLKIT depuis Pro/ENGINEER Wildfire :

| Type C++            | Buffer ProTOOLKIT      | Taille | Usage                                    |
|----------------------|------------------------|-------:|-------------------------------------------|
| `Name`               | `ProName`              |     32 | Nom générique (feature, paramètre, ...)  |
| `ModelName`          | `ProMdlName`           |    180 | Nom d'un modèle (ProMdlMdlNameGet)        |
| `Line`               | `ProLine`              |     81 | Ligne de texte (messages)                 |
| `Path`               | `ProPath`              |    260 | Chemin de fichier / répertoire            |
| `Comment`            | `ProComment`           |    256 | Commentaire                               |
| `Value`              | (`PRO_VALUE_SIZE`)     |    256 | Valeur de paramètre (texte)               |
| `FeatRefKey`         | (`PRO_FEATREF_KEY_SIZE`)|     81 | Clé de référence de feature              |
| `ModelExtension`     | `ProMdlExtension`      |     32 | Extension de fichier d'un modèle          |
| `Macro`              | `ProMacro`             |    256 | Macro (taille conservée pour compat. PTC) |
| `MdlFileName`        | `ProMdlFileName`       |    216 | Nom de fichier complet "nom.ext.#"        |
| `FileName`           | `ProFileName`          |     40 | Idem, cas générique                       |
| `FamTabColumnDesc`   | `ProFamtabClmDesc`     |    260 | Description de colonne de table de famille|
| `FamilyMdlName`      | `ProFamilyMdlName`     |    362 | Instance de table de famille "inst[gen]"  |
| `FamilyName`         | `ProFamilyName`        |     66 | Idem, cas générique                       |
| `DisplayModelName`   | `ProDisplayModelName`  |    362 | Nom d'affichage d'un modèle                |
| `ModelTypeCode`      | *(pas de typedef PTC)* |      4 | "prt"/"asm"/"drw" — brique interne         |
| `Extension`          | *(pas de typedef PTC)* |      4 | Extension générique — brique interne       |
| `VersionSuffix`      | *(pas de typedef PTC)* |      4 | Suffixe de version — brique interne        |

`ModelTypeCode`/`Extension`/`VersionSuffix` n'ont pas d'équivalent PTC
autonome : `PRO_TYPE_SIZE`, `PRO_EXTENSION_SIZE` et `PRO_VERSION_SIZE`
n'apparaissent dans les en-têtes PTC que combinés à l'intérieur de
`ProMdlFileName`/`ProFileName`. Ce sont des briques utilitaires du
wrapper, pas la réexposition d'un type PTC.

**Buffers étroits (`char[N]`, gabarit `FixedCharString<N>`)** :

| Type C++          | Buffer ProTOOLKIT     | Taille | Usage                          |
|--------------------|------------------------|-------:|---------------------------------|
| `CharName`         | `ProCharName`          |     32 | Variante char de `Name`         |
| `CharPath`         | `ProCharPath`          |    260 | Variante char de `Path`         |
| `CharLine`         | `ProCharLine`          |     81 | Variante char de `Line` (messages)|
| `MenuName`         | `ProMenuName`          |     32 | Nom de menu                     |
| `MenuFileName`     | `ProMenufileName`      |     32 | Nom de fichier menu (.mnu)      |
| `MenuButtonName`   | `ProMenubuttonName`    |     32 | Nom de bouton de menu           |

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
