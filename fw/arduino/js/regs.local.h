#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// This file is included by regs.h and then regs.h is included in most source files.
//

// Define version of regs schema. If you change this file, increment the number to force any existing EEPROM to flag as corrupt. 
#define REGS_VERSION 3

// Registers hold values of this type. It can be signed if required. 
typedef uint16_t regs_t;
static const regs_t REGS_MAXVAL = 65535;
#define REGS_VAL_SIGNED 0

// Extra NV stuff. 
typedef struct {
	// uint8_t event_trace_mask[EVENT_TRACE_MASK_SIZE];
} regs_extra_t;
void regsExtraSetDefaults(); // Defined in regs.local.cpp

// Extra NV stuff end. 

// Registers are defined with 3 parameters, name, default value, and a short comment that is turned into help text. 
#define REGS_DEFS(gen_)																														\
 gen_(FLAGS,						REGS_NO_DEFAULT,					"Various flags, see REGS_FLAGS_MASK_xxx")							\
 gen_(RESTART,						REGS_NO_DEFAULT,					"lo 8: MCUSR, hi 8: WDT mask")										\
																																			\
 /* NV regs. */																																\
 gen_(ENABLES,						0,									"Enable flags")														\
 gen_(JS_FILTER_K,				0,									"Filtering parameter applied to JS readings")					\
 gen_(MOUSE_XY_DEADBAND,			500,								"mouse deadband")													\
 gen_(MOUSE_XY_SCALE,				2000,								"mouse scale factor reading -> delta")								\
 
// Define the start of the NV regs. The region is from this index up to the end of the register array. 
#define REGS_START_NV_IDX REGS_IDX_ENABLES

// Define which regs to print in hex.
#define REGS_PRINT_HEX_MASK (_BV(REGS_IDX_FLAGS) | _BV(REGS_IDX_RESTART) | _BV(REGS_IDX_ENABLES))

// Flags are defined with 3 parameters, name, bit position and a short description that is turned into help text. 
#define REGS_FLAGS_DEFS(gen_)																							\
 /* Signal flags. */																									\
 gen_(JS_UPDATE,					0,  "JS data available")															\
 gen_(RELAY_0,						1,  "relay 0")																		\
 gen_(RELAY_1,						2,  "relay 1")																		\
 gen_(RELAY_2,						3,  "relay 2")																		\
 gen_(RELAY_3,						4,  "relay 3")																		\
																														\
 /* Debug flags. */																										\
																														\
 /* Fault flags, assigned down from bit 15. */																			\
 gen_(EEPROM_CORRUPT_BANK_0,		14,	"EEPROM bank 0 corrupt")														\
 gen_(EEPROM_CORRUPT_BANK_1,		15,	"EEPROM bank 1 corrupt")														\

// Enables are defined with 3 parameters, name, bit position and a short description that is turned into help text. 
#define REGS_ENABLES_DEFS(gen_)																							\
 gen_(DUMP_REGS,			0,	"Regs values dump to console")															\
 gen_(DUMP_REGS_FAST,		1,	"Dump at 5/s rather than 1/s")															\
 gen_(DUMP_JS,				2,	"Dump JS readings")																	\
 gen_(MOUSE_EMULATION,		3,	"Enable mouse emulation")																\
 gen_(AXIS_INVERT_X,		4,	"Invert x axis")																		\
 gen_(AXIS_INVERT_Y,		5,	"Invert y axis")																		\
 gen_(AXIS_INVERT_Z,		6,	"Invert z axis")																		\

// Generate FLAGS defs.
//

// Macro that generates enum items intialised to a mask for a set of bit flags, SO gen_(FLAGS, 99,	"Stuff") => 'REGS_FLAGS_MASK_FLAGS = (int)_BV(99),'. 
#define REGS_FLAGS_GEN_ENUM_MASK(name_, bit_, desc_) \
 REGS_FLAGS_MASK_##name_ = (int)_BV(bit_),

// Declare the flags enum.
enum {
	REGS_FLAGS_DEFS(REGS_FLAGS_GEN_ENUM_MASK)
};

// Macro that expands to a help string for the flag or enable, so gen_(FLAGS, 0,  "Stuff") =>  ==> "\r\n FLAGS: 99: ."
#define REGS_GENERIC_MASK_GEN_HELP_STR(name_, bit_, desc_) \
 "\r\n " #name_ ": " #bit_ " (" desc_ ".)"

// Generate ENABLES defs.
//

// Macro that generates enum items intialised to a mask for a set of bit flags, SO gen_(FLAGS, 99,	"Stuff") => 'REGS_ENABLES_MASK_FLAGS = (int)_BV(99),'. 
#define REGS_ENABLES_GEN_ENUM_MASK(name_, bit_, desc_) \
 REGS_ENABLES_MASK_##name_ = (int)_BV(bit_),

// Declare the enables enum.
enum {
	REGS_ENABLES_DEFS(REGS_ENABLES_GEN_ENUM_MASK)
};

// Generate help string for flags & enables.
#define REGS_EXTRA_HELP_STR "\r\nFlags:" REGS_FLAGS_DEFS(REGS_GENERIC_MASK_GEN_HELP_STR) "\r\nEnables:" REGS_ENABLES_DEFS(REGS_GENERIC_MASK_GEN_HELP_STR)

#endif // REGS_LOCAL_H__


