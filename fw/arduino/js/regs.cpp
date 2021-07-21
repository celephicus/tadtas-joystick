#ifdef ARDUINO
# include <Arduino.h>
#else
# include <stdint.h>
# include <stdbool.h>
#endif

#include "project_config.h"			// Global config file. 

#include "debug.h"					// Asserts etc. 
FILENUM(208);  						// All source files in common have file numbers starting at 200. 

#include "utils.h"					// Handy utilities.
#include "eeprom_driver.h"			// Contains all the EEPROM handler code. 
#include "regs.h"

// Global instance of registers and *extra* data. 
registers_t g_registers;

// Locate two copies of the package in EEPROM. 
typedef struct {
    eeprom_driver_checksum_t cs;
    registers_t registers;
} EepromPackage;

static EepromPackage EEMEM f_eeprom_package[2];

static void set_defaults(void* data, const void* defaultarg);

// Cath old usage of this lib that used this macro to add extra data. 
#ifdef REGS_EXTRA_FUNC_DEFS
# error Define these in a regs.local.cpp file.
#endif

// EEPROM block definition. 
static const eeprom_driver_block_t EEPROM_BLK PROGMEM = {
    REGS_VERSION,													// Defines schema of data. 
    sizeof(registers_t) - REGS_START_NV_IDX * sizeof(uint16_t),    	// Size of user data block.
    { (void*)&f_eeprom_package[0], (void*)&f_eeprom_package[1] },	// Address of two blocks of EEPROM data. They do not have to be contiguous
    (void*)&g_registers.vals[REGS_START_NV_IDX],             		// User data in RAM.
    set_defaults, 													// Fills user RAM data with default data.
};

// Macro that expands to a array initialization for the default value. 
#define REGS_GEN_DEFAULT_VALS(_name, default_, desc_) \
 (default_),

void regsNvSetRegisterDefaults(uint8_t start, uint8_t end) {
	// TODO: can we miss out the first few values, for the volatile registers, since they are never used?
	static const regs_t DEFAULT_VALUES[REGS_COUNT] PROGMEM = { 		
		REGS_DEFS(REGS_GEN_DEFAULT_VALS)
	};
	memcpy_P((void*)&g_registers.vals[start], (const void*)&DEFAULT_VALUES[start], sizeof(regs_t) * (end - start));
}

static void set_defaults(void* data, const void* defaultarg) {
    (void)data;													// Not used. 
    (void)defaultarg;											// Not used. 
    regsNvSetRegisterDefaults(REGS_START_NV_IDX, REGS_COUNT);	// Set default values for NV regs.
	regsExtraSetDefaults();										// Set default values for extra user data.
}

void regsNvSetDefaults() { set_defaults(NULL, NULL); }

void regsInit() { 
    eepromDriverInit(&EEPROM_BLK); 		// Inialise EEPROM driver, currently a no-op. 
    regsNvRead();						// Writes regs from REGS_START_NV_IDX on up, does not write to 0..(REGS_START_NV_IDX-1)
}

uint8_t regsNvRead() {
	return eepromDriverRead(&EEPROM_BLK, NULL);
}

void regsNvWrite() {
    eepromDriverWrite(&EEPROM_BLK);
}

bool regsWriteMask(uint8_t idx, regs_t mask, bool s) { return utilsWriteBitmask<regs_t>(g_registers.vals[idx], mask, s); }
bool regsFlagsWriteMask(regs_t mask, bool s) { return regsWriteMask(REGS_IDX_FLAGS, mask, s); }

// Help text for register names. Note that if you don't call regsGetRegisterName() then the linker will not include this in the program. 
//

// String returned if the index is out of range.
static const char NO_NAME[] PROGMEM = "??";

// Macro that expands to a string name for the register index item, so gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'REG_IDX_NAME_FLAGS'
#define REGS_GEN_REG_IDX_NAME_STR(_name, default_, desc_) \
 REG_IDX_NAME_##_name,

// Macro that expands to a string DEFINITION of the name for the register index item, e.g. 
//  gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'static const char REG_IDX_NAME_FLAGS[] PROGMEM = "FLAGS";'
#define REGS_GEN_REG_IDX_NAME_STR_DEF(_name, default_, desc_) \
 static const char REG_IDX_NAME_##_name[] PROGMEM = #_name;

// Now can define function for index names. 
const char* regsGetRegisterName(uint8_t idx) {
    REGS_DEFS(REGS_GEN_REG_IDX_NAME_STR_DEF)		// Declarations of the index name strings. 
	
	static const char* const REG_IDX_NAMES[REGS_COUNT] PROGMEM = { // Declaration of the array of these strings. 
        REGS_DEFS(REGS_GEN_REG_IDX_NAME_STR)
	};
   return (idx < REGS_COUNT) ? (const char*)pgm_read_word(&REG_IDX_NAMES[idx]) : NO_NAME;
}

// Help text for register items.
//

// Macro that expands to a string name for the register index description, so gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'REG_IDX_DESC_FLAGS'
#define REGS_GEN_REG_IDX_DESC_STR(_name, default_, desc_) \
 REG_IDX_DESC_##_name,

// Macro that expands to a string DEFINITION of the description of the register index item, e.g. 
//  gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'static const char REG_IDX_DESC_FLAGS[] PROGMEM = "Various flags, see REGS_FLAGS_MASK_xxx";'
#define REGS_GEN_REG_IDX_DESC_STR_DEF(_name, default_, desc_) \
 static const char REG_IDX_DESC_##_name[] PROGMEM = desc_;

// Now can define function for string descriptions. 
const char* regsGetRegisterDescription(uint8_t idx) {
    REGS_DEFS(REGS_GEN_REG_IDX_DESC_STR_DEF)		// Declarations of the index name strings. 
	
	static const char* const REG_IDX_DESCS[REGS_COUNT] PROGMEM = { // Declaration of the array of these strings. 
        REGS_DEFS(REGS_GEN_REG_IDX_DESC_STR)
	};        
   return (idx < REGS_COUNT) ? (const char*)pgm_read_word(&REG_IDX_DESCS[idx]) : NO_NAME;
}

// Extra help string
//
const char* regsGetHelpStr() {
    static const char HELP_STR[] PROGMEM = REGS_EXTRA_HELP_STR;
    return HELP_STR;
}

bool regsHexFormat(uint8_t reg_idx) { return !!(_BV(reg_idx) & REGS_PRINT_HEX_MASK); }

#if CFG_WANT_CONSOLE_PRINT_FOR_REGS
#include <FConsole.h>
void regsPrintRegValue(uint8_t idx) { 
	if (regsHexFormat(idx)) 
		consolePrint(CONSOLE_PRINT_HEX, (console_cell_t)REGS[idx]);
	else {
#if REGS_VAL_SIGNED
		consolePrint(CONSOLE_PRINT_SIGNED, (console_cell_t)REGS[idx]);
#else		
		consolePrint(CONSOLE_PRINT_UNSIGNED, (console_cell_t)REGS[idx]);
#endif
	}
}
#endif // #if CFG_WANT_CONSOLE_PRINT_FOR_REGS

void regsPrintRegValues(uint8_t start, uint8_t end) { 
	while (start < end) 
		regsPrintRegValue(start++);
}

void regsPrintRegValuesAll() { regsPrintRegValues(0, REGS_COUNT); }
void regsPrintRegValuesRam() { regsPrintRegValues(0, REGS_START_NV_IDX); }

// eof
