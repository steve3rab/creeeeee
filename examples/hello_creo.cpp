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
//  2) main(), partie "session Creo" : récupère le nom du modèle actif.
//     Nécessite le SDK réel (CREO_TOOLKIT_ROOT, voir README) pour faire
//     quoi que ce soit d'utile ; sinon affiche un message explicatif.
//
// Note technique : ce fichier n'utilise que std::printf/std::puts (jamais
// std::wprintf) pour l'affichage. Mélanger des appels "wide" et "narrow"
// sur le même flux stdout est un comportement indéfini en C/C++ (le flux
// se fige sur la première orientation utilisée) : creo::ToString() suffit
// pour tout afficher, y compris le contenu d'un buffer wide comme ProName.

#include "creo/error.hpp"
#include "creo/types.hpp"

#include <cstdio>
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

} // namespace

int main() {
  DemonstrateTypes();

  std::puts("\n--- Session Creo : nom du modèle actif ---");
#if CREO_WRAPPER_HAS_REAL_SDK
  creo::detail::RawMdl raw_model = nullptr;
  CREO_CHECK(ProMdlCurrentGet(&raw_model));

  creo::ModelHandle model(raw_model);
  if (!model) {
    std::puts("Aucun modèle actif dans la session Creo.");
    return 1;
  }

  creo::ModelName name;
  // ProMdlMdlNameGet remplace ProMdlNameGet, désormais dépréciée en Creo 10.
  CREO_CHECK(ProMdlMdlNameGet(model.Raw(), name.Raw()));

  std::printf("Modèle actif : %s\n", name.ToString().c_str());
#else
  std::puts(
      "SDK ProTOOLKIT introuvable : cette partie a été compilée en mode "
      "'shim'. Renseignez CREO_TOOLKIT_ROOT (cf. README) et recompilez "
      "pour l'exécuter dans une vraie session Creo 10.");
#endif
  return 0;
}
