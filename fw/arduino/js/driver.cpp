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

// Generic joystick driver
//

// Simple filter.
typedef int32_t filter_accum_t;
int32_t filter(filter_accum_t* accum, int32_t input, uint8_t k, bool reset=false) {
	#if 1
	*accum = reset ? ((filter_accum_t)input << k) : (*accum - (*accum >> k) + (filter_accum_t)input);
	return (int32_t)(*accum >> k);		// Is right shift on signed arithmetic or logical?
	#else
	const int32_t mult = 1L << k;
	*accum = reset ? ((filter_accum_t)input * mult) : (*accum - (*accum / mult) + (filter_accum_t)input);
	return (int32_t)(*accum / mult);
	#endif
}

// Holds state of single axis.
typedef struct {
	int32_t raw;
	int32_t filtered;
	int32_t offset;
	filter_accum_t accum;
} hx711_t;
static hx711_t f_hx711_data[COUNT_GPIO_HX711];

// Timer for emulation.
static uint16_t f_js_timer;
const uint16_t JS_EMU_PERIOD_MS = 90;

static const uint8_t f_hx711_data_pins[COUNT_GPIO_HX711] = { GPIO_PIN_HX711_DT_X, GPIO_PIN_HX711_DT_Y, GPIO_PIN_HX711_DT_Z };
static HX711MULTI f_hx711(COUNT_GPIO_HX711, (byte*)f_hx711_data_pins, GPIO_PIN_HX711_CLK);

// For emulation we have a simple abstract interface with a single operation which supplies data at 11Hz rate.
bool read_raw(int32_t* data) {	
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_JOYSTICK_EMULATION) {
		if (utilsIsTimerDone(f_js_timer, JS_EMU_PERIOD_MS)) {
			utilsStartTimer(f_js_timer);
			fori(COUNT_GPIO_HX711)
				data[i] = analogRead(A0 + i);
			return true;
		}
	}
	else {
		if (f_hx711.is_ready()) {
			(void)f_hx711.readRaw((long*)data);		// Can't fail, we just checked.
			return true;
		}
	}
	return false;
}

static void init_hx711() {
	fori(COUNT_GPIO_HX711) {
		f_hx711_data[i].raw	= f_hx711_data[i].filtered = 0;		
		f_hx711_data[i].offset = (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_JOYSTICK_EMULATION) ? 512 : 0;
	}
}

static void service_hx711() {
	static uint8_t filter_k = 0xff;		// Force filter reset on startup.
	int32_t raw[COUNT_GPIO_HX711];		// Get raw readings.
	if (read_raw(raw)) {
		// When we change the filter rate we must reset it to avoid big bumps.
		bool filter_reset = false;
		if (REGS[REGS_IDX_HX711_FILTER_K] != filter_k) {
			filter_k = REGS[REGS_IDX_HX711_FILTER_K];
			filter_reset = true;
		}
		
		fori(COUNT_GPIO_HX711) {
			f_hx711_data[i].raw = (REGS[REGS_IDX_ENABLES] & (REGS_ENABLES_MASK_AXIS_INVERT_X << i)) ? -raw[i] : raw[i];		// Copy raw readings with inversion set by flag. 
			f_hx711_data[i].filtered = utilsFilter<int32_t, int32_t>(f_hx711_data[i].accum, f_hx711_data[i].raw, filter_k, filter_reset);	// Apply filter.
		}
		regsFlagsWriteMask(REGS_FLAGS_MASK_HX711_UPDATE, true);
	}
}

void driverPeekHx711Data(int32_t* data) {
	fori(COUNT_GPIO_HX711) 
		data[i] = f_hx711_data[i].filtered - f_hx711_data[i].offset;
}

bool driverGetHx711Data(int32_t* data) {
	bool available = !!(regsFlags() & REGS_FLAGS_MASK_HX711_UPDATE);
	if (available) {
		driverPeekHx711Data(data);
		regsFlagsWriteMask(REGS_FLAGS_MASK_HX711_UPDATE, false);
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
		if (i < COUNT_GPIO_HX711 - 1) GPIO_SERIAL_CONSOLE.print(F(" "));
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

