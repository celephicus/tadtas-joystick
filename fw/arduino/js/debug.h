#ifndef DEBUG_H_
#define DEBUG_H_

/* Some very simple assertions and runtime error functions. These allow the use of assert() to cause an abort, recording file and line information
	in an accessible place that can be examined with a debugger. Or your function can display the information. 
*/	

// From Niall Murphy article "Assert Yourself" Gives each file a guaranteed unique number by misusing the linker.
// Usage: FILENUM(33);
#define FILENUM(num_) \
    void _dummy##num_(void) {} \
    enum { F_NUM = (num_) }

// Define this somewhere, note what it actually does it up to you, but you can't return. Your program is unlikely to work anymore.
void debugRuntimeError(uint8_t fileno, uint16_t lineno, uint8_t errorno) __attribute__((noreturn));

// Convenience function to raise a runtime error...
#define RUNTIME_ERROR(_errorno) debugRuntimeError(F_NUM, __LINE__, (_errorno))

// Our assert macro does not print the condition that failed, contrary to the usual version. This is intentional, it conserves
// string space on the target, and the user just has to report the file & line numbers.
#ifndef NDEBUG  /* A release build defines this macro, a debug build defines DEBUG. */ 
#define ASSERT(cond_) do { 				\
    if (!(cond_)) 						\
        RUNTIME_ERROR(0); 				\
} while (0)
#else   
    #define ASSERT(cond_) do { /* empty */ } while (0)
#endif

// When something must be true at compile time...
#define STATIC_ASSERT(expr_) extern int error_static_assert_fail__[(expr_) ? 1 : -1] __attribute__((unused))

/* Watchdog manager -- the idea is that the mainloop calls debugKickWatchdog() with DEBUG_WATCHDOG_MASK_MAINLOOP, and
	an interrupt timer routine calls it with one of the other masks. Make sure you set DRIVER_WATCHDOG_MASK_ALL to the bitwise
	OR of all the masks you use. Then the watchdog will only be kicked when the each mask has been called. This guards against
	eiither the mainloop locking up or the timer interrupt not working. You should always have a timer interrupt kicking the watchdog,
	imagine if interrupts got disabled and stayed that way. */

// Decide how big to make the watchdog mask.
#if CFG_WATCHDOG_MODULE_COUNT > 30
# error CFG_WATCHDOG_MODULE_COUNT too big
#elif CFG_WATCHDOG_MODULE_COUNT > 14
	typedef uint32_t watchdog_module_mask_t;
#elif CFG_WATCHDOG_MODULE_COUNT > 6
	typedef uint16_t watchdog_module_mask_t;
#else
	typedef uint8_t watchdog_module_mask_t;
#endif

// We always have at least these two masks.
const watchdog_module_mask_t DEBUG_WATCHDOG_MASK_MAINLOOP = (watchdog_module_mask_t)1 << 0;
const watchdog_module_mask_t DEBUG_WATCHDOG_MASK_TIMER = (watchdog_module_mask_t)1 << 1;
#define MK_MODULE_MASK(n_, name_) const watchdog_module_mask_t name_##n_ = (watchdog_module_mask_t)1 << ((n_)+2);

#include "repeat.h" // The weird repeat macro. 
// REPEAT(count, macro, data) => macro(0, data) macro(1, data) ... macro(count - 1, data)
REPEAT(CFG_WATCHDOG_MODULE_COUNT, MK_MODULE_MASK, DEBUG_WATCHDOG_MASK_USER_)

/* Call very early in the startup code, this records the value of MCUSR and then clears it, which is essential for disabling the watchdog, if 
	that's what you want. Returns the value of the MCUSR register. If the pinter argument is non-NULL then it is set to the value of the 
	watchdog mask, which could help determine what caused the last restart, if it was due to the watchdog. 
	It also sets up the watchdog. */
uint8_t debugWatchdogInit(watchdog_module_mask_t* old_mask);

// Call every so often to kink the 'dog...
void debugKickWatchdog(watchdog_module_mask_t m);

// Check if the restart code returned by debugWatchdogInit() indicated a watchdog restart.
bool debugIsRestartWatchdog(uint16_t rst); 

#define DEBUG_CONSOLE_COMMANDS																																\
	case /** RESTART **/ 0X7092: while (1) /* empty */; break;																								\
	case /** RT-ERROR **/ 0XADB6: RUNTIME_ERROR(console_u_pop()); break;																					\

#endif // DEBUG_H_
