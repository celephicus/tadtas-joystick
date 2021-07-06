#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "project_config.h"
#include "debug.h"
#include "utils.h"
#include "eeprom_driver.h"
#include "regs.h"

FILENUM(208);  // All source files in common have file numbers starting at 200.

// I like them at offset 0, not actually necessary.
STATIC_ASSERT(0 == REGS_IDX_FLAGS);

registers_t g_registers;

// Locate two copies of the package in EEPROM.
typedef struct {
    eeprom_driver_checksum_t cs;
    registers_t registers;
} EepromPackage;

static EepromPackage EEMEM f_eeprom_package[2];

static void set_defaults(void* data, const void* defaultarg);

// EEPROM block definition.
static const eeprom_driver_block_t EEPROM_BLK PROGMEM = {
    REGS_VERSION,													// Defines schema of data.
    sizeof(registers_t) - REGS_START_NV_IDX * sizeof(uint16_t),    	// Size of user data block.
    { &f_eeprom_package[0], &f_eeprom_package[1] },					// Address of two blocks of EEPROM data. They do not have to be contiguous
    &g_registers.vals[REGS_START_NV_IDX],             				// User data in RAM.
    set_defaults, 													// Fills user RAM data with default data.
};

// Macro that expands to a array initialization for the default value.
#define REGS_GEN_DEFAULT_VALS(_name, default_, desc_) \
 (default_),

// Local function to set all the NV regs (not the volatile regs) to default values. Args ignored.
static void set_register_defaults() {
	static const uint16_t DEFAULT_VALUES[REGS_COUNT] PROGMEM = { 		// TODO: can we miss out the first few values, for the volatile registers, since they are never used?
		REGS_DEFS(REGS_GEN_DEFAULT_VALS)
	};
	memcpy_P((void*)&g_registers.vals[REGS_START_NV_IDX], (const void*)&DEFAULT_VALUES[REGS_START_NV_IDX], sizeof(uint16_t) * (REGS_COUNT - REGS_START_NV_IDX));
}

static void set_defaults(void* data, const void* defaultarg) {
    (void)data;
    (void)defaultarg;
    set_register_defaults();									// Set default values for NV regs.
}

void regsNvSetDefaults() { set_defaults(NULL, NULL); }

uint8_t regsInit() {
    eepromDriverInit(&EEPROM_BLK); 		// Was a No-op last time I looked.
    return regsNvRead();				// Writes regs from REGS_START_NV_IDX on up, does not write to 0..(REGS_START_NV_IDX-1)
}

uint8_t regsNvRead() {
	const uint8_t nvregs_rc = eepromDriverRead(&EEPROM_BLK, NULL);
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_CORRUPT_BANK_0, !!(nvregs_rc & EEPROM_DRIVER_BANK_CORRUPT_0_MASK));
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_CORRUPT_BANK_1, !!(nvregs_rc & EEPROM_DRIVER_BANK_CORRUPT_1_MASK));
	return nvregs_rc;
}

void regsNvWrite() {
    eepromDriverWrite(&EEPROM_BLK);
}

bool regsWriteMask(uint8_t idx, uint16_t mask, bool s) { return utilsWriteFlags<uint16_t>(&g_registers.vals[idx], mask, s); }
bool regsUpdateMask(uint8_t idx, uint16_t mask, uint16_t value) { return utilsUpdateFlags<uint16_t>(&g_registers.vals[idx], mask, value); }

bool regsWriteMaskFlags(uint16_t mask, bool s) { return regsWriteMask(REGS_IDX_FLAGS, mask, s); }
bool regsUpdateMaskFlags(uint16_t mask, uint16_t value) { return regsUpdateMask(REGS_IDX_FLAGS, mask, value); }

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

#include "console.h"			// We use the console output functions.
#include <avr/wdt.h>

void regsPrintValue(uint8_t idx) {
	consolePrint((_BV(idx) & REGS_PRINT_HEX_MASK) ? CONSOLE_PRINT_HEX : CONSOLE_PRINT_UNSIGNED, REGS[idx]);
}
void regsPrintValues(uint8_t s_idx, uint8_t e_idx) {
	while (s_idx < e_idx)
		regsPrintValue(s_idx++);
}
void regsPrintValuesAll() { regsPrintValues(0, REGS_COUNT); }
void regsPrintValuesRam() { regsPrintValues(0, REGS_START_NV_IDX); }

void regsPrintValuesVerbose() {
	fori (REGS_COUNT) {
		consolePrint(CONSOLE_PRINT_NEWLINE, 0);
		consolePrint(CONSOLE_PRINT_SIGNED, i);
		consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)regsGetRegisterName(i));
		regsPrintValue(i);
		consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)regsGetRegisterDescription(i));
		wdt_reset();
	}
#ifdef REGS_EXTRA_HELP_STR			// This is defined in regs.local.h, or not.
	consolePrint(CONSOLE_PRINT_NEWLINE, 0);
	consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(REGS_EXTRA_HELP_STR));
#endif
 }

// eof
