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
* a joystick (or more buttons)
* One neopixel (WS2803) RGB LED (optional, but recommended)
* slider switch and potentiometer for autofire (optional)
* wires, resistors, etc. See the schematic in the extras folder

There are currently 5 unassigned arduino pins which can be used for additional buttons (2 digital, 3 analog)


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
