#ifndef REGS_H__
#define REGS_H__

// Include bunch of definitions local to a project.
#include "regs.local.h"

// Used for default value for declaring volatile registers in regs.local.h since they are not initialised. 
const regs_t REGS_NO_DEFAULT = 0;

// Macro that expands to an item declaration in an enum, so gen_(FUEL, 100, "Fuel gauge") ==> 'REGS_IDX_FUEL,'. 
#define REGS_IDX_DEF_GEN_ENUM_IDX(name_, default_, desc_) \
 REGS_IDX_##name_,

// Declare the indices to the registers.  
enum {
    REGS_DEFS(REGS_IDX_DEF_GEN_ENUM_IDX)
    REGS_COUNT
};

// Struct to include registers and other stuff in NV storage. 
typedef struct registers_t {
	regs_t vals[REGS_COUNT];							// Must be first in the struct as there are some non-NV regs first.
	regs_extra_t extra;
} registers_t;

// Access registers as an external.
extern registers_t g_registers;
#define REGS (g_registers.vals)
#define REGS_EXTRA (g_registers.extra)

// User function for setting default value of extra data. 
void regsExtraSetDefaults()  __attribute__((weak));

/* Initialise the driver, reads the NV regs from EEPROM with a call to regsNvRead(), sets default values if corrupt. Returns error code from 
	eeprom_driver.h EEPROM_DRIVER_READ_ERROR_xxx. DOES NOT clear volatile registers. This is up to you. */
void regsInit();

// Read the values from EEPROM, when it returns it is guaranteed to have valid data in the buffer, but it might be set to default values.
// Return values as for eepromDriverRead() so one of EEPROM_DRIVER_READ_ERROR_xxx. Status also written to register index REGS_IDX_EEPROM_RC.
uint8_t regsNvRead();

// Write EEPROM data. 
void regsNvWrite();

// Set NV regs and any extra data to default values.
void regsNvSetDefaults();

// Set just a range of registers _only_ to default values. Exposed just for the use case when a range of registers are used for something special, 
//  like a set of menu options. Note that the non-NV regs will just be set to zero, which is unlikely to be useful. 
void regsNvSetRegisterDefaults(uint8_t start, uint8_t end);

// Return flags register, this compiles to two lds instructions. 
static inline regs_t regsGetFlags() { return REGS[REGS_IDX_FLAGS]; }

// Set/clear all bits in the mask in the given register. Return true if value has changed. 
bool regsWriteMask(uint8_t idx, regs_t mask, bool s);

// Write the register at index idx bits in mask m with value val. Obviously no bits are set in v that are clear in m. Return true if value has changed. 
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t val);

// Set/clear all bits in the mask in the flags register. Return true if value has changed. 
bool regsWriteMaskFlags(regs_t mask, bool s);

// Update bits in flags register with set bits in mask m with mask value. Return true if value has changed. 
bool regsUpdateMaskFlags(regs_t mask, regs_t val);

// Return the name of the register derived from the settings.local.h file. 
const char* regsGetRegisterName(uint8_t idx);

// Return a help text derived from the settings.local.h file. 
const char* regsGetRegisterDescription(uint8_t idx);

// Return an additional help string, contents generated in the settings.local.h file. 
const char* regsGetHelpStr();

// See if the register is to be printed in hex, else decimal. Exposed in case you want to use a different printing method. 
bool regsHexFormat(uint8_t reg_idx);

/* Having a unified method of accessing the registers is really handy in combination with the console, which allows reading and setting values or printing 
	of values either by console commands or by the dump function that prints the values every so often. So we need a function to print them, which requires
	a memory heavy formatting functions. The console already has the consolePrint() function in console.h. But we don't want to force a	dependency on console. 
	So regs uses a function regsPrintRegValue() that will print the value however it likes. In case the regs_t type is compatible with the console_cell_t type, there is a config define that will use consolePrint(). */

// Print a single register value in hex or decimal. This function must either be defined explicitly _OR_ define macro CFG_WANT_CONSOLE_PRINT_FOR_REGS
// 	to define a function that uses consolePrint(). 
void regsPrintRegValue(uint8_t idx);

// Print a range of registers.
void regsPrintRegValues(uint8_t start, uint8_t end);

// Print all register values.
void regsPrintRegValuesAll();

// Print only RAM register values. Used for dumping regs to the console at intervals. 
void regsPrintRegValuesRam();

// Commands for working with nv settings. Read from NV, write to NV, load default values. 
#define REGS_CONSOLE_COMMANDS_NV																													\
	case /** NV-DEFAULT **/ 0xfcdb: regsNvSetDefaults(); break;																						\
	case /** NV-W **/ 0xa8c7: regsNvWrite(); break;																									\
	case /** NV-R **/ 0xa8c2: regsNvRead(); break;																									

// Commands v@, v! to read/write register value. Only if regs_t type is compatible with console_cell_t. 
#define REGS_CONSOLE_COMMANDS_ACCESS																												\
	case /** V! **/ 0x7472: { const uint8_t i = console_u_pop(); console_verify_bounds(i, REGS_COUNT); REGS[i] = console_u_pop(); } break;			\
	case /** V@ **/ 0x7413: { const uint8_t i = console_u_pop(); console_verify_bounds(i, REGS_COUNT); console_u_push(REGS[i]); } break;			

// Cmmands ?v, ??v to print one or all register values. Requires that regsPrintRegValue() is defined.
#define REGS_CONSOLE_COMMANDS_PRINT																													\
	case /** ?V **/ 0x688c: { const uint8_t i = console_u_pop(); console_verify_bounds(i, REGS_COUNT); regsPrintRegValue(i); } break;				\
	case /** ??V **/ 0x85d3: regsPrintRegValuesAll(); break;																						

// Command ???v to dump settings is glorious detail.
#define REGS_CONSOLE_COMMANDS_PRINT_VERBOSE																											\
	case /** ???V **/ 0x3cac: fori (REGS_COUNT) {																									\
			consolePrint(CONSOLE_PRINT_NEWLINE, 0);																									\
			consolePrint(CONSOLE_PRINT_SIGNED, i);																									\
			consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)regsGetRegisterName(i));																\
			regsPrintRegValue(i);																													\
			consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)regsGetRegisterDescription(i));														\
			wdt_reset();																															\
		}																																			\
		consolePrint(CONSOLE_PRINT_NEWLINE, 0);																										\
		consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(REGS_EXTRA_HELP_STR));																\
		break;																																		

#endif // REGS_H__
