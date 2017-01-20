# ArcadeStick
[![ArcadeStick Finished Photo](https://github.com/norpchen/ArcadeStick/blob/master/Extras/photo%20of%20finished%20ArcadeStick.jpg)
Arduino Leonardo based HID joystick / button panel with many features.
*	a digital joystick with special 4-way-only emulation mode for certain arcade game emulators
*		7+ buttons
*		autofire with button assignment and speed control
*		RGB status LED Neopixel
*		output joystick as buttons, digital (d-hat) and analog control values 
*		any number of alternate button mappings to emulate different game pads
*		user selectable control options (see programming mode below):
	* forced four way or 8 way joystick	
	* dpad mode
	* analog stick emulation
	* flip LR shoulder button mappings
	* change from LR shoulder to trigger buttons
	* NKEY rollover -- no ghosting or limit to how many buttons can be pressed at the same time.

## Physical hardware and layout
You will need 
* one Arduino Leonardo 
* a case or box of some form
* buttons buttons buttons
* a joystick (or 4 more buttons)
* One neopixel (WS2812) RGB LED (optional, but recommended)
* slider switch and potentiometer for autofire (optional)
* wires, resistors, etc. See the schematic in the extras folder

There are currently 3 unassigned analog arduino pins which can be used for additional buttons (3 analog A3, A4, A5).  You could use these to connect a true analog stick, for example. 

All digital pins are in use in the sketch as is. Digital pins D0 and D1 are not referenced in the code, but they are used for the UART / USB communication and are not available for general use.

If you'd like to add many more buttons, you could upgrade to an Arduino Mega and change the HID library to the one for the Mega.  

Or you could connect several buttons together on a single analog input (with a pull up resistor) using different pull-down resistors, (creating a unique voltage divider network for each button) and using the AnalogButtons class in my [button library](https://github.com/norpchen/Buttons)  However, these analog buttons _do not support multiple simultaneous presses_ (unlike the digital buttons which can all be pressed at the same time).  Therefore make sure the buttons you connect this way do not require that.  

Each instance of the AnalogButton class uses one analog pin and can support at least 8, likely 16 (maybe more?) buttons -- the number depends on your choice of resistor values and their precision (more information is available on the button library page).  With three unused analog pins (A3, A4, A5) available, this gives you up to 48 buttons (of which only one from each set of 3 can be pushed together).  If you do not want the autofire function, that frees up two analog pins, bringing the total to 80. If you move the LED to a digital pin, that frees another analog pin for a total of 96. Really at that point you should be looking at a matrix scanning arrangement to remain sane.


[![Schematic](https://github.com/norpchen/ArcadeStick/blob/master/Extras/schematic.png)


##Dependencies
* 	EEPROMVar http://playground.arduino.cc/Profiles/AlphaBeta 
* 	Nico Hood's HID library 2.4 or higher https://github.com/NicoHood/HID
* 	Task Action http://playground.arduino.cc/Code/TaskAction
*	Adafruit's Neopixel library https://github.com/adafruit/Adafruit_NeoPixel
*	My own button library https://github.com/norpchen/Buttons
*	My neopixel color support library https://github.com/norpchen/my_color

Built against Arduino 1.8.1, should work with older versions as well as long as they are above 1.6.6 (an HID library requirement). 

The output button mapping is set up to emulate a PS3 controller, but you can change button mappings to recreate any HID gamepad device.

Note that the code and documentation refer to buttons by colors. Obviously you don't have to follow the same color scheme, just rename the definitions in the enum. (also change the LED flashing colors to match your new colors?)

##Programming mode
Programming mode is a special mode where various settings of the controller can be adjusted by the user. Settings will be saved to eeprom and restored on boot.
To enter programming mode, press and hold down both **START & SELECT** until the status LED goes black. Then release **START & SELECT** and the status color will flash multi colors while in _programming mode_ 

Once in programming mode, you can set the status LED brightness adjusting the autofire rate knob

Press any of the following buttons to make changes, which will be confirmed with the button color flashes (matching the button that controls them).  One blink is OFF, two blinks is ON.
*	GREEN:  Toggle between 8-way (normal) and software emulated 4-way joystick (default is 8 way)
*	RED: 	Toggle analog joystick mode.  (default is on)
*	BLUE: 	Toggle D-hat joystick mode.  (default is on)
*	PURPLE: Toggle buttons joystick mode (default is on, although output buttons are not assigned)
*	WHITE: Step through the alternate button mappings.  The current alternate option is to switch left and right shoulder buttons to left and right triggers (default is the primary (first) mapping). 
*	YELLOW: Toggle swap left and right shoulder buttons (yellow and white buttons) (default is off) 
*	MOVE THE JOYSTICK IN A FULL CIRLE: will reset all settings to ‘factory defaults’  -- blinks cyan and orange many times.  Clockwise or counterclockwise doesn’t matter.

On startup, the ArcadeStick will report its current settings by flash by their colors (once for off, twice for on).

To exit programming mode, hold down both **START & SELECT** again.

##Autofire
Autofire mode will repeatedly send button commands as long as the button is held down. It can be assigned to one of the four main fire buttons (green, red, blue, or purple) through the four-way selector slide switch.  The knob will control the rate and turn off autofire.
