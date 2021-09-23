#include <Arduino.h>
#include "HX711-multi.h"

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

// Holds state of single axis.
typedef struct {
	int32_t raw;
	int32_t filtered;
	int32_t offset;
	filter_accum_t accum;
} hx711_t;
static hx711_t f_hx711_data[COUNT_GPIO_HX711];

// Timer for emulation.
static bool f_js_emu_ready;
const uint16_t JS_EMU_PERIOD_MS = 90;

static const uint8_t f_hx711_data_pins[COUNT_GPIO_HX711] = { GPIO_PIN_HX711_DT_X, GPIO_PIN_HX711_DT_Y, GPIO_PIN_HX711_DT_Z };
static HX711MULTI f_hx711(COUNT_GPIO_HX711, (byte*)f_hx711_data_pins, GPIO_PIN_HX711_CLK);

// Read pots for joystick emulation.
static void service_js_emu_pots() {
	fori(2) 		// Only emulate x, y. 
		REGS[REGS_IDX_JS_EMU_X + i] = analogRead(GPIO_PIN_ADC_EMU_JS_X + i);
	f_js_emu_ready = true;
}

// For emulation we have a simple abstract interface with a single operation which supplies data at 11Hz rate.
const int32_t JS_EMU_MAX = 8390L;
bool read_raw(int32_t* data) {	
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_JOYSTICK_EMULATION) {
		if (f_js_emu_ready) {
			f_js_emu_ready = false;
			fori(2) {		// Only emulate x, y. 
				// Rescale input 0..1023 to 0..(2*8.39E3) (maximum gui sensitivity). Max num. = 1023*2*8390*125=2145742500.
				data[i] = utilsRoundedDivide((int32_t)REGS[REGS_IDX_JS_EMU_X + i] * 2 * JS_EMU_MAX * 125L, 1023L * 125L);
			}
			data[2] = 0;
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
	
	int32_t raw[COUNT_GPIO_HX711];		// For raw readings.
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
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_JOYSTICK_EMULATION) { // Simple canned zero for emulation, since we know where zero is. 
		f_hx711_data[0].offset = f_hx711_data[1].offset = JS_EMU_MAX;
		f_hx711_data[2].offset = 0L;
	}
	else {																 // Take filtered values as zero point. 
		fori(COUNT_GPIO_HX711) 
			f_hx711_data[i].offset = f_hx711_data[i].filtered;
	}
}


void driverPrintHx711Data(const int32_t* data) {
	fori (COUNT_GPIO_HX711) {
		GPIO_SERIAL_CONSOLE.print(data[i]);
		if (i < COUNT_GPIO_HX711 - 1) GPIO_SERIAL_CONSOLE.print(F(" "));
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
	init_hx711();
	init_relays();
}

void driverService() {
	runEveryU16(JS_EMU_PERIOD_MS) service_js_emu_pots();
	runEveryU16(50) service_relays();
    service_hx711();
}

// This has to go somewhere. 
uint16_t threadGetTicks() { return (uint16_t)millis(); }

