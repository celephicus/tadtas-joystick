#include <Arduino.h>

#include "project_config.h"
#include "debug.h"
FILENUM(1);

#include "utils.h"
#include "regs.h"
//#include "driver.h"

#include "console.h"
#include "console-internals.h"
#include "FConsole.h"
#include "gpio.h"
#include <avr/wdt.h>

static void debug_init() {
	watchdog_module_mask_t wdt_mask;
	const uint16_t mcusr = debugWatchdogInit(&wdt_mask);
	REGS[REGS_IDX_RESTART] = mcusr | ((uint16_t)wdt_mask << 8);
}

static void gpio_init() {
  gpioInit();
  gpioDebugLedSetModeOutput();
  gpioDebugLedSet();
  gpioDebugLed1SetModeOutput();
  gpioDebugLed1Set();
}

static bool console_cmds_user(char* cmd) {
	switch (console_hash(cmd)) {
		case /** + **/ 0XB58E: console_binop(+); break;
		case /** - **/ 0XB588: console_binop(-); break;
		case /** NEGATE **/ 0X7A79: console_unop(-); break;
		case /** # **/ 0XB586: console_raise(CONSOLE_RC_STATUS_IGNORE_TO_EOL); break;
		case /** LED **/ 0XDC88: gpioDebugLedWrite(!console_u_pop()); break;
		case /** LED1 **/ 0X6DB9: gpioDebugLed1Write(!console_u_pop()); break;
		REGS_CONSOLE_COMMANDS
		DEBUG_CONSOLE_COMMANDS
		default: return false;
	}
	return true;
}
static void console_init() {
	GPIO_SERIAL_CONSOLE.begin(115200);
	while (!GPIO_SERIAL_CONSOLE) 
		/* empty */ ;
	
	FConsole.begin(console_cmds_user, GPIO_SERIAL_CONSOLE);

	// Signon message, note two newlines to leave a gap from any preceding output on the terminal.
	// Also note no following newline as the console prints one at startup, then a prompt. 
	consolePrint(CONSOLE_PRINT_STR_P, 
	  (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "TADTAS Strain Gauge Mouse Prototype"));
	FConsole.prompt();
}
	
void debugRuntimeError(uint8_t fileno, uint16_t lineno, uint8_t errorno) {
	(void)fileno;
	(void)lineno;
	(void)errorno;
	wdt_reset();
	cli();
	while (1) 
		wdt_reset();
}

void setup() {
	debug_init();
	gpio_init();
	regsInit();
	console_init();
	//driverInit();
}

static void do_dump_regs() {
    static int8_t s_ticker;
    
    if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS) {
		if (--s_ticker <= 0) {
			s_ticker = (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS_FAST) ? 2 : 10; // Dump 5Hz or 1Hz.
			GPIO_SERIAL_CONSOLE.print(F("Regs: ")); GPIO_SERIAL_CONSOLE.print(millis()); GPIO_SERIAL_CONSOLE.print(F(": ")); 
			regsPrintValuesRam();
			consolePrint(CONSOLE_PRINT_NEWLINE, 0);
		}
    }
	else 
		s_ticker = 0;
}

void loop() {
	FConsole.service();
	//driverService();
	
	// Dump registers every 100ms.
	runEveryU16(100) {
		do_dump_regs();
	}
	
	debugKickWatchdog(DEBUG_WATCHDOG_MASK_MAINLOOP);
}

