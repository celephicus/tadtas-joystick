// project-config.h -- Configuration file for shared libraries.

#ifndef PROJECT_CONFIG_H__
#define PROJECT_CONFIG_H__

// For lc2.h
#define CFG_LC2_USE_SWITCH 0

// Watchdog. 
#define CFG_WATCHDOG_TIMEOUT WDTO_2S   
#define CFG_WATCHDOG_ENABLE 1
#define CFG_WATCHDOG_MODULE_COUNT 0		// How many extra modules to kick the dog apart from main and a timer ISR?
#define CFG_RUNTIME_ERROR_ON_WATCHDOG_RESTART 0		// If true raise a runtime error on a watchdog restart. 

// Regs module settings...
#define CFG_WANT_CONSOLE_PRINT_FOR_REGS 1 // Define regsPrintRegValue() to use consolePrint(). Requires regs_t to be compatible with console_cell_t. 

// Console speed.
#define CFG_CONSOLE_BAUDRATE 115200
#define CFG_WANT_CONSOLE_ECHO 1
#define CFG_CONSOLE_INPUT_ACCEPT_BUFFER_SIZE (32)

#endif  // PROJECT_CONFIG_H__

