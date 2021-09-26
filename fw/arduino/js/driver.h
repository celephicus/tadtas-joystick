#ifndef DRIVER_H__
#define DRIVER_H__

// Always need this.
#include "gpio.h"

// Call first off to initialise the driver. 
void driverInit();

// Call in mainloop to perform services.
void driverService();

// Return most recent HX711 data, do not clear updated flag.
void driverPeekJsData(int32_t* data);

// Return true, set new data if it has been updated, then clear updated flag. Else return false. 
bool driverGetJs1Data(int32_t* data);

// Set the most recent data as the zero point. Like tare. 
void driverZeroJs();

// Print the HX711 data to the console as a comma seperated list of signed integers. Does not a[[end a newline.
void driverPrintJsData(const int32_t* data);

// Set relay state to flags register. 
void driverWriteRelayState(uint16_t s);

// Commands to access driver from console. 
#define DRIVER_CONSOLE_COMMANDS																																	\
	case /** JS-Z **/ 0XCD0B: driverZeroJs(); break;																											\
	case /** ?JS **/ 0X7AC3: { int32_t d[GPIO_JS_AXIS_COUNT]; driverPeekJsData(d); driverPrintJsData(d); consolePrint(CONSOLE_PRINT_NEWLINE, 0); } break;		\
	case /** JS-DUMP **/ 0XE11D: regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_JS, !!console_u_pop()); break;											\
	case /** RELAY **/ 0X1DA6: console_verify_can_pop(1); driverWriteRelayState((uint16_t)console_u_pop()); break;												\
	
#endif // DRIVER_H__
