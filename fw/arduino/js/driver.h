#ifndef DRIVER_H__
#define DRIVER_H__

// Always need this.
#include "gpio.h"

// Call first off to initialise the driver. 
void driverInit();

// Call in mainloop to perform services.
void driverService();

bool driverGetHx711Data(int32_t* data);

// Commands to access driver from console. 
#define DRIVER_CONSOLE_COMMANDS																										\


#endif // DRIVER_H__
