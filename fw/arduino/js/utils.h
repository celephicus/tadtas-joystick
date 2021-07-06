#ifndef UTILS_H__
#define UTILS_H__

// Checked deference from PROGMEM. Remember Overloading is usually better than function template specialisation. 
#if 0
static inline uint8_t  utilsReadProgmem(const uint8_t* addr)  { return pgm_read_byte(addr); } 
static inline uint16_t utilsReadProgmem(const uint16_t* addr) { return pgm_read_word(addr); } 
static inline uint32_t utilsReadProgmem(const uint32_t* addr) { return pgm_read_dword(addr); }
static inline uint64_t utilsReadProgmem(const uint64_t* addr) { uint64_t v; memcpy_P(&v, addr, sizeof(uint64_t)); return v; }
#endif

template <typename T>
static inline T utilsReadProgmem(const T* addr) { 
	switch (sizeof(T)) {
		case 1: return (T)pgm_read_byte(addr); 
		case 2: return (T)pgm_read_word(addr); 
		case 4: return (T)pgm_read_dword(addr); 
		default: {
			T x;
			memcpy_P(&x, addr, sizeof(T));
			return x;
		}
	}
}
// I can't believe how often I stuff up using millis() to time a period. So as usual, here's a function to do timeouts.
template <typename T>
void utilsStartTimer(T &then) { then = (T)millis(); }

template <typename T>
bool utilsIsTimerDone(T &then, T timeout) { return ((T)millis() - then) > timeout; }

#define CONCAT2(s1_, s2_) s1_##s2_
#define CONCAT(s1_, s2_) CONCAT2(s1_, s2_)

// The worlds simplest scheduler, run a block every so often. Requires RUN_EVERY_TIME_T & RUN_EVERY_GET_TIME be defined.
// Stolen from a discussion on some Arduino forum. 
#define runEvery(t_) for (static RUN_EVERY_TIME_T then; (RUN_EVERY_GET_TIME() - then) >= (RUN_EVERY_TIME_T)(t_); then += (RUN_EVERY_TIME_T)(t_))

// How many elements in an array?
#define ELEMENT_COUNT(x_) (sizeof(x_) / sizeof((x_)[0]))

// When you need to loop an index over a small range...
#define fori(limit_) for (uint8_t i = 0; i < (limit_); i += 1)
    
// Perhaps an inner loop as well...
#define forj(limit_) for (uint8_t j = 0; j < (limit_); j += 1)

// Fletcher16 checksum modulo 255, note that it will not notice a 00 changing to ff. 
uint16_t utilsChecksumFletcher16(uint8_t const *data, size_t count);

// Simple very fast checksum for EEPROM, guaranteed to note changing values to 0x00 or 0xff, which are the most likely errors. 
typedef struct { uint8_t sum1, sum2; } utils_checksum_eeprom_state_t;
void utilsChecksumEepromInit(utils_checksum_eeprom_state_t* s);
void utilsChecksumEepromUpdate(utils_checksum_eeprom_state_t* s, uint8_t const *data, size_t count);
uint16_t utilsChecksumEepromGet(utils_checksum_eeprom_state_t* s);

// Simple version of the above. 
uint16_t utilsChecksumEeprom(uint8_t const *data, size_t count);

// Return lowest set bit as a mask. 
template <typename T>
T utilsLowestSetBit(T x) { return x & (x - 1); }

// Increment/decrement a variable within limits. Returns true if value updated. 
template <typename T, typename U> // T is usually unsigned, U must be signed. 
bool utilsBump(T* val, U incdec, T min, T max, bool rollaround=false) {
    U bumped_val = *val + incdec; // Assume no overflow. 
    if (bumped_val < (U)min) 
        bumped_val = rollaround ? (U)max : (U)min; 
    if (bumped_val > (U)max) 
        bumped_val = rollaround ? (U)min : (U)max; 
    bool changed = (*val != (T)bumped_val); /* Now the result must be in range so we can compare it to the original value. */ 
    if (changed) 
        *val = (T)bumped_val; 
    return changed;
}

// Increment/decrement a variable within limits, given a target value and a delta value.
template <typename T> 
bool utilsSlew(T* val, T target, T slew) {
	if (*val != target) {
		const bool finc = (*val < target);
		T new_val = *val + (finc ? +slew : -slew);
		if (finc) {
			if ((new_val < *val) || (new_val > target))
				new_val = target;
		}
		else {
			if ((new_val > *val) || (new_val < target))
				new_val = target;
		}
		*val = new_val;
		return true;
	}
	return false;
}

// Increment or decrement the value by an amount not exceeding the delta value, within the inclusive limits. Returns true if value updated.
#define utilsBumpU8 utilsBump<uint8_t, int8_t>
#define utilsBumpU16 utilsBump<uint16_t, int16_t>
#define utilsBumpU32 utilsBump<uint32_t, int32_t>

// Limit a value to a maximum.
template <typename T>
T utilsLimitMax(T value, T max) {
	return (value > max) ? max : value; 
}
#define utilsLimitMaxU8 utilsLimitMax<uint8_t>
#define utilsLimitMaxU16 utilsLimitMax<uint16_t>
#define utilsLimitMaxU32 utilsLimitMax<uint32_t>

// Limit a value to a minimum.
template <typename T>
T utilsLimitMin(T value, T min) {
	return (value < min) ? min : value; 
}
#define utilsLimitMinU8 utilsLimitMin<uint8_t>
#define utilsLimitMinU16 utilsLimitMin<uint16_t>
#define utilsLimitMinU32 utilsLimitMin<uint32_t>

// Clip the value to the inclusive limits.
template <typename T>
T utilsLimit(T value, T min, T max)  {
	if (value > max) return max; else if (value < min) return min; else return value;
}
#define utilsLimitU8 utilsLimit<uint8_t>
#define utilsLimitU16 utilsLimit<uint16_t>
#define utilsLimitU32 utilsLimit<uint32_t>
#define utilsLimitI8 utilsLimit<int8_t>
#define utilsLimitI16 utilsLimit<int16_t>
#define utilsLimitI32 utilsLimit<int32_t>

// Return the absolute value. 
template <typename T>
T utilsAbs(T x)  {
	return (x < 0) ? -x : x;
}
#define utilsAbsI8 utilsAbs<int8_t>
#define utilsAbsI16 utilsAbs<int16_t>
#define utilsAbsI32 utilsAbs<int32_t>

// Update the bits matching the mask in the value. Return true if value has changed. 
template <typename T>
bool utilsUpdateFlags(T* flags, T mask, T val) { const T old = *flags; *flags = (*flags & ~mask) | val; return (old != *flags); }

// Update the bits matching the mask in the value. Return true if value has changed. 
template <typename T>
bool utilsWriteFlags(T* flags, T mask, bool s) { const T old = *flags; if (s) *flags |=  mask; else *flags &= ~mask; return (old != *flags); }

// Integer division spreading error.
template <typename T>
T utilsRoundedDivide(T num, T den) {
    return (num + den/2) / den;
}	

// Absolute difference for unsigned types.
template <typename T>
T utilsAbsDiff(T a, T b) {
    return  (a > b) ? (a - b) : (b - a);
}
	
// Rescale a scaled value, assuming no offset.
template <typename T, typename U>  // T is unsigned, U can be larger type that can hold input*max-output. 
T utilsRescale(T input_value, T max_input, T max_output) {
    return (T)utilsRoundedDivide<U>((U)input_value * (U)max_output, (U)max_input);
}
#define utilsRescaleU8 utilsRescale<uint8_t, uint16_t>
#define utilsRescaleU16 utilsRescale<uint16_t, uint32_t>

/* Multiple threshold comparator, use like this:

static const uint16_t PROGMEM THRESHOLDS[] = { 50, 100, 500 };
enum { HYSTERESIS = 20 };
uint8_t level;
	
After executing 	
	multiThreshold(THRESHOLDS,  ELEMENT_COUNT(THRESHOLDS),	HYSTERESIS, &level, x);
	
	as x ranges from 0 .. 600 .. 0, level changes so:

0..49	0
50.. 99 1
100..499 2
500..600 3	
600.. 480 3
479.. 80 2
79..30 1
29..0 0

Returns true if the level has changed.
*/
bool utilsMultiThreshold(const uint16_t* thresholds, uint8_t count, uint16_t hysteresis, uint8_t* level, uint16_t val);

/* Simple filter code, y[n] = (1-a).y[n-1] + a.x[n]. Replacing the last term by a.(x[n]+x[n-1])/2 makes it look like a first order RC.
	Rewrite:   y[n]	= y[n-1] - a.y[n-1] + a.x[n]
					= y[n-1] - a.(y[n-1] + x[n])
	In order not to lose precision we do the calculation scaled by a power of 2, 2^k, which can be calculated by shifting. This does require
	an accumulator to hold the scaled value of y.
*/
template <typename T>
void utilsFilterInit(T* accum, uint16_t input, uint8_t k) { utilsFilter(accum, input, k, true); }

template <typename T>
uint16_t utilsFilter(T* accum, uint16_t input, uint8_t k, bool reset) {
	*accum = reset ? ((T)input << k) : (*accum - (*accum >> k) + (T)input); 
	return (uint16_t)(*accum >> k);
}

// Simple implementation of strtoul for unsigned ints. String may have leading whitespace and an optional leading '+'. Then digits up to one less than the 
//  base are converted, until the first illegal character or a nul is read. Then, if the result has not rolled over, the function returns true, and the result is set to n.
//  If endptr is non-NULL, it is set to the first illegal character. 
bool utilsStrtoui(unsigned* n, const char *str, char **endptr, unsigned base);

#endif
