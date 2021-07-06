#include <Arduino.h>

#include "gpio.h"
#include "utils.h"

void gpioInit() {
	static const uint8_t UNUSED_PINS[] PROGMEM = { GPIO_PINS_UNUSED };
	fori(ELEMENT_COUNT(UNUSED_PINS)) {
		const uint8_t pin = pgm_read_byte(&UNUSED_PINS[i]);
		pinMode(pin, OUTPUT);
	}
}

	