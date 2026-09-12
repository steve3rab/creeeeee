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
  array.hpp                       Array<T> : RAII autour de ProArray
  error.hpp                       ProToolkitError + macro CREO_CHECK
  detail/protoolkit_compat.hpp    Bascule SDK réel / shim
  detail/protoolkit_shim.hpp      Types de substitution (sans SDK)
src/
  error.cpp
examples/
  hello_creo.cpp                  Tour des types/erreurs/Array + nom du modèle actif
cmake/
  FindProToolkit.cmake            Localise le SDK ProTOOLKIT installé
srcAcopier/
  types.hpp, array.hpp,           Version à plat (aucun sous-dossier, pas
  error.hpp,                      de commentaires) pour un vrai projet
  protoolkit_compat.hpp,          ProTOOLKIT (SDK + licence disponibles) :
  utf8.hpp, error.cpp             CREO_WRAPPER_HAS_REAL_SDK y est figé à 1
                                   (pas de mode shim, le SDK réel est
                                   requis à la compilation).
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
// model.Name() enveloppe CREO_CHECK + ProMdlMdlNameGet (qui remplace
// ProMdlNameGet, désormais dépréciée en Creo 10) et lève std::logic_error
// si le handle est invalide, sans même tenter l'appel ProTOOLKIT.
creo::ModelName name = model.Name();

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
conversion, indépendante de toute bibliothèque externe. La conversion
valide strictement son entrée dans les deux sens (séquences UTF-8
tronquées/mal formées, encodages surlongs, substituts UTF-16 isolés,
points de code hors de l'intervalle Unicode) : tout ce qui est invalide
est remplacé par le caractère de remplacement `U+FFFD` plutôt que d'être
silencieusement laissé passer ou de faire planter la conversion — utile
puisque ce texte peut provenir d'un fichier modèle externe.

Chaque type texte expose `kCapacity` (taille totale du buffer, terminateur
inclus) et `kMaxLength = kCapacity - 1` (nombre de caractères réellement
stockables). C'est `kMaxLength`, pas `kCapacity`, qui borne la taille
acceptée par `Assign()`/le constructeur.

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

### Comparaisons, `View()` et interopérabilité

Tous les types texte (`Name`, `Line`, `Path`, `ModelName`, `CharName`, ...)
sont comparables directement, entre eux comme contre une chaîne C++ :

```cpp
creo::Name a(L"engrenage_01");
creo::ModelName b(L"engrenage_01");   // capacité différente, comparaison OK

if (a == b) { /* ... */ }
if (a == L"engrenage_01") { /* ... */ }             // littéral wide
if (a == std::wstring_view(L"engrenage_01")) { }    // wstring_view / wstring

creo::CharName cn("MENU_A");
if (cn == "MENU_A") { /* ... */ }                   // littéral char
```

Ces opérateurs comparent le **contenu** (via `View()`, ci-dessous), jamais
l'adresse du buffer — un point qui mérite d'être explicite : chaque type
expose aussi un `operator wchar_t*()`/`operator char*()` implicite,
nécessaire pour l'interop directe avec les fonctions C ProTOOLKIT
(`ProConfigoptSet(option, option_value)`). Sans une surcharge dédiée
`const wchar_t*`/`const char*` en plus de celles en `wstring_view`/
`string_view`, comparer contre un littéral serait ambigu pour le
compilateur (deux conversions implicites de même rang : vers pointeur, ou
vers vue) ; ces surcharges existent précisément pour lever cette ambiguïté
en faveur de la comparaison par contenu.

`View()` renvoie un `std::wstring_view`/`std::string_view` sur le buffer
sans copie (contrairement à `ToWString()`/`ToString()`, qui en allouent une
nouvelle à chaque appel) :

```cpp
std::wstring_view v = path.View();
if (v.substr(v.size() - 4) == L".prt") { /* ... */ }
```

(`std::wstring_view::ends_with` est du C++20 ; ce wrapper cible le C++17.)

`Path` s'interface aussi avec `std::filesystem::path` :

```cpp
#include "creo/types.hpp"

std::filesystem::path fs = creo::ToFilesystemPath(path);
creo::Path p = creo::PathFromFilesystem(fs / "sous_dossier" / "piece.prt");
```

`creo::ValueUnused` correspond à `PRO_VALUE_UNUSED` (= -1), la sentinelle
"valeur/index non utilisé" acceptée par de nombreuses fonctions ProTOOLKIT
(par ex. tout index négatif passé à `ProArrayObjectAdd` ajoute en fin de
tableau — `ValueUnused` en est un exemple, pas la seule valeur qui
déclenche ce comportement). `creo::ValueDefault` correspond à
`PRO_VALUE_DEFAULT` (= -5), la sentinelle "valeur par défaut" — distincte
de `PRO_VALUE_UNUSED` malgré la proximité des noms, à ne pas confondre
dans un appel ProTOOLKIT. En mode SDK réel, les deux reprennent
directement les macros PTC.

### Boolean

`creo::Boolean` correspond à `ProBoolean`/`ProBool` (`ProToolkit.h`, enum
`ProBooleans` : `PRO_B_FALSE = 0`, `PRO_B_TRUE = 1`) — le booléen
ProTOOLKIT, un type distinct du `bool` C++ bien que ses deux valeurs
coïncident numériquement avec `false`/`true`. De nombreuses fonctions
ProTOOLKIT prennent ou renvoient précisément ce type, jamais un `bool`
C++ :

```cpp
creo::Boolean flag = creo::ToProBoolean(true);
// ... CREO_CHECK(UneFonctionProtoolkit(..., flag));

bool value = creo::ToBool(flag);
```

`ToBool()` teste `!= PRO_B_FALSE` plutôt que `== PRO_B_TRUE`, par prudence
défensive envers une valeur qui ne serait ni l'une ni l'autre des deux
documentées.

### ModelHandle

Au-delà de `IsValid()`/`Raw()`, `ModelHandle` expose une méthode de
confort pour le cas le plus courant :

```cpp
creo::ModelHandle model(raw_model);
creo::ModelName name = model.Name();  // CREO_CHECK(ProMdlMdlNameGet(...)) intégré
```

`Name()` lève `std::logic_error` (pas une `ProToolkitError`) si le handle
est invalide (nul) : l'erreur est détectée avant même de tenter l'appel
ProTOOLKIT, avec un message plus explicite qu'un `PRO_TK_BAD_INPUTS`
générique remonté depuis le SDK.

Deux `ModelHandle` sont comparables par égalité — ils désignent le même
modèle si et seulement s'ils portent le même handle ProTOOLKIT sous-jacent
(pas seulement le même nom, deux modèles distincts pouvant partager un nom
générique) :

```cpp
if (model1 == model2) { /* même modèle */ }
```

### Array&lt;T&gt;

`creo::Array<T>` (`include/creo/array.hpp`) enveloppe `ProArray`
(`ProArray.h`) : le tableau dynamique générique de ProTOOLKIT. À la
différence de `ModelHandle` (non-propriétaire — Creo gère le cycle de vie
d'un modèle), un `ProArray` est explicitement alloué/libéré par
l'appelant : `Array<T>` en prend donc la propriété complète en RAII
(alloue à la construction, libère au destructeur).

```cpp
#include "creo/array.hpp"

creo::Array<int> values(0, 8); // vide, croît par blocs de 8 éléments
values.Append(10);
values.Append(20);
values.Insert(1, 15);           // -> 10, 15, 20
values.Remove(0);               // -> 15, 20

for (int v : values) { /* ... */ }   // itération standard (begin()/end())
int v = values[0];                    // accès non vérifié, comme std::vector
int w = values.At(0);                 // accès vérifié, lève std::out_of_range

// Prendre possession d'un ProArray déjà alloué par une autre fonction
// ProTOOLKIT (au lieu d'en allouer un nouveau) :
creo::Array<ProFeature> feats = creo::Array<ProFeature>::Adopt(raw_pro_array);
```

**`T` peut être n'importe quel type C++** (`std::string`, une classe avec
destructeur/membres possédés, ...), pas seulement un type trivialement
copiable — voir plus bas pourquoi, et pourquoi ça a nécessité une
implémentation différente de ce qu'un simple appel aux fonctions
ProTOOLKIT natives aurait donné.

Points importants :
- **Déplaçable, non copiable** : ProTOOLKIT n'offre pas de primitive de
  duplication ; une copie profonde élément par élément serait coûteuse et
  surprenante à faire passer pour un simple constructeur de copie.
- **Gestion mémoire réellement C++, pas C** : les fonctions natives
  `ProArrayObjectAdd`/`ProArrayObjectRemove`/`ProArraySizeSet` déplacent
  les éléments par copie mémoire brute (`memmove`/`realloc` côté C), sans
  jamais appeler de constructeur/destructeur C++ — sans risque pour un
  type trivialement copiable, mais qui corromprait un type qui ne l'est
  pas (`std::string` peut stocker un pointeur interne vers son propre
  buffer ; le déplacer par `memmove` l'invalide). `Array<T>` n'utilise
  donc `ProArray` QUE comme fournisseur de mémoire brute
  (`ProArrayAlloc`/`ProArrayFree`) : toute la gestion du cycle de vie des
  éléments (construction, destruction, déplacement lors d'une croissance
  ou d'un décalage) est implémentée en C++ pur — exactement comme
  `std::vector` au-dessus de son allocateur — avec la garantie forte
  d'exception sur `Reserve()` (préférant la copie au déplacement quand ce
  dernier peut lever, via `std::move_if_noexcept`, comme le fait la
  bibliothèque standard). Résultat : aucune restriction de type visible
  pour l'utilisateur du wrapper.
- `Insert(index, ...)` : un `index` négatif équivaut à insérer en fin de
  tableau (mêmes bornes que `Append`).
- `Adopt()` reste, lui, spécifiquement réservé à un `T` trivialement
  copiable (vérifié par `static_assert`) : un `ProArray` construit par
  ProTOOLKIT lui-même ne peut contenir que des données C, jamais des
  objets C++ déjà construits.

En mode shim (sans SDK), `ProArrayAlloc`/`ProArrayFree` (et
`ProArraySizeGet`, utilisée par `Adopt()`) sont réimplémentées
fonctionnellement plutôt que d'être de simples types de substitution :
contrairement à `ProMdl`/`ProError`, `Array<T>` a un vrai comportement à
exercer pour être testable sans Creo installé. Le shim reproduit aussi
`ProArraySizeSet`/`ProArrayObjectAdd`/`ProArrayObjectRemove` par fidélité
à `ProArray.h`, même si `Array<T>` ne les utilise plus (voir plus haut).

### ObjectType

`creo::ObjectType` correspond à `ProType` (`pro_obj_types`,
`ProObjects.h`) : le type d'objet de base de données Creo **au sens
large**, pas seulement les modèles. Les modèles au sens courant
(part/assemblage/dessin/manufacturing/...) n'en sont qu'une petite
partie, à côté des features, courbes, entités de simulation, de
maillage, d'animation, etc. — plusieurs centaines de valeurs au total.

```cpp
creo::ObjectType t = creo::ObjectType::PRO_PART;
if (t == creo::ObjectType::PRO_ASSEMBLY) { /* ... */ }
```

Quelques valeurs "modèle" pour repère : `PRO_ASSEMBLY` (1), `PRO_PART`
(2), `PRO_DRAWING` (4), `PRO_MFG` (37), `PRO_SUB_ASSEMBLY` (34),
`PRO_DWGFORM` (33), `PRO_LAYOUT` (19), `PRO_REPORT` (105), `PRO_MARKUP`
(116), `PRO_DIAGRAM` (121).

Une valeur de l'énum réelle a été omise : `PRO_TYPE_UNUSED` (définie côté
PTC comme `= PRO_VALUE_UNUSED`), car `PRO_VALUE_UNUSED` n'a pas été
fournie et sa valeur n'a pas été devinée. Un build avec le SDK réel
l'obtient normalement via l'en-tête PTC ; seul le mode shim (hors SDK) ne
la propose pas.

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
d'imbrication d'assemblage pris en charge par ProTOOLKIT. `ValueUnused`/
`ValueDefault` (`PRO_VALUE_UNUSED`/`PRO_VALUE_DEFAULT`) et `Boolean`
(`ProBoolean`/`ProBool`) sont documentés plus haut, voir « Comparaisons,
`View()` et interopérabilité » et « Boolean ».

## Contribuer

Les contributions sont les bienvenues. N'hésitez pas à ouvrir une issue ou
une pull request.

## Licence

À définir.
