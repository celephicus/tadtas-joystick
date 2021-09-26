#ifndef GPIO_LOCAL_H__
#define GPIO_LOCAL_H__

// Joystick.
enum {
	GPIO_JS_AXIS_X,
	GPIO_JS_AXIS_Y,
	GPIO_JS_AXIS_Z,
	GPIO_JS_AXIS_COUNT,
};

// How many relays?
enum { GPIO_COUNT_RLY = 4 };

#if CFG_WANT_JS_EMULATION		

#if !defined(ARDUINO_AVR_PRO)
#error "JS Emulation only on Pro Mini target."
#endif

/*	PINOUT FOR ATMEGA328P
    =====================
	
TXO/D1	  PD1	TXD/PCINT17             Input from external serial for bootloader
RXI/D0    PD0	RXD/PCINT16             Output to external serial for bootloader
D2        PD2	INT0/PCINT18            
D3        PD3	PCINT19/OC2B/INT1       PWM
D4        PD4	PCINT20/XCK/T0          Timer 0 used for Arduino tick. 
D5        PD5	PCINT21/OC0B/T1         PWM (Timer 0 used for Arduino tick)
D6        PD6	PCINT22/OC0A/AIN0       PWM (Timer 0 used for Arduino tick)
D7        PD7	PCINT23/AIN1            
D8        PB0	PCINT0/CLKO/ICP1        
D9        PB1	PCINT1/OC1A             PWM
D10       PB2	PCINT2/SS/OC1B          PWM
D11/MOSI  PB3	PCINT3/OC2A/MOSI        [PWM] INPUT during SPI programming. ISP/4
D12/MISO  PB4	PCINT4/MISO             OUTPUT during SPI programming.      ISP/1
D13/SCK   PB5	SCK/PCINT5              INPUT during SPI programming, LED.       ISP/3
A0        PC0	ADC0/PCINT8             
A1        PC1	ADC1/PCINT9             
A2        PC2	ADC2/PCINT10            
A3        PC3	ADC3/PCINT11            
A4        PC4	ADC4/SDA/PCINT12        
A5        PC5	ADC5/SCL/PCINT13        
A6        ADC6	ADC6                    Analogue input only.
A7        ADC7	ADC7                    Analogue input only.

RST: ISP/5, GND: ISP/6, VCC: ISP/2.
*/

enum {
	// Misc.
	GPIO_PIN_DEBUG_LED_1 = 			2,			// No LED, monitor with CRO. 
	
	// Serial pins (not used directly).
	GPIO_PIN_CONSOLE_TXO = 			1,
	GPIO_PIN_CONSOLE_RXI = 			0,
	
	// Pots for Emulated joystaick.
	GPIO_PIN_ADC_EMU_JS_X = 		A0,
	GPIO_PIN_ADC_EMU_JS_Y = 		A1,
	
	// Relays.
	GPIO_PIN_RLY_0 =					8,
	GPIO_PIN_RLY_1 =					9,
	GPIO_PIN_RLY_2 =					10,
	GPIO_PIN_RLY_3 =					11,
	
	// I2C interface (setup by Wire/TWI lib).
//	GPIO_PIN_SDA = 					A4,
//	GPIO_PIN_SCL = 					A5,

	// SPI interface (setup by SPI lib).
//	GPIO_PIN_MISO = 				12,
//	GPIO_PIN_SCK = 					13,
//	GPIO_PIN_MOSI = 				11,
};

#define GPIO_PINS_UNUSED 3, 4, 5, 6, 7, 12, 13, A2, A3, A4, A5, A6

// Generic debug LED.
#define GPIO_WANT_DEBUG_LED 1
#define GPIO_PORT_DEBUG_LED	B
#define GPIO_BIT_DEBUG_LED 5

// Extra debug LED.
#define GPIO_PORT_DEBUG_LED_1 D
#define GPIO_BIT_DEBUG_LED_1 2
GPIO_DECLARE_PIN_ACCESS_FUNCS(DebugLed1, GPIO_PORT_DEBUG_LED_1, GPIO_BIT_DEBUG_LED_1);

// Serial ports.
#define GPIO_SERIAL_CONSOLE				 Serial

#else 	// #if CFG_WANT_JS_EMULATION	

#if !defined(ARDUINO_AVR_MICRO)
#error "Strain Gauge JS only on Pro Micro target."
#endif

/*
 Pin Assignments for ATMega32U4. 
D0/RXI		PD2	(RXD1/AIN1/INT2)
D1/TXO		PD3	(TXD1/INT3)
D2			PD1	(SDA/INT1)
D3			PD0	(OC0B/SCL/INT0)
D4/A6		PD4	(ICP1/ADC8)
D5			PC6	(OC3A/#0C4A)
D6/A7		PD7	(T0/OC4D/ADC10)
D7			PE6	(INT.6/AIN0)
D8			PB4	(ADC11/PCINT4)
D9/A8		PB5	(PCINT5/OC1A/#OC4B/ADC12)
D10			PB6	(PCINT6/OC1B/OC4B/ADC13)
D11			PB7	(PCINT7/OC0A/OC1C/#RTS)	(not used)
D12/A10		PD6	(T1/#OC4D/ADC9) 	(not used)
D13			PC7	(ICP3/CLK0/)0C4A)	(not used)
D14			PB3	(PDO/PCINT3/MISO)	ISP/1
D15			PB1	(PCINT1/SCLK)		ISP/3
D16			PB2	(PDI/PCINT2/MOSI)	ISP/4 
A0			PF7	(ADC7/TDI)			JTAG/9
A1			PF6	(ADC6/TDO)          JTAG/3
A2			PF5	(ADC5/TMS)          JTAG/5
A3			PF4	(ADC4/TCK)          JTAG/1
A4			PF1	(ADC1)				(not used)
A5			PF0	(ADC0)				(not used)
D17/RXLED	PB0	(SS/PCINT0)			Yellow
TXLED		PD5	(XCK1/#CTS)			Green
GND			PE2	(#HWB)

RST: ISP/5 JTAG/6
GND: ISP/6 JTAG/2 JTAG/10
VCC: ISP/2 JTAG/4
*/

enum {
	// Misc. 
	// GPIO_PIN_DEBUG_LED = 			???,		// Green 'TX' LED on board. Note returned to VCC so inverted.
	GPIO_PIN_DEBUG_LED_1 = 			17,			// Yellow 'RX' LED on board. 
	
	// HX711
	GPIO_PIN_HX711_CLK = 			4,
	GPIO_PIN_HX711_DT_X = 			5,
	GPIO_PIN_HX711_DT_Y = 			6,
	GPIO_PIN_HX711_DT_Z = 			7,
	
	// Serial pins for port Serial.
//	GPIO_PIN_CONSOLE_TXO = 			1,
//	GPIO_PIN_CONSOLE_RXI = 			0,
	
	// Relays.
	GPIO_PIN_RLY_0 =					8,
	GPIO_PIN_RLY_1 =					9,
	GPIO_PIN_RLY_2 =					10,
	GPIO_PIN_RLY_3 =					11,

	// I2C interface (setup by Wire/TWI lib).
//	GPIO_PIN_SDA = 					2,
//	GPIO_PIN_SCL = 					3,

	// SPI interface (setup by SPI lib).
//	GPIO_PIN_MISO = 				14,
//	GPIO_PIN_SCK = 					15,
//	GPIO_PIN_MOSI = 				16,
	
	// JTAG
	GPIO_PIN_JTAG_TDI =				A0,
	GPIO_PIN_JTAG_TDO =				A1,
	GPIO_PIN_JTAG_TMS =				A2,
	GPIO_PIN_JTAG_TCK =				A3,
};

#define GPIO_PINS_UNUSED 0, 1, 2, 3, 12, 13, 14, 15, 16, A0, A1, A2, A3, A4, A5			
							
// Generic debug LED.
#define GPIO_WANT_DEBUG_LED 1
#define GPIO_PORT_DEBUG_LED	D
#define GPIO_BIT_DEBUG_LED 5

// Extra debug LED. 
#define GPIO_PORT_DEBUG_LED_1 B
#define GPIO_BIT_DEBUG_LED_1 0
GPIO_DECLARE_PIN_ACCESS_FUNCS(DebugLed1, GPIO_PORT_DEBUG_LED_1, GPIO_BIT_DEBUG_LED_1);

// Serial ports.
#define GPIO_SERIAL_CONSOLE				 Serial1

#endif 	// #if CFG_WANT_JS_EMULATION	

#endif	// GPIO_LOCAL_H__
