#pragma once
// -----------------------------------------------------------------------
// MINIMAL substitutes for the base ProTOOLKIT types, used only when the
// real Creo SDK is not available on the build machine (a development
// workstation without Creo installed, CI, etc.). This lets the wrapper's
// logic compile and be tested independently of the PTC SDK's presence.
//
// The sizes below are the official PTC constants for Creo Parametric
// 10.0 (confirmed by the user from the real headers): PRO_LINE_SIZE,
// PRO_PATH_SIZE, PRO_COMMENT_SIZE, PRO_VALUE_SIZE, PRO_MDLNAME_SIZE,
// PRO_NAME_SIZE, PRO_TYPE_SIZE, PRO_EXTENSION_SIZE, PRO_MDLEXTENSION_SIZE,
// PRO_VERSION_SIZE, PRO_MAX_ASSEM_LEVEL and PRO_FEATREF_KEY_SIZE. The
// rest of this file only mimics the shape of the types (text buffers,
// opaque handle): as soon as a real Creo 10 SDK is detected (see
// creo/detail/ProtoolkitCompat.hpp and cmake/FindProToolkit.cmake), this
// file is no longer included and the real PTC headers take over
// automatically.
// -----------------------------------------------------------------------

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace creo::detail::shim {

// --- "Atomic" sizes (official PTC values, Creo 10) ------------------------

constexpr int kLineSize = 81;
constexpr int kPathSize = 260;
constexpr int kCommentSize = 256;
constexpr int kValueSize = 256;

constexpr int kMdlNameSize = 180; // Creo Parametric model name (ProMdl).
constexpr int kNameSize = 32;     // Any other Creo Parametric name.
constexpr int kTypeSize = 4;      // "prt", "asm", "drw", etc. + terminator.
constexpr int kExtensionSize = 4; // 3 characters + NULL terminator.
constexpr int kMdlExtensionSize = 32;
constexpr int kVersionSize = 4;
constexpr int kMaxAssemLevel = 25; // Not a buffer size: max number of
                                    // assembly nesting levels.
constexpr int kFeatRefKeySize = 81;
// PRO_MACRO_SIZE: kept by PTC for application compatibility only,
// ProMacroLoad() is no longer limited by this size.
constexpr int kMacroSize = 256;

// --- Composite sizes (same formulas as the PTC macros) --------------------

// "name.ext.#"
constexpr int kFileMdlNameSize = kMdlNameSize + kMdlExtensionSize + kVersionSize;
constexpr int kFileNameSize = kNameSize + kExtensionSize + kVersionSize;

constexpr int kFamTabFieldNameSize = kPathSize;

// "instance[generic]"
constexpr int kFamilyMdlNameSize = kMdlNameSize + kMdlNameSize + 2;
constexpr int kFamilyNameSize = kNameSize + kNameSize + 2;

// PRO_VALUE_UNUSED: the "value not used"/"end of array" sentinel used in
// many ProTOOLKIT signatures (already seen in ProArray.h: "if index < 0
// (PRO_VALUE_UNUSED), append at the end of the array"). Value confirmed
// by the user from the real header (`#define PRO_VALUE_UNUSED (-1)`).
constexpr int kValueUnused = -1;

// PRO_VALUE_DEFAULT: the "default value" sentinel (distinct from
// PRO_VALUE_UNUSED, do not confuse the two despite the similar names).
// Value confirmed by the user from the real header
// (`#define PRO_VALUE_DEFAULT (-5)`).
constexpr int kValueDefault = -5;

// Faithful reproduction of `ProBooleans` (ProToolkit.h): the ProTOOLKIT
// boolean, distinct from C++ bool even though its two values numerically
// coincide with false/true — many ProTOOLKIT functions take/return
// precisely this type (not a C++ bool, which has no standardized binary
// representation in C). The PTC SDK exposes both names ProBoolean/ProBool
// for the same type. See creo::ToBool()/ToProBoolean() (Types.hpp) to
// convert to/from a C++ bool without writing an explicit comparison on
// every call.
enum ProBooleans { PRO_B_FALSE = 0, PRO_B_TRUE = 1 };
using ProBoolean = ProBooleans;
using ProBool = ProBooleans;

// Creo model handle (ProMdl): an opaque pointer, never dereferenced by
// calling code. Only ProTOOLKIT knows the structure it points to; this
// just preserves the "distinct opaque pointer" semantics.
struct ProMdlOpaque;
using ProMdl = ProMdlOpaque *;

// ProTOOLKIT function return code: faithful reproduction of the official
// `ProError`/`ProErr` enum (ProError.h, Creo 10 — "most commonly used
// Creo Parametric TOOLKIT error statuses" per PTC). Useful in shim mode
// to simulate a specific error code in tests, without having the real
// SDK installed.
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
  // Added by PTC for a missing PTC Mechanical Design I/II license.
  PRO_TK_NO_PLM_LICENSE = -67,
  PRO_TK_INCOMPLETE_TESS = -68,
  PRO_TK_MULTIBODY_UNSUPPORTED = -69,
  PRO_TK_BROWSER_UNAVAILABLE = -70,
  PRO_TK_DLL_LOAD_ERROR = -71,

  // -72 to -87: reserved by PTC (unused, no gap to fill here).

  // -88 to -100: reserved for the Creo TOOLKIT API itself; an
  // application should never return these codes.
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

// The PTC SDK exposes both names for the same type.
using ProErr = ProError;

// Reproduces the signature of ProMdlMdlnameGet (successor of the
// now-deprecated ProMdlNameGet; note the lowercase "n" in "Mdlname" --
// confirmed by the user from two independent PTC references, consistent
// with ProMenufileName/ProMenubuttonName elsewhere in this file, which
// already showed PTC does not always capitalize compound-word
// boundaries), so that ModelHandle::Name() (ModelHandle.hpp) compiles
// in shim mode. Without a real Creo session, there is nothing meaningful
// to return: a valid ModelHandle cannot exist outside the real SDK
// anyway (no shim function ever produces a non-null ProMdl); so this
// stub always fails rather than inventing a model name.
inline ProError ProMdlMdlnameGet(ProMdl model, wchar_t *name_out) {
  if (model == nullptr || name_out == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProMdlCurrentGet, for
// ModelHandle::GetCurrent() (ModelHandle.hpp) to compile in shim mode.
// There is never a "current model" without a real Creo session, so this
// always fails too, for the same reason as the stub above.
inline ProError ProMdlCurrentGet(ProMdl *p_mdl) {
  if (p_mdl == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProMdlActiveGet (ProAssembly.h), for
// ModelHandle::GetActive() to compile in shim mode. Same rationale as
// ProMdlCurrentGet's stub above: no real session, no active model
// either (whatever the precise PTC distinction between "current" and
// "active" turns out to be for a given session).
inline ProError ProMdlActiveGet(ProMdl *p_mdl) {
  if (p_mdl == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProMdlExtensionGet (ProAssembly.h), for
// ModelHandle::Extension() to compile in shim mode.
inline ProError ProMdlExtensionGet(ProMdl model, wchar_t *ext_out) {
  if (model == nullptr || ext_out == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProMdlDirectoryPathGet (ProAssembly.h),
// for ModelHandle::DirectoryPath() to compile in shim mode.
inline ProError ProMdlDirectoryPathGet(ProMdl model, wchar_t *dir_path_out) {
  if (model == nullptr || dir_path_out == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProMdlDisplay (ProAssembly.h), for
// ModelHandle::Display() to compile in shim mode. There is no window to
// display anything in without a real session.
inline ProError ProMdlDisplay(ProMdl model) {
  if (model == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProMdlWindowGet (ProAssembly.h), for
// ModelHandle::WindowId() to compile in shim mode.
inline ProError ProMdlWindowGet(ProMdl model, int *window_id) {
  if (model == nullptr || window_id == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Faithful reproduction of `ProType` (struct pro_obj_types,
// ProObjects.h, Creo 10): the broad Creo database object type — models
// (PRO_PART, PRO_ASSEMBLY, PRO_DRAWING, PRO_MFG, ...) are only a small
// part of it, alongside features, curves, simulation/mesh/animation
// entities, etc. Provided by the user from the real header.
//
// PRO_TYPE_UNUSED (= PRO_VALUE_UNUSED in the real enum) was initially
// omitted here, since PRO_VALUE_UNUSED had not been provided yet and its
// value was not to be guessed; it has since been confirmed as -1 (see
// kValueUnused above, and creo::ValueUnused), so it is included below.
enum ProType : int {
  PRO_TYPE_UNUSED = kValueUnused,
  PRO_TYPE_DIR = -5,
  PRO_TYPE_INVALID = -2,
  PRO_ASSEMBLY = 1,
  PRO_PART = 2,
  PRO_FEATURE = 3,
  PRO_DRAWING = 4,
  PRO_SURFACE = 5,
  PRO_EDGE = 6,
  PRO_3DSECTION = 7,
  PRO_DIMENSION = 8,
  PRO_2DSECTION = 11,
  PRO_PAT_MEMBER = 12,
  PRO_PAT_LEADER = 13,
  PRO_XSEC = 18,
  PRO_LAYOUT = 19,
  PRO_AXIS = 21,
  PRO_CSYS = 25,
  PRO_REF_DIMENSION = 28,
  PRO_GTOL = 32,
  PRO_DWGFORM = 33,
  PRO_SUB_ASSEMBLY = 34,
  PRO_MFG = 37,
  PRO_SURF_FIN = 42,
  PRO_QUILT = 57,
  PRO_DATUM_TARGET = 61,
  PRO_CURVE = 62,
  PRO_POINT = 66,
  PRO_NOTE = 68,
  PRO_IPAR_NOTE = 69,
  PRO_EDGE_START = 71,
  PRO_EDGE_END = 72,
  PRO_CRV_START = 74,
  PRO_CRV_END = 75,
  PRO_SYMBOL_INSTANCE = 76,
  PRO_DRAFT_ENTITY = 77,
  PRO_DRAFT_DATUM = 79,
  PRO_DRAFT_GROUP = 83,
  PRO_DRAW_TABLE = 84,
  PRO_TABLE = 84,
  PRO_COSMETIC = 90,
  PRO_VIEW = 92,
  PRO_CABLE = 96,
  PRO_BODY = 98,
  PRO_REPORT = 105,
  PRO_MARKUP = 116,
  PRO_LAYER = 117,
  PRO_DIAGRAM = 121,
  PRO_SKETCH_ENTITY = 133,
  PRO_DATUM_TEXT = 144,
  PRO_ENTITY_TEXT = 145,
  PRO_DRAW_TABLE_CELL = 147,
  PRO_PIPE_SEG = 174,
  PRO_DATUM_PLANE = 176,
  PRO_COMP_CRV = 180,
  PRO_BND_TABLE = 211,
  PRO_ANNOTATION_ELEM = 219,
  PRO_SET_DATUM_TAG = 220,
  PRO_ANNOT_ELEM_DRIVING_DIM = 224,
  PRO_PARAMETER = 240,
  PRO_SILH_EDGE = 256,
  PRO_SILH_EDGE_MAX = 299,
  PRO_DIAGRAM_OBJECT = 305,
  PRO_DIAGRAM_WIRE = 308,
  PRO_SIMP_REP = 309,
  PRO_CE_DRAWING = 315,
  PRO_CE_SOLID = 316,
  PRO_DRW_SOLID = 319,
  PRO_WELD_PARAMS = 371,
  PRO_SNAP_LINE = 377,
  PRO_EXTOBJ = 385,
  PRO_HYBRID_BODY = 390,
  PRO_CSYS_AXIS_X = 407,
  PRO_CSYS_AXIS_Y = 408,
  PRO_CSYS_AXIS_Z = 409,
  PRO_STYLE_STATE = 460,
  PRO_VIS_STATE = 461,
  PRO_COLOR_STATE = 462,
  PRO_COMBINED_STATE = 463,
  PRO_REFONLY_STATE = 464,
  PRO_LAYER_STATE = 465,
  PRO_SUBSET_STATE = 466,
  PRO_APPEARANCE_STATE = 467,
  PRO_EXPLD_STATE = 500,
  PRO_CABLE_LOCATION = 504,
  PRO_RELSET = 533,
  PRO_ANALYSIS = 555,
  PRO_SURF_CRV = 556,
  PRO_SOLID_GEOMETRY = 622,
  PRO_LOG_SRF = 625,
  PRO_LOG_EDG = 626,
  PRO_DESKTOP = 627,
  PRO_SYMBOL_DEFINITION = 628,
  PRO_FACET_SET = 630,
  PRO_LOG_OBJECT = 640,
  PRO_NEUTRAL_LAYER = 641,
  PRO_IC_START = 642,
  PRO_IC_END = 643,
  PRO_EDGE_PNT = 660,
  PRO_CRV_PNT = 661,
  PRO_EDGE_END_PNT = 662,
  PRO_ECAD_CONDUCTOR = 704,
  PRO_CC_ASSEMBLY = 737,
  PRO_CC_PART = 740,
  PRO_NC_STEP_MODEL = 804,
  PRO_NC_STEP_OBJECT = 805,
  PRO_CATIA_MODEL = 819,
  PRO_ANNOT_PLANE = 849,
  PRO_CUSTOM_ANNOTATION = 850,
  PRO_UG = 872,
  PRO_INVENTOR_PART = 881,
  PRO_INVENTOR_ASSEM = 882,
  PRO_SW_PART = 886,
  PRO_SW_ASSEM = 890,
  PRO_TOOL_MOTION = 907,
  PRO_LOG_PNT = 913,
  PRO_LOG_PLANE = 914,
  PRO_LOG_CSYS = 915,
  PRO_LOG_AXIS = 916,
  PRO_SURFACE_PNT = 919,
  PRO_SURF_REGION_SIDE1 = 933,
  PRO_SURF_REGION_SIDE2 = 934,
  PRO_SRF_PLANE_PNT = 935,
  PRO_CRV_SIDE1SRF_CNTR = 937,
  PRO_CRV_SIDE2SRF_CNTR = 938,
  PRO_SKETCH_CONSTRAINT = 942,
  PRO_MODEL_BODIES = 974,
  PRO_UDG = 975, // internal use
  PRO_CMPST_PLY_DEF = 976,
  PRO_CMPST_PLY_ORDER = 977,
  PRO_CMPST_PLY_PNT = 978,
  // The following types do not correspond to real Pro/E database
  // objects, per PTC's original comment.
  PRO_CONTOUR = 1000,
  PRO_GROUP = 1001,
  PRO_UDF = 1002,
  PRO_FAMILY_TABLE = 1003,
  PRO_CATIA_PART = 1013,
  PRO_CATIA_PRODUCT = 1014,
  PRO_CATIA_CGR = 1015,
  PRO_AUTO_GROUP_BODIES = 4540, // internal use, custom group
  PRO_AUTO_GROUP_QUILTS = 4541, // internal use, custom group
  PRO_PATREL_FIRST_DIR = 10018,
  PRO_PATREL_SECOND_DIR = 10019,
  PRO_JAR_FILE = 10020,
  PRO_SIMULATION_LOAD = 11000,
  PRO_SIMULATION_WCS = 11001,
  PRO_SIMULATION_BEAM = 11004,
  PRO_SIMULATION_SHELL = 11005,
  PRO_SIMULATION_BEAM_SECTION = 11007,
  PRO_SIMULATION_BEAM_ORIENT = 11008,
  PRO_SIMULATION_BEAM_RELEASE = 11009,
  PRO_SIMULATION_SHELL_PROPS = 11010,
  PRO_SIMULATION_MATL_ORIENT = 11011,
  PRO_SIMULATION_SPRING = 11012,
  PRO_SIMULATION_SPRING_PROPS = 11013,
  PRO_SIMULATION_GAP = 11014,
  PRO_SIMULATION_CONTACT = PRO_SIMULATION_GAP,
  PRO_SIMULATION_MASS = 11015,
  PRO_SIMULATION_MASS_PROPS = 11016,
  PRO_SIMULATION_MESH_CNTRL = 11017,
  PRO_SIMULATION_LOAD_SET = 11018,
  PRO_SIMULATION_FUNCTION = 11019,
  PRO_SIMULATION_CONSTRAINT = 11020,
  PRO_SIMULATION_CONSTR_SET = 11021,
  PRO_SIMULATION_SHELL_PAIR = 11022,
  PRO_SIMULATION_CONNECTION = 11023,
  PRO_SIMULATION_INTERFACE = PRO_SIMULATION_CONNECTION,
  PRO_SIMULATION_WELD = 11024,
  PRO_SIMULATION_MATL_ASSIGN = 11025,
  PRO_SIMULATION_MEASURE = 11026,
  PRO_SIMULATION_RUNNER = 11027, // obsolete
  PRO_SIMULATION_ENTRANCE_PNT = 11028, // obsolete
  PRO_SIMULATION_STIFF_COND = 11029,
  PRO_SIMULATION_RIGID_LINK = 11030,
  PRO_SIMULATION_WEIGHT_LINK = 11033,
  PRO_SIMULATION_BOLT = 11035,
  PRO_SIMULATION_CONT_REGION = 11036, // obsolete
  PRO_SIMULATION_OBJECT = 11037,
  PRO_SIMULATION_ANALYSIS = 11038,
  PRO_SIMULATION_CRACK = 11039,
  PRO_SIMULATION_MATERIAL = 11040,
  PRO_SIMULATION_SCOPE = 11041,
  PRO_SIMULATION_INTEG_CONTACT = 11042,
  PRO_SIMULATION_INTEG_CONTACTPROP = 11043,
  PRO_SIMULATION_JOINT = 11044,
  PRO_SIMULATION_JOINT_BEHAV = 11045,
  PRO_SIMULATION_VOLUME = 11200,
  PRO_SIMULATION_PNT_PATTERN = 11201,
  PRO_SIMULATION_FEAT_SEC = 11202,
  PRO_SIMULATION_HP_FACE = 11203,
  PRO_SIMULATION_HP_EDGE = 11204,
  PRO_SIMULATION_HPE_BUNDLE = 11205,
  PRO_SIMULATION_3D_NOTE = 11206,
  PRO_SIMP_3D_LATTICE_ENT = 11207,
  PRO_SIMULATION_USER_STUDY = 11208, // internal use
  PRO_SIMULATION_LOAD_CASE = 11209, // internal use
  PRO_TOPOLOGYOPT_TOPO_REGION = 11501,
  PRO_TOPOLOGYOPT_DESIGN_OBJ = 11502,
  PRO_TOPOLOGYOPT_DESIGN_CONSTR = 11503,
  PRO_TOPOLOGYOPT_STUDY = 11504,
  PRO_MESH_MESH = 11999,
  PRO_MESH_COMPONENT = 12000,
  PRO_MESH_SURFACE = 12002,
  PRO_MESH_EDGE = 12004,
  PRO_MESH_CURVE = 12005,
  PRO_MESH_VERTEX = 12006,
  PRO_MESH_HARD_POINT = 12008,
  PRO_MESH_NODE = 12020,
  PRO_MESH_ELEMENT = 12021,
  PRO_FEM_NEUTRAL_FILE = 414,
  PRO_FEM_TM_FILE = 12900,
  PRO_MECH_DIR = 12905,
  PRO_DISPOBJ = 13000,
  PRO_RP_MATERIAL = 17001,
  PRO_RP_FUNCTION = 17002,
  PRO_RP_MATERIAL_SET = 17003, // internal use only
  PRO_SKETCH_COSMETIC = 20423,
  PRO_SEDGE_PART = 20253,
  PRO_SEDGE_ASSEMBLY = 20255,
  PRO_SEDGE_SHEETMETAL = 20256,
  PRO_QUERY = 45106,
  PRO_ARTWORK = 55100, // PI Artwork
  PRO_TRY_OUT_HDR = 60084,
  PRO_MDO_BODY = 70000,
  PRO_MDO_CAM_CONN = 70003,
  PRO_MDO_GEAR_CONN = 70004,
  PRO_MDO_SERVO_MOTOR = 70005,
  PRO_MDO_FORCE_MOTOR = 70006,
  PRO_MDO_SPRING = 70007,
  PRO_MDO_DAMPER = 70008,
  PRO_MDO_FORCE = 70009,
  PRO_MDO_TORQUE = 70010,
  PRO_MDO_ANALYSIS = 70011,
  PRO_MDO_SNAPSHOT = 70012,
  PRO_MDO_INIT_COND = 70013,
  PRO_MDO_MEASURE = 70014,
  PRO_MDO_JAS = 70015,
  PRO_MDO_GRAVITY = 70016,
  PRO_MDO_MASSPROP = 70017,
  PRO_MDO_SETTINGS = 70018,
  PRO_MDO_CONN = 70019,
  PRO_MDO_CONN_AXIS = 70020, // obsolete
  PRO_MDO_SLOT_CONN = 70021,
  PRO_MDO_CONN_PARAM = 70022,
  PRO_MDO_LOAD_XFER = 70023,
  PRO_MDO_SLOT_AXIS = 70024,
  PRO_MDO_CONN_AXIS_TR_1 = 70025,
  PRO_MDO_CONN_AXIS_TR_2 = 70026,
  PRO_MDO_CONN_AXIS_TR_3 = 70027,
  PRO_MDO_CONN_AXIS_ROT_1 = 70028,
  PRO_MDO_CONN_AXIS_ROT_2 = 70029,
  PRO_MDO_CONN_AXIS_ROT_3 = 70030,
  PRO_MDO_CONN_AXIS_EXT = 70031,
  PRO_MDO_CONTACT_3D = 70032,
  PRO_MDO_BELT = 70033,
  PRO_MDO_BUSHING_LD = 70034,
  PRO_MDO_CONN_AXIS_CONE = 70035,
  PRO_MDO_TERM_COND = 70036,
  PRO_PLACEMENT_SET = 70100,
  PRO_COLSN_DATA = 71000,
  PRO_ANIM_ANIMATION = 73000,
  PRO_ANIM_SUB_ANIMATION = 73001,
  PRO_ANIM_BODY = 73002,
  PRO_ANIM_BODY_LOCK = 73003,
  PRO_ANIM_CONN_STATUS = 73004,
  PRO_ANIM_EVENT = 73005,
  PRO_ANIM_DRIVER_INSTANCE = 73006,
  PRO_ANIM_KFS = 73007,
  PRO_ANIM_KFS_INSTANCE = 73008,
  PRO_ANIM_VIEW_AT_TIME = 73009,
  PRO_ANIM_DISPLAY_AT_TIME = 73010,
  PRO_ANIM_TRANS_AT_TIME = 73011,
  PRO_ANIM_COMB = 73012,
  PRO_ANIM_PI_KFS_INSTANCE = 73013, // obsolete
  PRO_ANIM_PI_INT_PT = 73014, // obsolete
  PRO_ANIM_PI_INT_AXIS = 73015, // obsolete
  PRO_ANIM_PI_INT_PLANE = 73016, // obsolete
  PRO_ANIM_EXPLD_KFS = 73017,
  PRO_ANIM_EXPLD_KFS_INSTANCE = 73018,
  PRO_ANIM_EXPLD_SUB_ANIMATION = 73019,
  PRO_ANIM_EXPLD_EVENT = 73020,
  PRO_ANIM_MDO_EVENT = 73021,
  PRO_ANIM_MDO_MOVIE = 73022,
  PRO_ANIM_EXPLD_COMB = 73023,
  PRO_ANIM_SNAP_COMB_KFS = 73024,
  PRO_ANIM_SNAP_COMB_KFS_INSTANCE = 73025,
  PRO_ANIM_EXPLD_COMB_KFS = 73026,
  PRO_ANIM_EXPLD_COMB_KFS_INSTANCE = 73027,
  PRO_LOG_CURVE = 74150,
  PRO_LOG_COLLECTION = 74151,
  PRO_LAYOUT_TAG = 74152, // internal use
  PRO_LAYOUT_NODE = 74153, // internal use
  PRO_LAYOUT_WP = 74154, // internal use
  PRO_DTM_CHK_PNT = 74266, // internal use
  PRO_PSEG_START = 74275,
  PRO_PSEG_END = 74276,
  PRO_QUILT_CONTOUR = 74287,
  PRO_SENSOR = 74288, // internal use
  PRO_ECAD_CUT = 74290,
  PRO_RP_MANIKIN_SET = 74345,
  PRO_ASM_LOG_SRF = 74360,
};

// Faithful reproduction of `ProMdlType` (ProMdl.h, Creo 10), as given by
// the user: a narrower classification than `ProType` above, covering
// only the "model-shaped" object types that ProMdlTypeGet can return
// (assembly, part, drawing, ...). Each enumerator reuses the exact same
// integer value as the corresponding ProType constant (e.g.
// PRO_MDL_ASSEMBLY == PRO_ASSEMBLY) -- this is how PTC itself defines
// it, not a coincidence this wrapper is relying on. See creo::MdlType
// (ObjectType.hpp) for the C++ side.
enum ProMdlType : int {
  PRO_MDL_UNUSED = PRO_TYPE_UNUSED,
  PRO_MDL_ASSEMBLY = PRO_ASSEMBLY,
  PRO_MDL_PART = PRO_PART,
  PRO_MDL_DRAWING = PRO_DRAWING,
  PRO_MDL_3DSECTION = PRO_3DSECTION,
  PRO_MDL_2DSECTION = PRO_2DSECTION,
  // (*.lay file) Notebook model. Formerly known as Layout model.
  PRO_MDL_LAYOUT = PRO_LAYOUT,
  PRO_MDL_DWGFORM = PRO_DWGFORM,
  PRO_MDL_MFG = PRO_MFG,
  PRO_MDL_REPORT = PRO_REPORT,
  PRO_MDL_MARKUP = PRO_MARKUP,
  PRO_MDL_DIAGRAM = PRO_DIAGRAM,
  // Read-only per PTC: passing this back to Creo may cause unpredictable
  // behavior.
  PRO_MDL_CE_SOLID = PRO_CE_SOLID,
  PRO_MDL_CE_DRAWING = PRO_CE_DRAWING, // reserved for internal use
  PRO_MDL_DRW_SOLID = PRO_DRW_SOLID,   // reserved for internal use
};

// Reproduces the signature of ProMdlTypeGet, for ModelHandle::Type()
// (ModelHandle.hpp) to compile in shim mode. Honors the documented
// contract given by the user precisely: "if the function fails, [the
// out-param] is set to PRO_TYPE_UNUSED" -- for every failure path here,
// not just some of them.
inline ProError ProMdlTypeGet(ProMdl model, ProMdlType *p_type) {
  if (p_type == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  *p_type = static_cast<ProMdlType>(PRO_TYPE_UNUSED);
  if (model == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Faithful reproduction of `pro_model_item` (ProObjects.h, Creo 10):
// PTC gives this exact same 3-field struct roughly thirty different
// typedef names — ProGeomitem, ProFeature, ProDimension, ProNote,
// ProLayer, ... — one per kind of database object, even though they are
// bit-for-bit identical at the C level. See creo::ModelItem (Types.hpp)
// and its aliases (GeomItem, Feature, Dimension, ...) for the C++ side of
// this. Provided by the user from the real header.
struct ProModelitem {
  ProType type;
  int id;
  ProMdl owner;
};
using ProGeomitem = ProModelitem;
using ProExtobj = ProModelitem;
using ProFeature = ProModelitem;
using ProProcstep = ProModelitem;
using ProSimprep = ProModelitem;
using ProExpldstate = ProModelitem;
using ProLayer = ProModelitem;
using ProDimension = ProModelitem;
using ProDtlnote = ProModelitem;
using ProDtlsyminst = ProModelitem;
using ProGtol = ProModelitem;
using ProCompdisp = ProModelitem;
using ProDwgtable = ProModelitem;
using ProNote = ProModelitem;
using ProAnnotationElem = ProModelitem;
using ProAnnotation = ProModelitem;
using ProAnnotationPlane = ProModelitem;
using ProSymbol = ProModelitem;
using ProSurfFinish = ProModelitem;
using ProMechItem = ProModelitem;
using ProMaterialItem = ProModelitem;
using ProCombstate = ProModelitem;
using ProLayerstate = ProModelitem;
using ProApprnstate = ProModelitem;
using ProSolidBody = ProModelitem;
using ProPly = ProModelitem;
using ProTable = ProModelitem;

// Reproduces the signature of ProModelitemNameGet, for
// creo::ModelItem::Name() (Types.hpp) to compile in shim mode. Unlike
// Type()/Id()/Owner() (plain field reads, valid on any ProModelitem value
// including one built by hand in a test), a name lookup genuinely needs a
// real model database: this stub always fails rather than inventing a
// name, exactly like ProMdlMdlnameGet's stub above.
inline ProError ProModelitemNameGet(ProModelitem *p_handle,
                                     wchar_t *name_out) {
  if (p_handle == nullptr || name_out == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// Reproduces the signature of ProFeatureRegenerate, for
// creo::Feature::Regenerate() (Types.hpp) to compile in shim mode. Signature
// as given by the user (a description of the API's usual shape, not a
// pasted header), not independently verified: `solid` is taken as a plain
// ProMdl here (the user's own description treats ProSolid/ProMdl as
// interchangeable) — if a real SDK's ProSolid turns out to be a genuinely
// distinct type, the real-SDK branch in ProtoolkitCompat.hpp will fail to
// compile there, loudly, rather than silently doing the wrong thing. Like
// the other "needs a real session" stubs above, this always fails.
inline ProError ProFeatureRegenerate(ProMdl solid, ProFeature *feature) {
  if (solid == nullptr || feature == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  return PRO_TK_NOT_IMPLEMENTED;
}

// -----------------------------------------------------------------------
// ProArray (ProArray.h, Creo 10): PTC's generic dynamic array, a plain
// opaque `void*` on the API side. Unlike ProMdl/ProError above (simple
// substitute types, never algorithmically exercised in shim mode),
// ProArray has real behavior (allocation, growth, insertion/removal)
// that creo::Array<T> (see creo/Array.hpp) actually exercises: this
// reproduction must therefore be a functional implementation, not just a
// shape declaration.
//
// Implementation: a hidden header right before the data (the classic
// "stretchy buffer" technique), so that the ProArray pointer handed back
// to the caller points directly at the data — reproducing the documented
// PTC behavior where a ProArray can be cast directly to T* for
// contiguous read access.
// -----------------------------------------------------------------------

using ProArray = void *;

struct ProArrayHeader {
  int size;
  int capacity;
  int obj_size;
  int reallocation_size;
};

inline ProArrayHeader *ProArrayHeaderOf(ProArray array) {
  return reinterpret_cast<ProArrayHeader *>(static_cast<char *>(array) -
                                             sizeof(ProArrayHeader));
}

inline ProError ProArrayMaxCountGet(int obj_size, int *max_num_objs) {
  if (obj_size <= 0 || max_num_objs == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  // Reproduces the order of magnitude documented by PTC ("about 2 MB"),
  // not a real system limit: purely indicative in shim mode.
  *max_num_objs = (2 * 1024 * 1024) / obj_size;
  return PRO_TK_NO_ERROR;
}

inline ProError ProArrayAlloc(int n_objs, int obj_size, int reallocation_size,
                               ProArray *p_array) {
  if (n_objs < 0 || obj_size <= 0 || p_array == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  auto unsigned_obj_size = static_cast<std::size_t>(obj_size);
  std::size_t total = sizeof(ProArrayHeader) +
                       static_cast<std::size_t>(n_objs) * unsigned_obj_size;
  void *mem = std::malloc(total);
  if (mem == nullptr) {
    return PRO_TK_OUT_OF_MEMORY;
  }
  auto *header = static_cast<ProArrayHeader *>(mem);
  header->size = n_objs;
  header->capacity = n_objs;
  header->obj_size = obj_size;
  header->reallocation_size = reallocation_size > 0 ? reallocation_size : 1;
  void *data = static_cast<char *>(mem) + sizeof(ProArrayHeader);
  if (n_objs > 0) {
    std::memset(data, 0, static_cast<std::size_t>(n_objs) * unsigned_obj_size);
  }
  *p_array = data;
  return PRO_TK_NO_ERROR;
}

inline ProError ProArrayFree(ProArray *p_array) {
  if (p_array == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  if (*p_array != nullptr) {
    std::free(ProArrayHeaderOf(*p_array));
  }
  *p_array = nullptr;
  return PRO_TK_NO_ERROR;
}

inline ProError ProArraySizeGet(ProArray array, int *p_size) {
  if (array == nullptr || p_size == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  *p_size = ProArrayHeaderOf(array)->size;
  return PRO_TK_NO_ERROR;
}

inline ProError ProArrayEnsureCapacity(ProArray *p_array,
                                        int needed_capacity) {
  ProArrayHeader *header = ProArrayHeaderOf(*p_array);
  if (needed_capacity <= header->capacity) {
    return PRO_TK_NO_ERROR;
  }
  int new_capacity = header->capacity;
  while (new_capacity < needed_capacity) {
    new_capacity += header->reallocation_size;
  }
  std::size_t total =
      sizeof(ProArrayHeader) + static_cast<std::size_t>(new_capacity) *
                                    static_cast<std::size_t>(header->obj_size);
  void *mem = std::realloc(header, total);
  if (mem == nullptr) {
    return PRO_TK_OUT_OF_MEMORY;
  }
  header = static_cast<ProArrayHeader *>(mem);
  header->capacity = new_capacity;
  *p_array = static_cast<char *>(mem) + sizeof(ProArrayHeader);
  return PRO_TK_NO_ERROR;
}

inline ProError ProArraySizeSet(ProArray *p_array, int size) {
  if (p_array == nullptr || *p_array == nullptr || size < 0) {
    return PRO_TK_BAD_INPUTS;
  }
  ProError err = ProArrayEnsureCapacity(p_array, size);
  if (err != PRO_TK_NO_ERROR) {
    return err;
  }
  ProArrayHeader *header = ProArrayHeaderOf(*p_array);
  if (size > header->size) {
    auto obj_size = static_cast<std::size_t>(header->obj_size);
    std::memset(static_cast<char *>(*p_array) +
                    static_cast<std::size_t>(header->size) * obj_size,
                0, static_cast<std::size_t>(size - header->size) * obj_size);
  }
  header->size = size;
  return PRO_TK_NO_ERROR;
}

inline ProError ProArrayObjectAdd(ProArray *p_array, int index, int n_objects,
                                   void *p_object) {
  if (p_array == nullptr || *p_array == nullptr || n_objects <= 0) {
    return PRO_TK_BAD_INPUTS;
  }
  ProArrayHeader *header = ProArrayHeaderOf(*p_array);
  int old_size = header->size;
  int insert_at = (index < 0) ? old_size : index;
  if (insert_at > old_size) {
    return PRO_TK_BAD_INPUTS;
  }
  int new_size = old_size + n_objects;
  ProError err = ProArrayEnsureCapacity(p_array, new_size);
  if (err != PRO_TK_NO_ERROR) {
    return err;
  }
  header = ProArrayHeaderOf(*p_array); // realloc may have moved the block.
  char *base = static_cast<char *>(*p_array);
  std::size_t obj_size = static_cast<std::size_t>(header->obj_size);
  if (insert_at < old_size) {
    std::memmove(base +
                     (static_cast<std::size_t>(insert_at) +
                      static_cast<std::size_t>(n_objects)) *
                         obj_size,
                 base + static_cast<std::size_t>(insert_at) * obj_size,
                 static_cast<std::size_t>(old_size - insert_at) * obj_size);
  }
  if (p_object != nullptr) {
    std::memcpy(base + static_cast<std::size_t>(insert_at) * obj_size,
                p_object, static_cast<std::size_t>(n_objects) * obj_size);
  }
  header->size = new_size;
  return PRO_TK_NO_ERROR;
}

inline ProError ProArrayObjectRemove(ProArray *p_array, int index,
                                      int n_objects) {
  if (p_array == nullptr || *p_array == nullptr || n_objects <= 0) {
    return PRO_TK_BAD_INPUTS;
  }
  ProArrayHeader *header = ProArrayHeaderOf(*p_array);
  int old_size = header->size;
  int remove_at = (index < 0) ? (old_size - n_objects) : index;
  if (remove_at < 0 || remove_at + n_objects > old_size) {
    return PRO_TK_BAD_INPUTS;
  }
  char *base = static_cast<char *>(*p_array);
  std::size_t obj_size = static_cast<std::size_t>(header->obj_size);
  int tail_count = old_size - (remove_at + n_objects);
  if (tail_count > 0) {
    std::memmove(
        base + static_cast<std::size_t>(remove_at) * obj_size,
        base + static_cast<std::size_t>(remove_at + n_objects) * obj_size,
        static_cast<std::size_t>(tail_count) * obj_size);
  }
  header->size = old_size - n_objects;
  return PRO_TK_NO_ERROR;
}

// Reproduces the signature of ProSessionMdlList (ProAssembly.h), for
// ModelHandle::List() (ModelHandle.hpp) to compile in shim mode.
// Unlike the ProMdl*-returning stubs above, this one is genuinely
// functional rather than an always-failing stub: ProArray itself is
// fully implemented in this shim (see above), and "there are zero
// models in a session that does not exist" is an honest answer, not a
// fabricated one — so this allocates and returns a real, empty ProArray
// rather than failing outright, exactly matching PTC's own documented
// contract ("the function allocates the memory for this argument; call
// ProArrayFree() to free it").
inline ProError ProSessionMdlList(ProMdlType model_type,
                                   ProMdl **p_model_array, int *p_count) {
  (void)model_type;
  if (p_model_array == nullptr || p_count == nullptr) {
    return PRO_TK_BAD_INPUTS;
  }
  ProArray array = nullptr;
  ProError err = ProArrayAlloc(0, static_cast<int>(sizeof(ProMdl)), 1, &array);
  if (err != PRO_TK_NO_ERROR) {
    return err;
  }
  *p_model_array = static_cast<ProMdl *>(array);
  *p_count = 0;
  return PRO_TK_NO_ERROR;
}

} // namespace creo::detail::shim
