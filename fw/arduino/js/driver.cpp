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

// Simple filter.
typedef int32_t filter_accum_t;
int32_t filter(filter_accum_t* accum, int32_t input, uint8_t k, bool reset=false) {
#if 1	
	*accum = reset ? ((filter_accum_t)input << k) : (*accum - (*accum >> k) + (filter_accum_t)input); 
	return (int32_t)(*accum >> k);		// Is right shift on signed arithmetic or logical?
#else
	const int32_t mult = 1L << k;
	*accum = reset ? ((filter_accum_t)input * mult) : (*accum - (*accum / mult) + (filter_accum_t)input); 
	return (int32_t)(*accum / mult);		// Is right shift on signed arithmetic or logical?
#endif
}
void filterInit(filter_accum_t* accum, int32_t input, uint8_t k) { filter(accum, input, k, true); }

// Holds state of single HX711. 
typedef struct {
	int32_t raw;
	int32_t filtered;
	int32_t offset;
	filter_accum_t accum;
} hx711_t;

static hx711_t f_hx711_data[COUNT_GPIO_HX711];

static void init_hx711() {
	memset(f_hx711_data, 0, sizeof(f_hx711));
}


static void service_hx711() {
	if (f_hx711.is_ready()) {
		int32_t raw[COUNT_GPIO_HX711];		// Get raw readings.
		f_hx711.readRaw((long*)raw);
		
		fori(COUNT_GPIO_HX711) {
			f_hx711_data[i].raw = (REGS[REGS_IDX_ENABLES] & (REGS_ENABLES_MASK_AXIS_INVERT_X << i)) ? - raw[i] : raw[i];		// Copy raw readings with inversion set by flag. 
			f_hx711_data[i].filtered = filter(&f_hx711_data[i].accum, f_hx711_data[i].raw, REGS[REGS_IDX_HX711_FILTER_K]);	// Apply filter.
		}
		regsWriteMaskFlags(REGS_FLAGS_MASK_HX711_UPDATE, true);
	}
}

void driverPeekHx711Data(int32_t* data) {
	fori(COUNT_GPIO_HX711) 
		data[i] = f_hx711_data[i].filtered - f_hx711_data[i].offset;
}

bool driverGetHx711Data(int32_t* data) {
	bool available = !!(regsGetFlags() & REGS_FLAGS_MASK_HX711_UPDATE);
	if (available) {
		driverPeekHx711Data(data);
		regsWriteMaskFlags(REGS_FLAGS_MASK_HX711_UPDATE, false);
	}
	return available;
}

void driverZeroHx711() {
		fori(COUNT_GPIO_HX711) 
			f_hx711_data[i].offset = f_hx711_data[i].filtered;
}


void driverPrintHx711Data(const int32_t* data) {
	fori (COUNT_GPIO_HX711) {
		GPIO_SERIAL_CONSOLE.print(data[i]);
		if (i < COUNT_GPIO_HX711 - 1) GPIO_SERIAL_CONSOLE.print(F(", "));
	}
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

