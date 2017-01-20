# ArcadeStick
Arduino Leonardo based HID joystick / button panel with many features.
*	a digital joystick with special 4-way-only emulation mode for certain arcade game emulators
*		7+ buttons
*		autofire with button assignment and speed control
*		RGB status LED Neopixel
*		output joystick as buttons, digital (d-hat) and analog control values 
*		user selectable control schemes (see programming mode below):
** forced four way or 8 way joystick	
** dpad mode
** analog stick emulation
** flip LR shoulder button mappings
** change from LR shoulder to trigger buttons


#Dependencies
* 	EEPROMVar http://playground.arduino.cc/Profiles/AlphaBeta 
* 	Nico Hood's HID library 2.4 or higher https://github.com/NicoHood/HID
* 	Task Action http://playground.arduino.cc/Code/TaskAction
*	Adafruit's Neopixel library https://github.com/adafruit/Adafruit_NeoPixel
*	My own button library https://github.com/norpchen/Buttons
*	My neopixel color support library https://github.com/norpchen/my_color

Built against Arduino 1.8.1, should work with older versions as well as long as they are above 1.6.6 (an HID library requirement). 

The output button mapping is set up to emulate a PS3 controller, but you can change button mappings 

#Programming mode
Programming mode is a special mode where various settings of the controller can be adjusted. Settings will be saved even after the device is unplugged.  You can reset to ‘factory defaults’ by holding down START when you plug in the device.
To enter programming mode, press and hold down both START & SELECT until the status LED goes black. Then release START & SELECT and the status color will flash multi colors while in ‘programming mode’ 
While in programming mode, you can set the status LED brightness using the autofire rate knob
Press any of the following buttons to make changes, which will be confirmed with the button color flashes.  One blink is OFF, two blinks is ON.
	GREEN:  Toggle between 8-way (normal) and software emulated 4-way joystick 
	RED: 	Toggle analog joystick mode.  
	BLUE: 	Toggle D-hat joystick mode.  
	PURPLE: Toggle buttons joystick mode.
	WHITE: Use alternate button mapping mode – when on, it will switch left and right shoulder buttons to left and right triggers
	YELLOW: Swap left and right shoulder buttons (yellow and white buttons)  
	MOVE THE JOYSTICK IN A FULL CIRLE: will reset all settings to ‘factory defaults’  -- blinks cyan and orange many times.  Clockwise or counterclockwise doesn’t matter.
To exit programming mode, hold down both START & SELECT again.
On startup, the current settings will flash by their colors.

#Autofire
Autofire mode will repeatedly send button commands as long as the button is held down. It can be assigned to one of the four main fire buttons (green, red, blue, or purple) through the four-way selector slide switch on the front left of the controller.  The knob will control the rate and turn off autofire.
