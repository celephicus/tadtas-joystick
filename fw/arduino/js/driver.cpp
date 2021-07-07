#include <Arduino.h>
#include "HX711-multi.h"

#include "project_config.h"
#include "debug.h"
FILENUM(3);

#include "driver.h"
#include "utils.h"
#include "thread.h"
#include "regs.h"
#include "console.h"

// HX711 driver
//
static const uint8_t f_hx711_data_pins[COUNT_GPIO_HX711] = { GPIO_PIN_HX711_DT_X, GPIO_PIN_HX711_DT_Y, GPIO_PIN_HX711_DT_Z };
static HX711MULTI f_hx711(COUNT_GPIO_HX711, (byte*)f_hx711_data_pins, GPIO_PIN_HX711_CLK);

static int32_t f_raw_hx711[COUNT_GPIO_HX711];

static void init_hx711() {
}

static void service_hx711() {
	if (f_hx711.is_ready()) {
		f_hx711.readRaw((long*)f_raw_hx711);
		regsWriteMaskFlags(REGS_FLAGS_MASK_HX711_UPDATE, true);
	}
}

bool driverGetHx711Data(int32_t* data) {
	bool available = !!(regsGetFlags() & REGS_FLAGS_MASK_HX711_UPDATE);
	if (available) {
		memcpy(data, f_raw_hx711, sizeof(f_raw_hx711));
		regsWriteMaskFlags(REGS_FLAGS_MASK_HX711_UPDATE, false);
	}
	return available;
}
// Externals
//

void driverInit() {
	init_hx711();
}

void driverService() {
    service_hx711();
}

// This has to go somewhere. 
uint16_t threadGetTicks() { return (uint16_t)millis(); }

