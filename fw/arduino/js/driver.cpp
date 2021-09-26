#include <Arduino.h>

#include "project_config.h"
#include "debug.h"
FILENUM(3);

#include "driver.h"
#include "gpio.h"
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

// Holds state of single axis, common for strain gauge & emulated joystick.
typedef struct {
	int32_t raw;
	int32_t filtered;
	int32_t offset;
	filter_accum_t accum;
} js_context_t;
static js_context_t f_js_context[GPIO_JS_AXIS_COUNT];

#if CFG_WANT_JS_EMULATION
static const uint16_t JS_EMU_PERIOD_MS = 90;		// About 11 Sa/s from HX711.
static const int32_t JS_EMU_MAX = 8390L;			// max gui sensitivity is +/- this value. 
static uint16_t f_js_emu_delay;

// For emulation we have a simple abstract interface with a single operation which supplies data at 11Hz rate.
bool js_read_raw(int32_t* data) {	
	if (utilsIsTimerDone(f_js_emu_delay, JS_EMU_PERIOD_MS)) {
		utilsStartTimer(f_js_emu_delay);
		for(uint8_t i = GPIO_JS_AXIS_X; i <= GPIO_JS_AXIS_Y; i += 1) {		// Only emulate x, y; z axis not touched. 
			// Rescale input 0..1023 to -M..+M where M=8.39E3, maximum gui sensitivity. Max numerator = 1023*2*8390*125=2145742500.
			static const int32_t SF = 125L;
			data[i] = utilsRoundedDivide((int32_t)analogRead(GPIO_PIN_ADC_EMU_JS_X + i) * 2 * JS_EMU_MAX * SF, 1023L * SF) - JS_EMU_MAX;
		}
		data[GPIO_JS_AXIS_Z] = 0;
		return true;
	}
	return false;
}
	
void driverZeroJs() {
	/* empty */
}

#else

#include "HX711-multi.h"
static const uint8_t HX711_DATA_PINS[GPIO_JS_AXIS_COUNT] = { GPIO_PIN_HX711_DT_X, GPIO_PIN_HX711_DT_Y, GPIO_PIN_HX711_DT_Z };
static HX711MULTI f_hx711(GPIO_JS_AXIS_COUNT, (byte*)HX711_DATA_PINS, GPIO_PIN_HX711_CLK);

bool js_read_raw(int32_t* data) {	
	if (f_hx711.is_ready()) {
		(void)f_hx711.readRaw((long*)data);		// Can't fail, we just checked.
		return true;
	}
	return false;
}

void driverZeroJs() {
	fori(GPIO_JS_AXIS_COUNT)
		f_js_context[i].offset = f_js_context[i].filtered;
}

#endif

static void init_js() {
	fori(GPIO_JS_AXIS_COUNT)
		f_js_context[i].raw	= f_js_context[i].filtered = f_js_context[i].offset = 0;
}

static void service_js() {
	static uint8_t filter_k = 0xff;		// Force filter reset on startup.
	
	int32_t raw[GPIO_JS_AXIS_COUNT];		// For raw readings.
	if (js_read_raw(raw)) {
		// When we change the filter rate we must reset it to avoid big bumps.
		bool filter_reset = false;
		if (REGS[REGS_IDX_JS_FILTER_K] != filter_k) {
			filter_k = REGS[REGS_IDX_JS_FILTER_K];
			filter_reset = true;
		}
		
		fori(GPIO_JS_AXIS_COUNT) {
			f_js_context[i].raw = (REGS[REGS_IDX_ENABLES] & (REGS_ENABLES_MASK_AXIS_INVERT_X << i)) ? -raw[i] : raw[i];		// Copy raw readings with inversion set by flag. 
			f_js_context[i].filtered = utilsFilter<int32_t, int32_t>(f_js_context[i].accum, f_js_context[i].raw, filter_k, filter_reset);	// Apply filter.
		}
		regsFlagsWriteMask(REGS_FLAGS_MASK_JS_UPDATE, true);
	}
}

void driverPeekJsData(int32_t* data) {
	fori(GPIO_JS_AXIS_COUNT) 
		data[i] = f_js_context[i].filtered - f_js_context[i].offset;
}

bool driverGetJs1Data(int32_t* data) {
	bool available = !!(regsFlags() & REGS_FLAGS_MASK_JS_UPDATE);
	if (available) {
		driverPeekJsData(data);
		regsFlagsWriteMask(REGS_FLAGS_MASK_JS_UPDATE, false);
	}
	return available;
}

void driverPrintJsData(const int32_t* data) {
	fori (GPIO_JS_AXIS_COUNT) {
		GPIO_SERIAL_CONSOLE.print(data[i]);
		if (i < GPIO_JS_AXIS_COUNT - 1) GPIO_SERIAL_CONSOLE.print(F(" "));
	}
}

// Relays

static const uint8_t RLY_PINS[GPIO_COUNT_RLY] PROGMEM = { GPIO_PIN_RLY_0, GPIO_PIN_RLY_1, GPIO_PIN_RLY_2, GPIO_PIN_RLY_3, };
static const regs_t RLY_MASK_ALL = REGS_FLAGS_MASK_RELAY_0 | REGS_FLAGS_MASK_RELAY_1 | REGS_FLAGS_MASK_RELAY_2 | REGS_FLAGS_MASK_RELAY_3;

static void init_relays() {
	fori(ELEMENT_COUNT(RLY_PINS)) {
		const uint8_t rly_pin = pgm_read_byte(&RLY_PINS[i]);
		pinMode(rly_pin, OUTPUT);
		digitalWrite(rly_pin, 1);
	}
}
static void service_relays() {
	static regs_t s_rly_state = REGS_FLAGS_MASK_RELAY_0;
	const regs_t rly_flags = regsFlags() & RLY_MASK_ALL;
	if (rly_flags != s_rly_state) {
		s_rly_state = rly_flags;
		regs_t m = REGS_FLAGS_MASK_RELAY_0;
		fori(ELEMENT_COUNT(RLY_PINS)) {
			const uint8_t rly_pin = pgm_read_byte(&RLY_PINS[i]);
			digitalWrite(rly_pin, !(rly_flags & m));
			m <<= 1;
		}
	}
}
void driverWriteRelayState(uint16_t s) {
	regs_t m = REGS_FLAGS_MASK_RELAY_0;
	while (m > 1) {
		m >>= 1;
		s <<= 1;
	}
	utilsUpdateBitmask(REGS[REGS_IDX_FLAGS], RLY_MASK_ALL, s);
}

// Externals
//

void driverInit() {
	init_js();
	init_relays();
}

void driverService() {
	service_js();
	runEveryU16(50) service_relays();
}

// This has to go somewhere. 
uint16_t threadGetTicks() { return (uint16_t)millis(); }

