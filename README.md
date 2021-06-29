# tadtas-joystick
A USB mouse using a force-sensing joystick.

## History
[TADTas](http://tadtas.org.au) are a non-profit organisation based in Hobart, Tasmania, Australia that provides custom equipment/aids for people with disabilities. I was introduced by a mutual friend to the Technical Co-ordinator Tony Sprent who told me of a stalled project of his to make a computer mouse using strain gauges, which sense force without actually moving (much), to assist a client with very limited movement. 

## Existing Hardware
The hardware had been built with 3 orthogonally mounted strain gauges of 1Kg capacity connected to 3 HX711 amplifier modules, in turn connected to an Arduino Uno. Photos below, also including an Arduino Pro Micro (repackaged Leonardo) that was not supplied.

![1](/docs/tadtas-joystick-hw-orig-1.jpg) ![2](/docs/tadtas-joystick-hw-orig-2.jpg) ![3](/docs/tadtas-joystick-hw-orig-3.jpg) 

## Strain Gauge Amplifiers
The existing amplifiers I decided to reuse, as they have acceptable performance and there is an Arduino library to drive them, not that they are very difficult to interface with. An important point is that they operate at 5V, rather than 3.3V. The modules used are Chinese knockoffs of a ![Sparkfun design](https://www.sparkfun.com/products/13879), see their page for further information.


## New Hardware
I decided to replace the Arduino Uno as a start as it can only be a USB serial port, and cannot emulate a PC mouse. I have used another controller, the Arduino Pro Micro in the past, as it has an onboard USB controller that can emulate just about any USB device, including the HID ("Human Interface Device") that is the USB profile that is used by mice. I asked a question on my favourite local supplier's forum ![here](https://forum.core-electronics.com.au/t/arduino-for-mouse-emulation/10406) and got some helpful information, including a response from technical support that this board was suitable.

I ended up ordering this part ![Pro Micro - 5V/16MHz](https://core-electronics.com.au/pro-micro-5v-16mhz.html) since it was in stock. I would have preferred to order a version that has the 6 pin ISP header on board, since I usually do my Arduino software development using a hardware debugger connected via this connector, that allows me to load and step through the code, examine variables & memory etc. However it is not much trouble to wire one up. Searching ![Core Electronics](https://core-electronics.com.au) for "32U4" (the Atmel AVR processor used on these boards) will turn up a few options. The ![A-Star 32U4 Micro](https://core-electronics.com.au/a-star-32u4-micro.html) looks a good option as it is tiny.

## IO Pin Assignment.
Since the HX711 library just uses bit bashing to talk to the chips, it doesn't really matter which pins are used, so I just assigned them from D4 up, leaving D0, D1 for the serial port & D2, D3 for I2C, since these often come in handy. So:

* D4, D5: X axis CLK, DATA
* D6, D7: Y axis CLK, DATA
* D8, D9: Y axis CLK, DATA
