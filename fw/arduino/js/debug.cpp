#include <Arduino.h>

#include "project_config.h"
#include "debug.h"
#include "avr/wdt.h"

FILENUM(201);  // All source files in common have file numbers starting at 200. 

// Watchdog
//

// Place in section that is not zeroed by startup code. Then after a wd restart we can read this, any set bits are the masks of modules that did NOT refresh in time.
static watchdog_module_mask_t f_wdacc __attribute__ ((section (".noinit")));   

static void setup_watchdog() { 
#if CFG_WATCHDOG_ENABLE											
	wdt_enable(CFG_WATCHDOG_TIMEOUT);
#else
	wdt_disable();
#endif

	f_wdacc = 0;						// Force immediate watchdog reset.
    OCR0A = 0xAF;						// Setup timer 0 compare to a point in the cycle where it won't interfere with timing. Timer 0 overflow is used by the Arduino for millis(). 
    TIMSK0 |= _BV(OCIE0A);				// Enable interrupt. 
}

ISR(TIMER0_COMPA_vect) {				// Kick watchdog with timer mask.
    debugKickWatchdog(DEBUG_WATCHDOG_MASK_TIMER);
}

// Mask for all used modules that have watchdog masks. Note use of 8  byte integer to avoid overflow if count+2 is equal to the size of the type.
static const watchdog_module_mask_t DEBUG_WATCHDOG_MASK_ALL = (watchdog_module_mask_t)( ((uint64_t)1 << (CFG_WATCHDOG_MODULE_COUNT+2)) - 1 );

void debugKickWatchdog(watchdog_module_mask_t m) {
    f_wdacc &= ~m;
    if (0 == f_wdacc) {
        wdt_reset();
        f_wdacc = DEBUG_WATCHDOG_MASK_ALL;
    }
}

#if 0
// How to check if we restarted as a result of a watchdog? Ordinarily we could read the MCUSR, but the bootloader overwrites it.
//  So we check for a signature in RAM that is not cleared by the startup code.
enum { STARTUP_SIGNATURE = 0x781A };
static uint16_t f_startup_signature __attribute__ ((section (".noinit")));   
static void check_startup_signature() {
    f_is_watchdog = (STARTUP_SIGNATURE == f_startup_signature);
    f_startup_signature = STARTUP_SIGNATURE;
}
#endif

uint8_t debugWatchdogInit(watchdog_module_mask_t* old_mask) {
	uint8_t mcusr = MCUSR;		// (JTRF) WDRF BORF EXTRF PORF , JTRF only on JTAG parts
	MCUSR = 0;					// Necessary to disable watchdog on some processors. 
	if (NULL != old_mask)		// Grab watchdog mask from possible previous run.
		*old_mask = f_wdacc;
    setup_watchdog();
	return mcusr;
}
	
bool debugIsRestartWatchdog(uint16_t rst) { return !!(rst & _BV(WDRF)); }
