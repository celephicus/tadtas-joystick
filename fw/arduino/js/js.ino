#include <Arduino.h>

#include "project_config.h"
#include "debug.h"
FILENUM(1);

#if CFG_WANT_MOUSE
#include "Mouse.h"
#endif

#include "utils.h"
#include "regs.h"
#include "driver.h"

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
}

static bool console_cmds_user(char* cmd) {
	switch (console_hash(cmd)) {
		case /** + **/ 0XB58E: console_binop(+); break;
		case /** - **/ 0XB588: console_binop(-); break;
		case /** * **/ 0XB58F : console_binop(*); break;
		case /** / **/ 0XB58A : console_verify_can_pop(2); { const console_cell_t divisor = console_u_pop(); if (0 == divisor) { console_drop(); console_raise(CONSOLE_RC_ERROR_DIVISON_BY_ZERO); } console_u_tos() = console_u_tos() / divisor; }; break;
		case /** NEGATE **/ 0X7A79: console_unop(-); break;
		case /** # **/ 0XB586: console_raise(CONSOLE_RC_STATUS_IGNORE_TO_EOL); break;
		case /** LED **/ 0XDC88: gpioDebugLedWrite(!console_u_pop()); break;
		case /** .S **/ 0X66B8: { uint8_t i = console_u_depth(); while (i > 0) consolePrint(CONSOLE_PRINT_SIGNED, console_u_pick(--i)); } break; 

		REGS_CONSOLE_COMMANDS_NV
		REGS_CONSOLE_COMMANDS_ACCESS
		REGS_CONSOLE_COMMANDS_PRINT
		REGS_CONSOLE_COMMANDS_PRINT_VERBOSE
		DEBUG_CONSOLE_COMMANDS
		DRIVER_CONSOLE_COMMANDS
		default: return false;
	}
	return true;
}
static void console_init() {
	GPIO_SERIAL_CONSOLE.begin(CFG_CONSOLE_BAUDRATE);
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
	wdt_reset();
	GPIO_SERIAL_CONSOLE.print(F("Runtime error: ")); 
	GPIO_SERIAL_CONSOLE.print(fileno); GPIO_SERIAL_CONSOLE.print(F("."));
	GPIO_SERIAL_CONSOLE.print(lineno); GPIO_SERIAL_CONSOLE.print(F("."));
	GPIO_SERIAL_CONSOLE.print(errorno); GPIO_SERIAL_CONSOLE.print(F(": time to die.\r\n"));
	delay(100);		// Allow serial data to escape.
	cli();
	gpioDebugLedSetModeOutput();
	gpioDebugLedSet();
	while (1) {
		gpioDebugLedToggle();
		fori(6) delayMicroseconds(16000);		// delay() doesn't work if interrupts off. 
		wdt_reset();
	}
}

void setup() {
	debug_init();
	gpio_init();
	regsInit();
	console_init();
	driverInit();
#if CFG_WANT_MOUSE
	Mouse.begin();
#endif
}

static void do_dump_regs() {
    static int8_t s_ticker;
    
    if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS) {
		if (--s_ticker <= 0) {
			s_ticker = (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS_FAST) ? 2 : 10; // Dump 5Hz or 1Hz.
			GPIO_SERIAL_CONSOLE.print(F("Regs: ")); GPIO_SERIAL_CONSOLE.print(millis()); GPIO_SERIAL_CONSOLE.print(F(": ")); 
			regsPrintRegValuesRam();
			consolePrint(CONSOLE_PRINT_NEWLINE, 0);
		}
    }
	else 
		s_ticker = 0;
}

static void service_hx711() {
	int32_t hx711_readings[GPIO_JS_AXIS_COUNT];
	if (driverGetJs1Data(hx711_readings)) {
		if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_JS) {
			GPIO_SERIAL_CONSOLE.print(F("JS: ")); GPIO_SERIAL_CONSOLE.print(millis()); GPIO_SERIAL_CONSOLE.print(F(" ")); 
			driverPrintJsData(hx711_readings);
			consolePrint(CONSOLE_PRINT_NEWLINE, 0);
		}
 	}

#if CFG_WANT_MOUSE
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_MOUSE_EMULATION) {
			int8_t delta[2] = {0, 0};
			fori (2) {
				if ((hx711_readings[i] > +REGS[REGS_IDX_MOUSE_XY_DEADBAND]) || (hx711_readings[i] < -REGS[REGS_IDX_MOUSE_XY_DEADBAND]))
					delta[i] = utilsLimitI32(hx711_readings[i] / (int32_t)REGS[REGS_IDX_MOUSE_XY_SCALE], -100, +100);
			}
			if (delta[0] || delta[1])
				Mouse.move(delta[0], delta[1]);
		}
	}
#endif

}

void loop() {
	FConsole.service();
	driverService();
	
	// Dump registers every 100ms.
	runEveryU16(100) {
		do_dump_regs();
	}
	service_hx711();
	
	debugKickWatchdog(DEBUG_WATCHDOG_MASK_MAINLOOP);
}

