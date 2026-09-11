#include "creo/error.hpp"

namespace creo {

namespace {

std::string MakeMessage(ErrorCode code, std::string_view context) {
  std::string message =
      "ProTOOLKIT error: " + ToString(code);
  if (!context.empty()) {
    message += " (";
    message += context;
    message += ")";
  }
  return message;
}

} // namespace

ProToolkitError::ProToolkitError(ErrorCode code, std::string_view context)
    : std::runtime_error(MakeMessage(code, context)), code_(code) {}

std::string ToString(ErrorCode code) {
  if (code == detail::kNoError) {
    return "PRO_TK_NO_ERROR";
  }
  // On évite volontairement de coder en dur les ~150 libellés de
  // ProToolkitErrors.h : n'ayant pas la certitude qu'ils correspondent
  // exactement à la version Creo 10 ciblée, un mauvais mapping serait pire
  // qu'une absence de mapping. Le code numérique reste néanmoins la donnée
  // qui compte pour un traitement automatisé (voir ProToolkitError::code()).
  return "code #" + std::to_string(static_cast<int>(code)) +
         " (voir ProToolkitErrors.h du SDK Creo 10 pour le libellé exact)";
}

} // namespace creo
