#ifndef REGS_H__
#define REGS_H__

// Used for default value for declaring volatile registers in regs.local.h since they are not initialised. 
const uint16_t REGS_NO_DEFAULT = 0;

// Include bunch of definitions local to a project. 
#include "regs.local.h"

// Macro that expands to an item declaration in an enum, so gen_(FUEL, 100, "Fuel gauge") ==> 'REGS_IDX_FUEL,'. 
#define REGS_IDX_DEF_GEN_ENUM_IDX(name_, default_, desc_) \
 REGS_IDX_##name_,

// Declare the indices to the registers.  
enum {
    REGS_DEFS(REGS_IDX_DEF_GEN_ENUM_IDX)
    REGS_COUNT
};

// Struct to include registers in NV storage. 
typedef struct registers_t {
	uint16_t vals[REGS_COUNT];							// Must be first in the struct as there are some non-NV regs first.
} registers_t;

// Access registers as an external.
extern registers_t g_registers;
#define REGS (g_registers.vals)

/* Initialise the driver, reads the NV regs from EEPROM with a call to regsNvRead(), sets default values if corrupt. Returns error code from 
	eeprom_driver.h EEPROM_DRIVER_READ_ERROR_xxx. DOES NOT clear volatile registers. This is up to you. */
uint8_t regsInit();

// Read the values from EEPROM, when it returns it is guaranteed to have valid data in the buffer, but it might be set to default values.
// Return values as for eepromDriverRead() so one of EEPROM_DRIVER_READ_ERROR_xxx. 
uint8_t regsNvRead();

// Write EEPROM data. 
void regsNvWrite();

// Set NV regs to default values.
void regsNvSetDefaults();

// Return flags register, this compiles to two lds instructions. 
static inline uint16_t regsGetFlags() { return REGS[REGS_IDX_FLAGS]; }

// Set/clear all bits in the mask in the given register. Return true if value has changed. 
bool regsWriteMask(uint8_t idx, uint16_t mask, bool s);

// Write the register at index idx bits in mask m with value val. Obviously no bits are set in v that are clear in m. Return true if value has changed. 
bool regsUpdateMask(uint8_t idx, uint16_t mask, uint16_t val);

// Set/clear all bits in the mask in the flags register. Return true if value has changed. 
bool regsWriteMaskFlags(uint16_t mask, bool s);

// Update bits in flags register with set bits in mask m with mask value. Return true if value has changed. 
bool regsUpdateMaskFlags(uint16_t mask, uint16_t val);

// Return the name of the register derived from the regs.local.h file. 
const char* regsGetRegisterName(uint8_t idx);

// Return a help text derived from the regs.local.h file. 
const char* regsGetRegisterDescription(uint8_t idx);

// Return an additional help string, contents generated in the regs.local.h file. 
const char* regsGetHelpStr();

// Print a single register value in hex or decimal.
void regsPrintValue(uint8_t idx);

// Print a range register value in hex or decimal.
void regsPrintValues(uint8_t s_idx, uint8_t e_idx);

// Print all the register values.
void regsPrintValuesAll();

// Print just the regster values in RAM.
void regsPrintValuesRam();

// Print all the register values with lots of verbosity.
void regsPrintValuesVerbose();

#define REGS_CONSOLE_COMMANDS																																					\
	case /** V! **/ 0x7472: { const uint8_t i = console_u_pop(); if (i >= REGS_COUNT) console_raise(CONSOLE_RC_ERROR_INDEX_OUT_OF_RANGE); REGS[i] = console_u_pop(); } break;	\
	case /** V@ **/ 0x7413: { const uint8_t i = console_u_pop(); if (i >= REGS_COUNT) console_raise(CONSOLE_RC_ERROR_INDEX_OUT_OF_RANGE); console_u_push(REGS[i]); } break;		\
	case /** ?V **/ 0x688c: { const uint8_t i = console_u_pop(); if (i >= REGS_COUNT) console_raise(CONSOLE_RC_ERROR_INDEX_OUT_OF_RANGE); regsPrintValue(i); } break;			\
	case /** ??V **/ 0x85d3: regsPrintValuesAll(); break;																														\
	case /** ???V **/ 0x3cac: regsPrintValuesVerbose(); break;																													\
	case /** NV-DEFAULT **/ 0xfcdb: regsNvSetDefaults(); break;																													\
	case /** NV-W **/ 0xa8c7: regsNvWrite(); break;																																\
	case /** NV-R **/ 0xa8c2: regsNvRead(); break;																																\


#endif // REGS_H__
