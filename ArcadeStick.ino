#include <Arduino.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>				// found at http://playground.arduino.cc/Profiles/AlphaBeta 
#include <HID.h>					// Nico Hood's HID library https://github.com/NicoHood/HID
#include <HID-Settings.h>
#include <HID-Project.h>
#include <Buttons.h>				// https://github.com/norpchen/Buttons
#include <TaskAction.h>				// http://playground.arduino.cc/Code/TaskAction
#include <Adafruit_NeoPixel.h>		// https://github.com/adafruit/Adafruit_NeoPixel
#include <my_color.h>				// https://github.com/norpchen/my_color

	
#define  DEBUG 1

extern Gamepad_ Gamepad;

int EEPROMAddressCounter::availableAddress= 0;

#define MY_EXTENSIONS_TO_EEPROM_VAR 
// ignore this, it's for my own extensions to the EEPROM_VAR library ( which are not helpful here )
#ifdef MY_EXTENSIONS_TO_EEPROM_VAR 

bool EEPROMAddressCounter::restore_enabled = true;
#endif 

//--------------------------------------------------------------------------------------


// SKetch for an Arcade Stick controller with :
//		a digital joystick with special 4-way-only emulation mode for certain arcade game emulators
//		7+ buttons
//		autofire with button assignment and speed control
//		RGB status LED
//		outputjoystick as both digital (d-hat) and analog control values 
//		
//	
//	
//	Things you need to know: 
//	
// OK, the buttons... there's several mappiongs, and it works like this:
// 
//		 Arduino digital pin -> button manager ID -> gamepad controller button
//		 
// ie: 
// read digital pin and convert digital pin number to a button ID
// fire an up or down event with the button ID
// Send a controller button to the host, based on the button ID
// 
// we basically have three levels of a "button" -- 
// 1.	buttons in as they are assigned to arduino pins
// 2.	logical buttons which have context and perhaps special functionality associated (ie: START) 
// 3.	output buttons which are gamepad button definitions based on HID 
// 


// this is the first step -- a table of arduino digital pin numbers that is passed into the button manager 
// so it knows which arduino input pins map to a given button index.  
const unsigned int button_pins[]
{
		7,	
		6,
		8,
		9,
		12,	
		11,
		10,
		13,
		5,
		4,
		2, 
		3
};

// pins not used in this sketch: 0, 1, A3, A4, A5 (in case you want to add more buttons) 

// The button index from the button manager is a number and it's easy to forget which button is which in the code
// This enum provides context / a more understandable way of referencing button manager index values 
// This is the second step -- translating the button IDs into button names for context 
// This is the same order as button_pins[]
enum BUTTON_INDEX
{ 
	BUTTON_RED = 0,
	BUTTON_GREEN,
	BUTTON_BLUE,
	BUTTON_PURPLE ,
	BUTTON_UP,
	BUTTON_LEFT,
	BUTTON_DOWN,
	BUTTON_RIGHT,
	BUTTON_YELLOW,
	BUTTON_WHITE,
	BUTTON_START,
	BUTTON_SELECT,
	BUTTON_MAX
};


// Finally we take the button IDs from the button manager, and figure out which gamepad button # to output the USB port to the host.

// 
// 
// for example: 
//	BUTTON_RED, which is index 0 in button_pins will send a gamepad button 3 to the host
//	
// These are in the order of the button_pins[] and BUTTON_INDEX tables.
// 
// we support alternate tables that the user can switch between if some games don't map correctly
// 
// These tables are for gamepad buttons and not the d-pad, analog sticks, etc. 
// 
// in this case, we're trying to match the PS3 controller button IDs
int output_buttons[13] =  {  

	3, // RED
	4, // GREEN
	2, // BLUE
	0, // PURPLE
	-1, // UP
	-1, // LEFT
	-1, // DOWN
	-1, // RIGHT
	5,  // YELLOW
	6,  // WHITE
	9,  // START 
	10,  // SELECT

};

// a user - selectable alternate which swaps two buttons (the ones mapped to LR shoulder buttons) 
int alternate_output_buttons[13] =   {  

	3, // RED
	4, // GREEN
	2, // BLUE
	0, // PURPLE
	-1, // UP
	-1, // LEFT
	-1, // DOWN
	-1, // RIGHT
	11,  // YELLOW		-- not sure these are correct....
	12,  // WHITE
	9,  // START 
	10,  // SELECT

};

// if you are trying to emulate a specific USB HID gamepad / joystick, you can plug it in to windows and use the gaming controller control panel 
// to determine the output buttons you need.

// you can create more output mapping tables and the stick will step through them every time you press the white button in programming mode
 // and here we track which of the output mapping tables is active
const byte MAX_ALT_TABLES = 2;
int * button_outputs[MAX_ALT_TABLES]=  { output_buttons   ,alternate_output_buttons};
int * current_button_output_table = button_outputs[0];

//---------------------------------------------------------------------------------------------------------------------
Buttons buttons (BUTTON_MAX, button_pins);



// value 0 = analogRead() < analog_slider_thresholds[0]
// value 1 = analogRead() > analog_slider_thresholds[0] && analogRead() < analog_slider_thresholds[1]
// etc.
// these values are for a 4 position 4PST sliding switch.  There is a final value of 1023 implied for the fourth position.
int analog_slider_thresholds [] = { 200, 350, 525} ;


// internaL global counters and timers
unsigned long  button_up_time		=0;
unsigned long  button_down_time		=0;
unsigned long  button_left_time		=0;
unsigned long  button_right_time	=0;
unsigned int programming_mode_counter =0;

#define PIN_STATUS_PIXEL A1
Adafruit_NeoPixel pixel (1,PIN_STATUS_PIXEL,NEO_RGB);


// autofire stuff
// we read the analog knob to set autofire.  But we remap that to a set of fixed autofire intervals so we can have a more meaningful 
// adjustment range (if we used a linear scale, with a max of 1000, we'd have way too much sensitivity at the low end of the range) 
const int AUTOFIRE_RATES_MAX = 13;
int autofire_delay_rates[AUTOFIRE_RATES_MAX] = {  50,  75, 100, 150, 200, 250, 333, 400, 500, 600, 750, 875, 1000};		// in ms
int min_af_val = autofire_delay_rates[0]-20;		// this is the minimum analog reading to consider autofire to be on.  below this, autofire is off.
int autofire_delay;
int autofire_button = -1;
byte autofire_phase  = 0;
my_color autofire_color(np_black);

const int AUTOFIRE_PULSE_DURATION = 45;
#define PIN_AUTOFIRE_ASSIGNMENT_SWITCH  A2
#define PIN_AUTOFIRE_RATE A0

void updateAutofireRate ();
void checkAutofireAssignment ();
void updateAutoFire();

// set up background timer handlers for autofire.
TaskAction check_autofire_controls (updateAutofireRate, 150);
TaskAction autofire (updateAutoFire, 10);

// variables that are saved to EEPROM when changed -- user config stuff
EEPROMVar <unsigned int> pixel_brightness(255);
// do we send joystick as dpad?
EEPROMVar <byte> dpad_mode(1);
// do we send joystick as analog stick emulation?
EEPROMVar <byte> analog_stick_mode(1);

// this mode sends buttons for the joystick movement (instead of dpad, analog, etc)  You need to set the output button assignments for this mode
EEPROMVar <byte> joy_buttons_mode(1);	

// flip the LR buttons 
EEPROMVar <byte> flipped_lr(0);

// use the alternate output button table
EEPROMVar <byte> alt_table(0);


// force four way only joystick emulation
EEPROMVar <byte> fourway(0);


//--------------------------------------------------------------------------------------
void swap (int& a, int& b)
{
	int c = a;
	a = b;
	b = c;
}


//-----------------------------------------------------------------------------------------------------------------

void updateJoystick()
{
	if ((byte) fourway==1) 
	{ 
		updateJoystick4way();
		return;
	}
	byte dpad =0;
	Gamepad.yAxis(0);
	Gamepad.xAxis(0); 
	if (buttons.getButton(BUTTON_UP) ) 
	{ 
		dpad = 1;
		if (analog_stick_mode) Gamepad.yAxis(-0x7fff);
		if (buttons.getButton(BUTTON_LEFT) ) {  dpad = 8;if (analog_stick_mode) Gamepad.xAxis(-0x7fff);}
		else if (buttons.getButton(BUTTON_RIGHT) ) {  dpad = 2; if (analog_stick_mode) Gamepad.xAxis(0x7fff);  } 
	} 
	else if (buttons.getButton(BUTTON_DOWN) ) 
	{ 
		dpad = 5;
		if (analog_stick_mode) Gamepad.yAxis(0x7fff);
		if (buttons.getButton(BUTTON_LEFT) ) {  dpad = 6;if (analog_stick_mode) Gamepad.xAxis(-0x7fff);} 
		else if (buttons.getButton(BUTTON_RIGHT) ) { dpad = 4;  if (analog_stick_mode) Gamepad.xAxis(0x7fff); } 
	} 
	else
	{ 
		if (buttons.getButton(BUTTON_LEFT) ) { dpad = 7; if (analog_stick_mode) Gamepad.xAxis(-0x7fff);} 
		else if (buttons.getButton(BUTTON_RIGHT) ) { dpad = 3; if (analog_stick_mode) Gamepad.xAxis(0x7fff); } 
	} 
	if (dpad_mode) 
		Gamepad.dPad1(dpad);
	Gamepad.write();
}

//-----------------------------------------------------------------------------------------------------------------
// software emulation of digital (not analog) 4 way ONLY controller
// some arcade games in MAME do not like diagonals -- this is a work around for that
// This will send whatever direction is currently held down.  if more than one is active, 
// then only send the *most recently pushed*
// 

void updateJoystick4way()
{
	byte dpad =0;
	Gamepad.yAxis(0);
	Gamepad.xAxis(0); 
	unsigned long last_pressed = 9999999UL;

	char lp = -1;
	if (buttons.getButton(BUTTON_UP) ) 
	{ 
		if (last_pressed > millis() - button_up_time) 
		{ 
			last_pressed = millis() - button_up_time;
			lp = BUTTON_UP;
		} 
	} 

	if (buttons.getButton(BUTTON_DOWN) ) 
	{ 
		if (last_pressed > millis() - button_down_time) 
		{ 
			last_pressed = millis() - button_down_time;
			lp =BUTTON_DOWN;
		} 
	}

	if (buttons.getButton(BUTTON_LEFT) ) 
	{ 
		if (last_pressed > millis()  - button_left_time) 
		{ 
			last_pressed = millis() - button_left_time;
			lp = BUTTON_LEFT;
		} 
	}	
	if (buttons.getButton(BUTTON_RIGHT) ) 
	{ 
		if (last_pressed > millis() - button_right_time) 
		{ 
			last_pressed = millis() - button_right_time;
			lp =BUTTON_RIGHT;
		} 
	}

	// generate the joystick gamepad outputs, with optional analog stick emulation
	switch (lp) 
	{ 
	case BUTTON_UP: 	dpad = 1; if (analog_stick_mode) Gamepad.yAxis(-0x7fff); break;
	case BUTTON_DOWN: 	dpad = 5; if (analog_stick_mode) Gamepad.yAxis(0x7fff); break;
	case BUTTON_LEFT: 	dpad = 7; if (analog_stick_mode) Gamepad.xAxis(-0x7fff); break;
	case BUTTON_RIGHT: 	dpad = 3; if (analog_stick_mode) Gamepad.xAxis(0x7fff); break;
	} 

	if (dpad_mode) 
		Gamepad.dPad1(dpad);
	Gamepad.write();
}


//--------------------------------------------------------------------------------------
// we got a button down -- this is a button index
void buttonDown (byte button_id )
{
#if DEBUG
	Serial.print ("DOWN: ");
	Serial.print (button_id);
	Serial.print (" <");
	Serial.print (current_button_output_table[button_id]);
	Serial.println (">");
#endif

	if (!joy_buttons_mode && (
		( button_id==BUTTON_UP) || 
		( button_id==BUTTON_LEFT) || 
		( button_id==BUTTON_DOWN) || 
		( button_id==BUTTON_RIGHT) ))
	{
		;		// skipping sending buttons for joystick
	} else 
	{ 
		if (current_button_output_table[button_id]>=0) 
			Gamepad.press (current_button_output_table[button_id] ) ;
		Gamepad.write();
	} 

	// start up the autofire timer for this button....
	if (button_id == autofire_button && autofire_delay != 0)
	{
		autofire.Enable (true);
		autofire.SetInterval(AUTOFIRE_PULSE_DURATION);
		autofire.SetTicks(0);
		autofire_phase  = 1;
	}
	switch (button_id)
	{
	case BUTTON_PURPLE: pixel.setPixelColor(1,np_purple); pixel.show(); return;
	case BUTTON_WHITE : pixel.setPixelColor(1,np_white); pixel.show(); return;
	case BUTTON_RED   : pixel.setPixelColor(1,np_red); pixel.show(); return;
	case BUTTON_GREEN : pixel.setPixelColor(1,np_green); pixel.show(); return;
	case BUTTON_YELLOW: pixel.setPixelColor(1,np_yellow); pixel.show(); return;
	case BUTTON_BLUE  : pixel.setPixelColor(1,np_blue); pixel.show(); return;
	case BUTTON_SELECT: pixel.setPixelColor(1,np_orange); pixel.show(); return;
	case BUTTON_START : pixel.setPixelColor(1,np_cyan); pixel.show(); return;
	case BUTTON_UP:		button_up_time		= millis(); break;
	case BUTTON_DOWN:	button_down_time	= millis(); break;
	case BUTTON_LEFT:	button_left_time	= millis(); break;
	case BUTTON_RIGHT:	button_right_time	= millis(); break;
	}
	updateJoystick();
} ;


//--------------------------------------------------------------------------------------

void buttonUp (byte button_id )
{
#if DEBUG
	Serial.print ("UP: ");
	Serial.println (button_id);
#endif
	if (current_button_output_table[button_id] >=0) 
		Gamepad.release (current_button_output_table[button_id] ) ;
	Gamepad.write();
	if (button_id == autofire_button)
		autofire.Enable (false);
	pixel.setPixelColor(1,0);
	pixel.show();
	updateJoystick();
} ;

//--------------------------------------------------------------------------------------
// process change to auto fire rate
void updateAutofireRate ()
{
	checkAutofireAssignment();
	int val = (analogRead  (PIN_AUTOFIRE_RATE) )  ;

	if (val < min_af_val)
	{
		autofire_delay = 0;
		autofire.Enable (false);
	}
	else
	{
		val *= AUTOFIRE_RATES_MAX;
		val /= 1024 - min_af_val;
		autofire_delay = autofire_delay_rates[min( val, AUTOFIRE_RATES_MAX - 1) ]/2;
	}


}


//--------------------------------------------------------------------------------------
// implement autofire
void updateAutoFire()
{
	if (autofire_delay < 50) autofire_delay= 50;		// minimum delay
	if (autofire_phase)
	{
		pixel.setPixelColor(1,blend(autofire_color , np_black, 192) );
		Gamepad.release (current_button_output_table[autofire_button] ) ;
		autofire.SetInterval(autofire_delay -AUTOFIRE_PULSE_DURATION);
		Gamepad.write();

	}
	else
	{
		pixel.setPixelColor(1,autofire_color);
		Gamepad.press (current_button_output_table[autofire_button] ) ;
		autofire.SetInterval(AUTOFIRE_PULSE_DURATION);
		Gamepad.write();

	}
	autofire_phase = !autofire_phase;
	pixel.show();
}



//--------------------------------------------------------------------------------------
// check to see which button, is assigned to autofire, and handle the changes.
void checkAutofireAssignment ()
{
	int pos = analogRead (PIN_AUTOFIRE_ASSIGNMENT_SWITCH);
	int old_autofire = autofire_button;

	// the autofire button assignment switch is a 4PST slide switch with different pull down resistors 
	// for each slider position.  We then convert the analog input to a button here:
	if (pos < analog_slider_thresholds[0])
	{ autofire_button = BUTTON_PURPLE; autofire_color = np_purple;}
	else
		if (pos < analog_slider_thresholds[1])
		{ autofire_button = BUTTON_BLUE; autofire_color = np_blue; } 
		else
			if (pos <  analog_slider_thresholds[2])
			{ autofire_button = BUTTON_RED; autofire_color = np_red;}
			else
			{ autofire_button = BUTTON_GREEN; autofire_color = np_green;}
			if (autofire_button!=old_autofire) // we changed buttons, clear the gamepad object so we don't have stuck buttons
			{
				Gamepad.releaseAll();
				Gamepad.write();
			}
}


//-----------------------------------------------------------------------------------------------------------------
// flash between two pixel colors a given number of times and delays
// this is BLOCKING -- only use when starting
void flashColors(int count, const my_color& color1, const my_color& color2, int rate=300,int rate2=300)
{
	byte a;
	for (a = 0; a < count; a++)
	{
		pixel.setPixelColor (1,color1.r, color1.g, color1.b);
		pixel.show ();
		delay (rate);
		pixel.setPixelColor (1,color2.r,color2.g,color2.b);
		pixel.show ();
		delay (rate2);

	}
	pixel.setPixelColor (1,0);
	pixel.show ();

}

///-----------------------------------------------------------------------------------------------------------------------------------------
void resetDefaults () ;


void setup()
{
	int a;
	for (a = 2; a <= 13; a++)
		pinMode (a, INPUT_PULLUP);

	pinMode (PIN_AUTOFIRE_RATE, INPUT);
	pinMode (PIN_AUTOFIRE_ASSIGNMENT_SWITCH, INPUT);
	pinMode (PIN_STATUS_PIXEL, OUTPUT);
	updateAutofireRate();
	autofire.Enable (false);
	Gamepad.begin();
	Gamepad.releaseAll();
	Gamepad.write();
	buttons.begin();
	buttons.read();
#if DEBUG
	delay (2500);
	Serial.begin (115200);
	Serial.println ("HELLO");
#endif 

	// do a little status reporting on startup, in the form of flashing color codes
	pixel.setBrightness (pixel_brightness);
	flashColors (1+fourway,np_black, np_green);
	delay (400);
	flashColors (1+analog_stick_mode,np_black, np_red);
	delay (400);
	flashColors (dpad_mode+1,np_black, np_blue);
	delay (400);
	flashColors (joy_buttons_mode+1,np_black, np_purple);
	delay (400);
	flashColors (alt_table+1, np_black,np_white);
	delay (400);
	flashColors (1+flipped_lr, np_black, np_yellow);
	delay (400);
	if (flipped_lr) 
		swap (current_button_output_table[BUTTON_WHITE] , current_button_output_table[BUTTON_YELLOW]);
	current_button_output_table = button_outputs[alt_table];
	pixel.setPixelColor (1,0);
	pixel.show ();
	buttons.buttonDown  (buttonDown);
	buttons.buttonUp  (buttonUp);
}


void resetDefaults () 
{ 
	Gamepad.releaseAll();
	Gamepad.write();
	fourway= 0;
	joy_buttons_mode=1;
	pixel_brightness=255;
	pixel.setBrightness (pixel_brightness);
	pixel.show();
	dpad_mode=1;
	analog_stick_mode=1;
	joy_buttons_mode=1;
	flipped_lr=0;
	alt_table=0;
	flashColors (4,np_cyan,np_orange);
} 


// Callback from the button class to process when a button is pressed while in programming mode
void progNModeButtonDown (byte button_id )
{
	static byte spin_counter=0;
	// check for stick-spin-reset-to-defaults
	switch (button_id)
	{
	case BUTTON_LEFT: 
		spin_counter |=1;
		if (spin_counter == 15) resetDefaults();
		else return;
	case BUTTON_RIGHT: 
		spin_counter |=2;
		if (spin_counter == 15) resetDefaults();
		else return;
	case BUTTON_UP: 
		spin_counter |=4;
		if (spin_counter == 15) resetDefaults();
		else return;
	case BUTTON_DOWN: 
		spin_counter |=8;
		if (spin_counter == 15) resetDefaults();
		else return;
	} 
	spin_counter=0;
	switch (button_id)
	{
	case BUTTON_PURPLE: 
		joy_buttons_mode = (joy_buttons_mode?1:0);
		flashColors (joy_buttons_mode+1,np_black, np_purple);
		return;
	case BUTTON_WHITE:	//switch shoulder trigger buttons
		alt_table ++;
		if (alt_table >=MAX_ALT_TABLES) alt_table =0 ;
		current_button_output_table = button_outputs[alt_table];
		flashColors(alt_table+1, np_black, np_white);
		return;
	case BUTTON_RED: 
		analog_stick_mode = (analog_stick_mode?1:0);
		flashColors (1+analog_stick_mode,np_black, np_red);
		return;
	case BUTTON_GREEN: 
		fourway = (fourway?0:1);
		flashColors(1+fourway, np_black, np_green);
		return;
	case BUTTON_YELLOW:				// switch left and right triggers 
		swap (current_button_output_table[BUTTON_WHITE] , current_button_output_table[BUTTON_YELLOW]);
		flipped_lr = (flipped_lr?0:1);
		flashColors(1+flipped_lr, np_black, np_yellow);
		return;
	case BUTTON_BLUE: 
		dpad_mode=(dpad_mode?0:1);
		flashColors (dpad_mode+1,np_black, np_blue);
		return;
	case BUTTON_SELECT: return;
	case BUTTON_START: return;

	}
} ;



void doProgMode () 
{ 
	buttons.buttonDown  (progNModeButtonDown);
	buttons.read();
	int t_pixel_brightness=pixel_brightness;
	while ( ! ((buttons.getButton(BUTTON_SELECT)) &&  (buttons.getButton(BUTTON_START)))) 
	{ 
		buttons.read();
		pixel.setPixelColor (1,random(255), random(255), random(255)); 
		delay (20);
		pixel.show();
		t_pixel_brightness = 256-constrain ((analogRead  (PIN_AUTOFIRE_RATE)/4) +1,1,255);
		pixel.setBrightness (t_pixel_brightness);
		pixel.show();

	} 
	pixel_brightness = t_pixel_brightness;
} 


void loop()
{
	autofire.tick();
	check_autofire_controls.tick();
	buttons.read();

	// check to see if we're in user adjustments mode 
	if ((buttons.getButton(BUTTON_SELECT)) &&  (buttons.getButton(BUTTON_START)))
	{ 
		programming_mode_counter++;
		delay (1) ;
		if (programming_mode_counter > 2500)
		{ 
			programming_mode_counter=0;
			// wait to release start & select 
			buttons.buttonDown  (NULL);
			buttons.buttonUp  (NULL);
			buttons.read();
			pixel.setPixelColor (1,0);
			pixel.show();
			while ( ((buttons.getButton(BUTTON_SELECT)) &&  (buttons.getButton(BUTTON_START))))
			{ 
				buttons.read();
			} ; 
			doProgMode();
			pixel.setPixelColor (1,0);
			pixel.show();
			buttons.buttonDown  (buttonDown);
			buttons.buttonUp  (buttonUp);
			buttons.clearAll();
			Gamepad.releaseAll();
			Gamepad.write();
		} 
	} 
	else
		programming_mode_counter=0;
}
