#include <Arduino.h>

#include "console.h"
#include "console-internals.h"
#include "FConsole.h"
#include "gpio.h"

// Needed for user handler.

static bool console_cmds_user(char* cmd) {
	switch (console_hash(cmd)) {
		case /** + **/ 0XB58E: console_binop(+); break;
		case /** - **/ 0XB588: console_binop(-); break;
		case /** NEGATE **/ 0X7A79: console_unop(-); break;
		case /** # **/ 0XB586: console_raise(CONSOLE_RC_STATUS_IGNORE_TO_EOL); break;
		case /** LED **/ 0XDC88: gpioDebugLedWrite(!console_u_pop()); break;
		case /** LED1 **/ 0X6DB9: gpioDebugLed1Write(!console_u_pop()); break;
		default: return false;
	}
	return true;
}

void setup() {
	GPIO_SERIAL_CONSOLE.begin(115200);
  while (!GPIO_SERIAL_CONSOLE) 
    /* empty */ ;
	
	FConsole.begin(console_cmds_user, GPIO_SERIAL_CONSOLE);

	// Signon message, note two newlines to leave a gap from any preceding output on the terminal.
	// Also note no following newline as the console prints one at startup, then a prompt. 
	consolePrint(CONSOLE_PRINT_STR_P, 
	  (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "TADTAS Strain Gauge Mouse Prototype"));
	FConsole.prompt();

  gpioInit();
  gpioDebugLedSetModeOutput();
  gpioDebugLedSet();
  gpioDebugLed1SetModeOutput();
  gpioDebugLed1Set();
}

void loop() {
	FConsole.service();
}
