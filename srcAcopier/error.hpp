#pragma once
#include "protoolkit_compat.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace creo {

// Code d'erreur natif ProTOOLKIT (ProError), réexposé tel quel : le wrapper
// ne réinvente pas cette énumération, il s'appuie sur celle du SDK PTC
// quand celui-ci est disponible (cf. detail/protoolkit_compat.hpp).
using ErrorCode = detail::ProErrorCode;

// Exception levée par le wrapper à chaque appel ProTOOLKIT en échec.
// Conserve le code d'erreur natif pour permettre un traitement fin en amont
// (catch ciblé sur un code précis), en plus du message lisible standard
// hérité de std::runtime_error.
class ProToolkitError : public std::runtime_error {
public:
  ProToolkitError(ErrorCode code, std::string_view context);

  ErrorCode code() const noexcept { return code_; }

private:
  ErrorCode code_;
};

// Convertit un ProError en texte, ex : ToString(-2) -> "PRO_TK_BAD_INPUTS".
// Couvre l'intégralité de l'énum ProError/ProErr officielle (ProError.h,
// Creo 10). Pour un code hors de cette liste (autre version de Creo, ou
// code applicatif), renvoie le numéro brut plutôt que d'inventer un
// libellé — voir src/error.cpp pour la table complète.
std::string ToString(ErrorCode code);

// Lève une ProToolkitError si `code` n'est pas PRO_TK_NO_ERROR. `context`
// sert de repère de diagnostic (typiquement le nom de l'appel ProTOOLKIT
// concerné) et apparaît dans le message de l'exception.
inline void ThrowIfError(ErrorCode code, std::string_view context = {}) {
  if (code != detail::kNoError) {
    throw ProToolkitError(code, context);
  }
}

} // namespace creo

// Appelle `expr` (une fonction ProTOOLKIT retournant un ProError) et
// transforme automatiquement un échec en creo::ProToolkitError, en
// capturant l'expression elle-même comme contexte de diagnostic. Usage :
//
//   CREO_CHECK(ProMdlMdlNameGet(model.Raw(), name.Raw()));
//
#define CREO_CHECK(expr) ::creo::ThrowIfError((expr), #expr)
