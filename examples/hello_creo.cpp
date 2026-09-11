// Exemple minimal d'utilisation du wrapper : récupère le nom du modèle
// actif dans la session Creo et l'affiche.
//
// - Compilé avec le SDK ProTOOLKIT réel (CREO_TOOLKIT_ROOT renseigné, voir
//   README), ce fichier produit un exécutable ProTOOLKIT valide, à lancer
//   comme n'importe quel exécutable ProTOOLKIT depuis une session Creo 10.
// - Compilé sans le SDK (mode shim), il se contente d'expliquer pourquoi il
//   ne peut rien faire de plus : cela permet de vérifier que le wrapper
//   compile même hors poste Creo.

#include "creo/error.hpp"
#include "creo/types.hpp"

#include <cstdio>

int main() {
#if CREO_WRAPPER_HAS_REAL_SDK
  creo::detail::RawMdl raw_model = nullptr;
  CREO_CHECK(ProMdlCurrentGet(&raw_model));

  creo::ModelHandle model(raw_model);
  if (!model) {
    std::puts("Aucun modèle actif dans la session Creo.");
    return 1;
  }

  creo::ModelName name;
  CREO_CHECK(ProMdlNameGet(model.Raw(), name.Raw()));

  std::wprintf(L"Modèle actif : %ls\n", name.ToWString().c_str());
#else
  std::puts(
      "SDK ProTOOLKIT introuvable : cet exemple a été compilé en mode "
      "'shim'. Renseignez CREO_TOOLKIT_ROOT (cf. README) et recompilez "
      "pour l'exécuter dans une vraie session Creo 10.");
#endif
  return 0;
}
