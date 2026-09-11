#pragma once
// -----------------------------------------------------------------------
// Conversion UTF-8 <-> wide string, sans dépendance externe.
//
// ProTOOLKIT manipule le texte en wchar_t, mais la taille (et donc
// l'encodage implicite) de wchar_t diffère selon la plateforme :
//   - 2 octets, UTF-16 (avec paires de substituts) sous Windows ;
//   - 4 octets, UTF-32 (un wchar_t = un point de code) sous Linux/macOS.
//
// `std::wstring_convert`/`codecvt_utf8<wchar_t>` ne gère correctement que
// le second cas et est de toute façon dépréciée depuis C++17 : ces
// fonctions font la conversion à la main, correctement dans les deux cas,
// pour offrir une représentation std::string (UTF-8) stable quelle que
// soit la plateforme de compilation.
//
// Les deux directions valident strictement leur entrée (séquences UTF-8
// tronquées/mal formées, surrogates UTF-16 isolés, points de code hors de
// l'intervalle Unicode valide) et substituent le caractère de remplacement
// U+FFFD plutôt que de tronquer ou de laisser passer une séquence
// invalide : le texte manipulé ici peut provenir d'un fichier modèle
// externe, il n'y a pas de raison de lui faire confiance par défaut.
// -----------------------------------------------------------------------

#include <cstdint>
#include <string>
#include <string_view>

namespace creo::detail {

inline constexpr std::uint32_t kUnicodeReplacementChar = 0xFFFD;
inline constexpr std::uint32_t kUnicodeMaxCodepoint = 0x10FFFF;

inline void AppendUtf8(std::string &out, std::uint32_t codepoint) {
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

// Ajoute un point de code à une chaîne wide, en le découpant en paire de
// substituts UTF-16 si nécessaire (wchar_t 2 octets). Centralise cette
// logique pour FromUtf8() (chemin normal et substitution U+FFFD).
inline void AppendWide(std::wstring &out, std::uint32_t codepoint) {
  if constexpr (sizeof(wchar_t) == 2) {
    if (codepoint > 0xFFFF) {
      codepoint -= 0x10000;
      out.push_back(static_cast<wchar_t>(0xD800 + (codepoint >> 10)));
      out.push_back(static_cast<wchar_t>(0xDC00 + (codepoint & 0x3FF)));
      return;
    }
  }
  out.push_back(static_cast<wchar_t>(codepoint));
}

// Encode une chaîne wide (telle que renvoyée par un buffer ProTOOLKIT
// ProName/ProLine/ProPath) en UTF-8. `reserve` majore la taille de sortie
// au pire cas (4 octets UTF-8 par unité wide) pour éviter toute
// réallocation en cours de conversion.
inline std::string ToUtf8(std::wstring_view text) {
  std::string out;
  out.reserve(text.size() * 4);

  if constexpr (sizeof(wchar_t) == 2) {
    // wchar_t = unité UTF-16 : recombiner les paires de substituts, et
    // remplacer par U+FFFD tout substitut isolé (haut sans bas suivant,
    // ou bas sans haut précédent) plutôt que de produire de l'UTF-8
    // invalide représentant directement un point de code de substitution.
    for (std::size_t i = 0; i < text.size(); ++i) {
      std::uint32_t unit = static_cast<std::uint16_t>(text[i]);
      if (unit >= 0xD800 && unit <= 0xDBFF && i + 1 < text.size()) {
        std::uint32_t low = static_cast<std::uint16_t>(text[i + 1]);
        if (low >= 0xDC00 && low <= 0xDFFF) {
          AppendUtf8(out, 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00));
          ++i;
          continue;
        }
      }
      if (unit >= 0xD800 && unit <= 0xDFFF) {
        AppendUtf8(out, kUnicodeReplacementChar);
      } else {
        AppendUtf8(out, unit);
      }
    }
  } else {
    // wchar_t = point de code direct (UTF-32).
    for (wchar_t ch : text) {
      auto codepoint = static_cast<std::uint32_t>(ch);
      if (codepoint > kUnicodeMaxCodepoint ||
          (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        AppendUtf8(out, kUnicodeReplacementChar);
      } else {
        AppendUtf8(out, codepoint);
      }
    }
  }
  return out;
}

// Décode une chaîne UTF-8 en wide string, pour construire un buffer
// ProTOOLKIT (ProName/ProLine/ProPath) à partir d'un std::string. Rejette
// (remplace par U+FFFD) les séquences tronquées, les octets de
// continuation invalides, les encodages surlongs (ex : un octet ASCII
// réencodé sur 2 octets) et les points de code hors de l'intervalle
// Unicode valide ou dans la plage réservée aux substituts UTF-16 : une
// séquence UTF-8 ne doit jamais encoder directement 0xD800-0xDFFF.
inline std::wstring FromUtf8(std::string_view text) {
  std::wstring out;
  out.reserve(text.size());

  std::size_t i = 0;
  while (i < text.size()) {
    unsigned char c0 = static_cast<unsigned char>(text[i]);
    std::uint32_t codepoint = 0;
    std::size_t extra = 0;
    std::uint32_t min_codepoint = 0;

    if (c0 < 0x80) {
      codepoint = c0;
    } else if ((c0 & 0xE0) == 0xC0) {
      codepoint = c0 & 0x1F;
      extra = 1;
      min_codepoint = 0x80;
    } else if ((c0 & 0xF0) == 0xE0) {
      codepoint = c0 & 0x0F;
      extra = 2;
      min_codepoint = 0x800;
    } else if ((c0 & 0xF8) == 0xF0) {
      codepoint = c0 & 0x07;
      extra = 3;
      min_codepoint = 0x10000;
    } else {
      AppendWide(out, kUnicodeReplacementChar);
      ++i;
      continue;
    }

    if (i + extra >= text.size()) {
      AppendWide(out, kUnicodeReplacementChar);
      break;
    }

    bool valid = true;
    for (std::size_t k = 1; k <= extra; ++k) {
      unsigned char c = static_cast<unsigned char>(text[i + k]);
      if ((c & 0xC0) != 0x80) {
        valid = false;
        break;
      }
      codepoint = (codepoint << 6) | (c & 0x3F);
    }

    if (!valid || codepoint < min_codepoint ||
        codepoint > kUnicodeMaxCodepoint ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
      AppendWide(out, kUnicodeReplacementChar);
      ++i;
      continue;
    }

    i += extra + 1;
    AppendWide(out, codepoint);
  }
  return out;
}

} // namespace creo::detail
