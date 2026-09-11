#pragma once
// -----------------------------------------------------------------------
// Substituts MINIMAUX pour les types ProTOOLKIT de base, utilisés
// uniquement quand le SDK Creo réel n'est pas disponible sur la machine de
// compilation (poste de développement sans Creo installé, CI, etc.). Cela
// permet de compiler et de tester la logique du wrapper indépendamment de
// la présence du SDK PTC.
//
// Les tailles ci-dessous sont les constantes officielles PTC pour Creo
// Parametric 10.0 (confirmées par l'utilisateur à partir des en-têtes
// réels) : PRO_LINE_SIZE, PRO_PATH_SIZE, PRO_COMMENT_SIZE, PRO_VALUE_SIZE,
// PRO_MDLNAME_SIZE, PRO_NAME_SIZE, PRO_TYPE_SIZE, PRO_EXTENSION_SIZE,
// PRO_MDLEXTENSION_SIZE, PRO_VERSION_SIZE, PRO_MAX_ASSEM_LEVEL et
// PRO_FEATREF_KEY_SIZE. Le reste de ce fichier ne fait qu'imiter la forme
// des types (buffers texte, handle opaque) : dès qu'un SDK Creo 10 réel
// est détecté (voir creo/detail/protoolkit_compat.hpp et
// cmake/FindProToolkit.cmake), ce fichier n'est plus inclus et les vrais
// en-têtes PTC prennent le relais automatiquement.
// -----------------------------------------------------------------------

namespace creo::detail::shim {

// --- Tailles "atomiques" (valeurs officielles PTC, Creo 10) --------------

constexpr int kLineSize = 81;
constexpr int kPathSize = 260;
constexpr int kCommentSize = 256;
constexpr int kValueSize = 256;

constexpr int kMdlNameSize = 180; // Nom de modèle Creo Parametric (ProMdl).
constexpr int kNameSize = 32;     // Tout autre nom Creo Parametric.
constexpr int kTypeSize = 4;      // "prt", "asm", "drw", etc. + terminateur.
constexpr int kExtensionSize = 4; // 3 caractères + terminateur NULL.
constexpr int kMdlExtensionSize = 32;
constexpr int kVersionSize = 4;
constexpr int kMaxAssemLevel = 25; // Pas une taille de buffer : nombre max
                                    // de niveaux d'imbrication d'assemblage.
constexpr int kFeatRefKeySize = 81;
// PRO_MACRO_SIZE : conservée par PTC pour compatibilité applicative
// uniquement, ProMacroLoad() n'est plus limitée par cette taille.
constexpr int kMacroSize = 256;

// --- Tailles composites (mêmes formules que les macros PTC) --------------

// "name.ext.#"
constexpr int kFileMdlNameSize = kMdlNameSize + kMdlExtensionSize + kVersionSize;
constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

constexpr int kFamTabFieldNameSize = kPathSize;

// "instance[generic]"
constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;
constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

// Handle de modèle Creo (ProMdl) : un pointeur opaque, jamais déréférencé
// par le code appelant. Seul ProTOOLKIT connaît la structure pointée ; on
// se contente ici de préserver la sémantique "pointeur opaque distinct".
struct ProMdlOpaque;
using ProMdl = ProMdlOpaque *;

// Code de retour des fonctions ProTOOLKIT : reproduction fidèle de l'énum
// `ProError`/`ProErr` officielle (ProError.h, Creo 10 — "most commonly
// used Creo Parametric TOOLKIT error statuses" selon PTC). Utile en mode
// shim pour simuler un code d'erreur précis dans des tests, sans avoir le
// SDK réel installé.
enum ProError : int {
  PRO_TK_NO_ERROR = 0,
  PRO_TK_GENERAL_ERROR = -1,
  PRO_TK_BAD_INPUTS = -2,
  PRO_TK_USER_ABORT = -3,
  PRO_TK_E_NOT_FOUND = -4,
  PRO_TK_E_FOUND = -5,
  PRO_TK_LINE_TOO_LONG = -6,
  PRO_TK_CONTINUE = -7,
  PRO_TK_BAD_CONTEXT = -8,
  PRO_TK_NOT_IMPLEMENTED = -9,
  PRO_TK_OUT_OF_MEMORY = -10,
  PRO_TK_COMM_ERROR = -11, // communication error
  PRO_TK_NO_CHANGE = -12,
  PRO_TK_SUPP_PARENTS = -13,
  PRO_TK_PICK_ABOVE = -14,
  PRO_TK_INVALID_DIR = -15,
  PRO_TK_INVALID_FILE = -16,
  PRO_TK_CANT_WRITE = -17,
  PRO_TK_INVALID_TYPE = -18,
  PRO_TK_INVALID_PTR = -19,
  PRO_TK_UNAV_SEC = -20,
  PRO_TK_INVALID_MATRIX = -21,
  PRO_TK_INVALID_NAME = -22,
  PRO_TK_NOT_EXIST = -23,
  PRO_TK_CANT_OPEN = -24,
  PRO_TK_ABORT = -25,
  PRO_TK_NOT_VALID = -26,
  PRO_TK_INVALID_ITEM = -27,
  PRO_TK_MSG_NOT_FOUND = -28,
  PRO_TK_MSG_NO_TRANS = -29,
  PRO_TK_MSG_FMT_ERROR = -30,
  PRO_TK_MSG_USER_QUIT = -31,
  PRO_TK_MSG_TOO_LONG = -32,
  PRO_TK_CANT_ACCESS = -33,
  PRO_TK_OBSOLETE_FUNC = -34,
  PRO_TK_NO_COORD_SYSTEM = -35,
  PRO_TK_E_AMBIGUOUS = -36,
  PRO_TK_E_DEADLOCK = -37,
  PRO_TK_E_BUSY = -38,
  PRO_TK_E_IN_USE = -39,
  PRO_TK_NO_LICENSE = -40,
  PRO_TK_BSPL_UNSUITABLE_DEGREE = -41,
  PRO_TK_BSPL_NON_STD_END_KNOTS = -42,
  PRO_TK_BSPL_MULTI_INNER_KNOTS = -43,
  PRO_TK_BAD_SRF_CRV = -44,
  PRO_TK_EMPTY = -45,
  PRO_TK_BAD_DIM_ATTACH = -46,
  PRO_TK_NOT_DISPLAYED = -47,
  PRO_TK_CANT_MODIFY = -48,
  PRO_TK_CHECKOUT_CONFLICT = -49,
  PRO_TK_CRE_VIEW_BAD_SHEET = -50,
  PRO_TK_CRE_VIEW_BAD_MODEL = -51,
  PRO_TK_CRE_VIEW_BAD_PARENT = -52,
  PRO_TK_CRE_VIEW_BAD_TYPE = -53,
  PRO_TK_CRE_VIEW_BAD_EXPLODE = -54,
  PRO_TK_UNATTACHED_FEATS = -55,
  PRO_TK_REGEN_AGAIN = -56,
  PRO_TK_DWGCREATE_ERRORS = -57,
  PRO_TK_UNSUPPORTED = -58,
  PRO_TK_NO_PERMISSION = -59,
  PRO_TK_AUTHENTICATION_FAILURE = -60,
  PRO_TK_OUTDATED = -61,
  PRO_TK_INCOMPLETE = -62,
  PRO_TK_CHECK_OMITTED = -63,
  PRO_TK_MAX_LIMIT_REACHED = -64,
  PRO_TK_OUT_OF_RANGE = -65,
  PRO_TK_CHECK_LAST_ERROR = -66,
  // Ajoutée par PTC pour l'absence de licence PTC Mechanical Design I/II.
  PRO_TK_NO_PLM_LICENSE = -67,
  PRO_TK_INCOMPLETE_TESS = -68,
  PRO_TK_MULTIBODY_UNSUPPORTED = -69,
  PRO_TK_BROWSER_UNAVAILABLE = -70,
  PRO_TK_DLL_LOAD_ERROR = -71,

  // -72 à -87 : réservés par PTC (non utilisés, pas de trou à combler ici).

  // -88 à -100 : réservés à l'API Creo TOOLKIT elle-même ; une application
  // ne devrait jamais retourner ces codes.
  PRO_TK_APP_CREO_BARRED = -88,
  PRO_TK_APP_TOO_OLD = -89,
  PRO_TK_APP_BAD_DATAPATH = -90,
  PRO_TK_APP_BAD_ENCODING = -91,
  PRO_TK_APP_NO_LICENSE = -92,
  PRO_TK_APP_XS_CALLBACKS = -93,
  PRO_TK_APP_STARTUP_FAIL = -94,
  PRO_TK_APP_INIT_FAIL = -95,
  PRO_TK_APP_VERSION_MISMATCH = -96,
  PRO_TK_APP_COMM_FAILURE = -97,
  PRO_TK_APP_NEW_VERSION = -98,
  PRO_TK_APP_UNLOCK = -99,
  PRO_TK_APP_JLINK_NOT_ALLOWED = -100,
};

// Le SDK PTC expose les deux noms pour le même type.
using ProErr = ProError;

} // namespace creo::detail::shim
