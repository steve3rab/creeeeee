// Exemple d'utilisation du wrapper C++17 pour ProTOOLKIT.
//
// Deux parties, à lire dans cet ordre :
//
//  1) DemonstrateTypes() : tour des types du wrapper (Name, Line, Path,
//     CharName, gestion d'erreurs) et de leurs conversions. Ne dépend
//     d'aucune fonction ProTOOLKIT réelle : s'exécute à l'identique en
//     mode SDK réel et en mode shim, donc vous pouvez lancer cet
//     exécutable directement (`./hello_creo`) même sans Creo installé.
//
//  2) DemonstrateArray() : tour de creo::Array<T> (RAII autour de
//     ProArray). Comme DemonstrateTypes(), fonctionne identiquement en
//     mode SDK réel et en mode shim (le shim réimplémente réellement le
//     comportement d'un ProArray, pas juste sa forme).
//
//  3) PrintCurrentModelName() : récupère le nom du modèle actif via un
//     enchaînement de deux appels ProTOOLKIT (ProMdlCurrentGet puis
//     ProMdlMdlNameGet), le tout dans un seul try/catch — illustre comment
//     CREO_CHECK court-circuite la suite du bloc dès la première erreur.
//     Nécessite le SDK réel (CREO_TOOLKIT_ROOT, voir README) pour faire
//     quoi que ce soit d'utile ; sinon main() affiche un message explicatif.
//
// Note technique : ce fichier n'utilise que std::printf/std::puts (jamais
// std::wprintf) pour l'affichage. Mélanger des appels "wide" et "narrow"
// sur le même flux stdout est un comportement indéfini en C/C++ (le flux
// se fige sur la première orientation utilisée) : creo::ToString() suffit
// pour tout afficher, y compris le contenu d'un buffer wide comme ProName.

#include "creo/array.hpp"
#include "creo/error.hpp"
#include "creo/types.hpp"

#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace {

void DemonstrateTypes() {
  std::puts("--- Types texte : Name / Line / Path ---");

  // Construction depuis un littéral wide (L"...") ou directement depuis une
  // std::string en UTF-8 : les deux constructeurs existent sur chaque type.
  creo::Name part_name(L"engrenage_01");
  creo::Line message(std::string("pièce vérifiée : OK"));
  creo::Path file_path("/home/user/creo/projet/engrenage_01.prt");

  std::printf("Name : %s (longueur wide = %zu)\n", part_name.ToString().c_str(),
              part_name.ToWString().size());
  std::printf("Line : %s\n", message.ToString().c_str());
  std::printf("Path : %s\n", file_path.ToString().c_str());

  // La capacité (PRO_NAME_SIZE = 32 pour Name) est vérifiée : pas de
  // troncature silencieuse comme en C, une exception est levée à la place.
  try {
    creo::Name too_long(std::wstring(50, L'x'));
  } catch (const std::length_error &e) {
    std::printf("Capacité dépassée (attendu) : %s\n", e.what());
  }

  // Variante en char (FixedCharString), pour les buffers ProTOOLKIT qui ne
  // sont pas wide (ProCharName, ProMenuName, ProMenubuttonName, ...).
  creo::CharName menu_entry("EDIT_FEATURE");
  std::printf("CharName : %s\n", menu_entry.ToString().c_str());

  std::puts("\n--- Comparaisons, View() et interop std::filesystem ---");

  // Comparaison par contenu, y compris contre un littéral wide : voir le
  // README ("Comparaisons, View() et interopérabilité") pour l'ambiguïté
  // de résolution de surcharge que ces opérateurs évitent.
  creo::Name same_part(L"engrenage_01");
  std::printf("part_name == same_part : %s\n",
              part_name == same_part ? "oui" : "non");
  std::printf("part_name == L\"engrenage_01\" : %s\n",
              part_name == L"engrenage_01" ? "oui" : "non");

  // View() : accès sans copie au contenu (contrairement à ToWString(),
  // qui alloue une nouvelle std::wstring à chaque appel). ends_with() est
  // du C++20 : ce wrapper visant le C++17, on compare via substr/compare.
  std::wstring_view view = file_path.View();
  constexpr std::wstring_view kExt = L".prt";
  bool ends_with_prt =
      view.size() >= kExt.size() &&
      view.compare(view.size() - kExt.size(), kExt.size(), kExt) == 0;
  std::printf("file_path se termine par .prt : %s\n",
              ends_with_prt ? "oui" : "non");

  // Path <-> std::filesystem::path.
  std::filesystem::path fs_path = creo::ToFilesystemPath(file_path);
  creo::Path round_trip = creo::PathFromFilesystem(fs_path / ".." / "autre.prt");
  std::printf("Round-trip filesystem::path : %s\n",
              round_trip.ToString().c_str());

  std::puts("\n--- Boolean (ProBoolean/ProBool) ---");

  // ProBoolean est un type distinct du bool C++ côté ProTOOLKIT (même si
  // ses deux valeurs, PRO_B_FALSE/PRO_B_TRUE, coïncident numériquement
  // avec false/true) : ToBool()/ToProBoolean() évitent d'écrire la
  // conversion à la main à chaque appel d'une fonction qui en attend un.
  creo::Boolean flag = creo::ToProBoolean(true);
  std::printf("ToBool(flag) : %s\n", creo::ToBool(flag) ? "true" : "false");
  std::printf("ValueUnused = %d, ValueDefault = %d\n", creo::ValueUnused,
              creo::ValueDefault);

  std::puts("\n--- Gestion d'erreurs : CREO_CHECK / ProToolkitError ---");

  // Simule un appel ProTOOLKIT qui échouerait avec PRO_TK_BAD_INPUTS (-2),
  // sans avoir besoin d'un vrai appel : montre uniquement le mécanisme de
  // conversion ProError -> exception (fonctionne identiquement pour un
  // vrai appel, ex : CREO_CHECK(ProMdlMdlNameGet(model.Raw(), name.Raw()));).
  try {
    CREO_CHECK(static_cast<creo::ErrorCode>(-2));
  } catch (const creo::ProToolkitError &e) {
    std::printf("Erreur interceptée : %s (code brut = %d)\n", e.what(),
                static_cast<int>(e.code()));
  }
}

void DemonstrateArray() {
  std::puts("\n--- creo::Array<T> (RAII autour de ProArray) ---");

  creo::Array<int> values(0, 4); // vide, croît par blocs de 4
  for (int i = 1; i <= 5; ++i) {
    values.Append(i * 10);
  }
  std::printf("Taille après 5 Append : %d\n", values.Size());

  values.Insert(1, 999); // décale le reste
  std::printf("Après Insert(1, 999) : ");
  for (int v : values) {
    std::printf("%d ", v);
  }
  std::putchar('\n');

  values.Remove(1); // retire l'élément qu'on vient d'insérer
  std::printf("Après Remove(1) : ");
  for (int v : values) {
    std::printf("%d ", v);
  }
  std::putchar('\n');

  try {
    values.At(100);
  } catch (const std::out_of_range &e) {
    std::printf("At(100) hors limites (attendu) : %s\n", e.what());
  }
}

#if CREO_WRAPPER_HAS_REAL_SDK
// Récupère le nom du modèle actuellement actif dans la session Creo.
// Retourne false (et affiche le motif) si aucun modèle n'est actif ou si
// un appel ProTOOLKIT échoue.
//
// Montre le fonctionnement de CREO_CHECK sur ProMdlCurrentGet (sortie via
// pointeur, d'où le `&`), puis de ModelHandle::Name() qui enveloppe à la
// fois la vérification de validité du handle et l'appel ProMdlMdlNameGet
// (qui remplace ProMdlNameGet, désormais dépréciée en Creo 10). Si
// ProMdlCurrentGet échoue, Name() n'est jamais atteint : l'exception saute
// directement au catch, sans `if` intermédiaire à écrire soi-même.
bool PrintCurrentModelName() {
  try {
    creo::detail::RawMdl raw_model = nullptr;
    CREO_CHECK(ProMdlCurrentGet(&raw_model));

    creo::ModelHandle model(raw_model);
    if (!model) {
      std::puts("Aucun modèle actif dans la session Creo.");
      return false;
    }

    std::printf("Modèle actif : %s\n", model.Name().ToString().c_str());
    return true;

  } catch (const creo::ProToolkitError &e) {
    std::printf("Impossible de récupérer le modèle actif : %s\n", e.what());

    // e.code() permet un traitement différencié si besoin, par ex :
    if (e.code() == static_cast<creo::ErrorCode>(-4)) { // PRO_TK_E_NOT_FOUND
      std::puts("(aucun modèle n'est actuellement chargé dans Creo)");
    }
    return false;
  }
}
#endif

} // namespace

int main() {
  // Filet de sécurité : chaque section a déjà son propre try/catch pour
  // les erreurs attendues (ProToolkitError, std::out_of_range, ...), mais
  // une exception vraiment imprévue (ex: std::bad_alloc) ne doit pas
  // remonter jusqu'à std::terminate() sans message exploitable.
  try {
    DemonstrateTypes();
    DemonstrateArray();

    std::puts("\n--- Session Creo : nom du modèle actif ---");
#if CREO_WRAPPER_HAS_REAL_SDK
    PrintCurrentModelName();
#else
    std::puts(
        "SDK ProTOOLKIT introuvable : cette partie a été compilée en mode "
        "'shim'. Renseignez CREO_TOOLKIT_ROOT (cf. README) et recompilez "
        "pour l'exécuter dans une vraie session Creo 10.");
#endif
  } catch (const std::exception &e) {
    std::fprintf(stderr, "Erreur inattendue : %s\n", e.what());
    return 1;
  }
  return 0;
}
