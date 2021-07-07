#ifndef GPIO_LOCAL_H__
#define GPIO_LOCAL_H__

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
	
	// Serial pins (not used directly).
	GPIO_PIN_CONSOLE_TXO = 			1,
	GPIO_PIN_CONSOLE_RXI = 			0,
	
	// I2C interface (setup by Wire/TWI lib).
	GPIO_PIN_SDA = 					2,
	GPIO_PIN_SCL = 					3,

	// SPI interface (setup by SPI lib).
	GPIO_PIN_MISO = 				14,
	GPIO_PIN_SCK = 					15,
	GPIO_PIN_MOSI = 				16,
	
	// JTAG
	GPIO_PIN_JTAG_TDI =				A0,
	GPIO_PIN_JTAG_TDO =				A1,
	GPIO_PIN_JTAG_TMS =				A2,
	GPIO_PIN_JTAG_TCK =				A3,
};

#define GPIO_PINS_UNUSED 			
							
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

// HX711 amplifiers.
enum {
	GPIO_HX711_X,
	GPIO_HX711_Y,
	GPIO_HX711_Z,
	COUNT_GPIO_HX711,
};

#endif	// GPIO_LOCAL_H__
