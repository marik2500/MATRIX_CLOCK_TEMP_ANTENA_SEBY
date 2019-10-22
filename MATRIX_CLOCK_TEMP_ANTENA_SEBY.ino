
#include <math.h>
const int ThermPin = A1;  // The thermistor is on Analogue Pin 1 
double ThermValue = 0; // Somewhere to stick the raw Analogue Pin Value.

#include <nRF24L01.h>
#include <RF24.h>
float temp1, temp2, temp3;
RF24 radiohf(9, 10);
const uint64_t pipes[3] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL };

#include <EEPROM.h>
#include <Wire.h>
#include <LedControl.h>

//YOU CAN CHANGE THIS VALUE TO CHANGE SHOWN TEMPERATURE - IT'S ONLY TEMPERATURE OFFSET
//Example: Real temperature is 23°C. Clock shows 26°C. Difference is -3°C. You have to change value to 100 - 3 = 97
const byte temperatureOffset = 97;	//99 = -1, 100 = 0, 101 = 1 (only integer values)


//OTHERS
const byte versionMajor = 1;
const byte versionMinor = 6;

//MATRIX DISPLAY
byte devices = 6;	//count of displays
LedControl lc = LedControl(6, 8, 7, devices);	//DIN, CLK, CS, count of displays

//RTC DS3231
//How to read time from RTC without library, see https://www.mylms.cz/text-kusy-kodu-k-arduinu/#ds3231
#define DS3231_I2C_ADDRESS 0x68 //address of DS3231 module
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year; //global variables for time
byte currentTemperatureIN = 0;	//temperature in degC
byte currentTemperatureOUT = 0;	//255 = 0.75, 128 = 0.5, 64 = 0,25, other = 0.0

//TIMMING
unsigned long presentTime;
unsigned long displayTime;	//drawing
unsigned long temperatureTime;	//gat temp

//IO
#define BTN1 2
#define BTN2 3
//#define INPUT1 10
//#define INPUT2 11
//How to detect signal rising/falling edge, see https://www.mylms.cz/kusy-kodu-k-arduinu/#edge_detection2
bool presentButton1, presentButton2, presentInput1, presentInput2; //actual state of buttons and inputs
bool lastButton1, lastButton2; //last state button  [RO]starea ultimului buton  
bool edgeButton1, edgeButton2; //edge state buttons [RO]starea butoanelor de margine

//SYSTEM STATE
byte systemState;	//0 = show time/date/temp, other = menu
byte showMode = 0;	//0 = time, 1 = date, 2 = temperature
bool showDots;	//dots are shown
bool pmDotEnable = false;	//pm dot is shown
bool showText = false;	//some user text is shown

//chars
const uint64_t symbols[] = {
	0x0000000000000000,	//0 - space
	0x0008183f7f3f1808,	//1 - arrowL
	0x0010307efe7e3010,	//2 - arrowR
	0x0000000070d8d870,	//3 - degree
	0x0018180000000000,	//4 - .
	0x0018001818181818,	//5 - !
	0x0018180000181800,	//6 - :
	0x000103060c183060,	//7 - /
	0x006666667e66663c,	//8 - A
	0x003e66663e66663e,	//9 - B
	0x003c66060606663c,	//10 - C
	0x003e66666666663e,	//11 - D
	0x007e06063e06067e,	//12 - E
	0x000606063e06067e,	//13 - F
	0x003c66760606663c,	//14 - G
	0x006666667e666666,	//15 - H
	0x003c18181818183c,	//16 - I
	0x001c363630303078,	//17 - J
	0x0066361e0e1e3666,	//18 - K
	0x007e060606060606,	//19 - L
	0x006363636b7f7763,	//20 - M
	0x006363737b6f6763,	//21 - N
	0x003c66666666663c,	//22 - O
	0x0006063e6666663e,	//23 - P
	0x00603c766666663c,	//24 - Q
	0x0066361e3e66663e,	//25 - R
	0x003c66603c06663c,	//26 - S
	0x0018181818185a7e,	//27 - T
	0x003c666666666666,	//28 - U
	0x00183c6666666666,	//29 - V
	0x0063777f6b636363,	//30 - W
	0x00c6c66c386cc6c6,	//31 - X
	0x001818183c666666,	//32 - Y
	0x007e060c1830607e,	//33 - Z
	0x007c667c603c0000,	//34 - a
	0x003e66663e060606,	//35 - b
	0x003c6606663c0000,	//36 - c
	0x007c66667c606060,	//37 - d
	0x003c067e663c0000,	//38 - e
	0x000c0c3e0c0c6c38,	//39 - f
	0x003c607c66667c00,	//40 - g
	0x006666663e060606,	//41 - h
	0x003c181818001800,	//42 - i
	0x001c363630300030,	//43 - j
	0x0066361e36660606,	//44 - k
	0x0018181818181818,	//45 - l
	0x006b6b7f77630000,	//46 - m
	0x006666667e3e0000,	//47 - n
	0x003c6666663c0000,	//48 - o
	0x0006063e66663e00,	//49 - p
	0x00f0b03c36363c00,	//50 - q
	0x00060666663e0000,	//51 - r
	0x003e403c027c0000,	//52 - s
	0x00182c0c0c3e0c08,	//53 - t
	0x007c666666660000,	//54 - u
	0x00183c6666000000,	//55 - v
	0x003e6b6b6b630000,	//56 - w
	0x00663c183c660000,	//57 - x
	0x003c607c66660000,	//58 - y
	0x003c0c18303c0000,	//59 - z
	0x003e676f7b73633e,	//60 - 0 (font 1)
	0x007e181818181c18,
	0x007e660c3860663c,
	0x003c66603860663c,
	0x0078307f33363c38,
	0x003c6660603e067e,
	0x003c66663e060c38,
	0x001818183060667e,
	0x003c66663c66663c,
	0x001c30607c66663c,
	0x003c66666e76663c,	//70 - 0 (font 2)
	0x007e1818181c1818,
	0x007e060c3060663c,
	0x003c66603860663c,
	0x0030307e32343830,
	0x003c6660603e067e,
	0x003c66663e06663c,
	0x001818183030667e,
	0x003c66663c66663c,
	0x003c66607c66663c,
	0x1c2222222222221c,	//80 - 0 (font 3)
	0x1c08080808080c08,
	0x3e0408102020221c,
	0x1c2220201820221c,
	0x20203e2224283020,
	0x1c2220201e02023e,
	0x1c2222221e02221c,
	0x040404081020203e,
	0x1c2222221c22221c,
	0x1c22203c2222221c,
	0x001c22262a32221c,	//90 - 0 (font 4)
	0x003e080808080c08,
	0x003e04081020221c,
	0x001c22201008103e,
	0x0010103e12141810,
	0x001c2220201e023e,
	0x001c22221e020418,
	0x000404040810203e,
	0x001c22221c22221c,
	0x000c10203c22221c,
	0x003c24242424243c,	//100 - 0 (font 5)
	0x0020202020202020,
	0x003c04043c20203c,
	0x003c20203c20203c,
	0x002020203c242424,
	0x003c20203c04043c,
	0x003c24243c04043c,
	0x002020202020203c,
	0x003c24243c24243c,
	0x003c20203c24243c,
  0x0000000097d2b297, //110 - IN
  0x0000005721275107, //111 - Ex
  0x0000003e3e000000  //112 - minus
};

//some chars for my use...you can add
#define char_space 0
#define char_arrowL 1
#define char_arrowR 2
#define char_degree 3
#define char_onedot 4
#define char_exc 5
#define char_dot 6
#define char_slash 7

#define char_B 9
#define char_C 10
#define char_D 11
#define char_E 12
#define char_H 15
#define char_L 19
#define char_M 20
#define char_R 25
#define char_S 26
#define char_U 28
#define char_Y 32
#define char_a 34

#define char_b 35
#define char_d 37
#define char_e 38
#define char_f 39
#define char_m 46
#define char_o 48
#define char_r 51
#define char_t 53
#define char_v 55
#define char_y 58
#define char_IN 110
#define char_Ex 111
#define char_minus 112

byte fontCount = 5;	//how many fonts is used
byte fontOffset = 60;	//count of symbols before 1st number

//default values...but they are load from EEPROM
byte bright = 7;	//brightness
byte font = 1;	//do not set less than 1. symbols 0-19 are used for letters etc.
byte dotStyle = 2;	//0 - off, 1 - on, 2 - blinking
byte timeMode1224 = 1;	//12/24 hour mode 12 = 0, 24 = 1
byte rotateFont1 = 0;	//font rotateing vertically (one char)
byte rotateFont2 = 0;	//font rotateing vertically (all display)
byte showDate = 0;	//how many second in one minute cycle is date shown
byte showTemperature = 0;	//how many second in one minute cycle is temperature shown
byte upsideDown = 0;	//us font Upside down
byte verticalMode = 0;	//clock is vertical


void setup() 
{
radiohf.begin();
radiohf.setPALevel ( RF24_PA_MAX ) ;
radiohf.setDataRate(RF24_250KBPS);
radiohf.openReadingPipe(1, pipes[1]);
radiohf.openReadingPipe(2, pipes[2]);
radiohf.openReadingPipe(3, pipes[3]);
radiohf.startListening();

	//COMMUNICATION
	Wire.begin(); //start I2C communication
	Serial.begin(9600);	//for communication with PC

	//IO
	//there is internal pull-up used - read variables are negated
	pinMode(BTN1, INPUT_PULLUP);
	pinMode(BTN2, INPUT_PULLUP);
	//pinMode(INPUT1, INPUT_PULLUP);
	//pinMode(INPUT2, INPUT_PULLUP);

	//LOAD DATA FROM EEPROM
	bright = EEPROM.read(0);	//load light intensity from EEPROM
	if (bright < 0 || bright > 15) 
	{
		//in case variable out of range
		bright = 7;
	}

	font = EEPROM.read(1);	//load font style from EEPROM
	if (font < 1 || font > fontCount) 
	{
		//in case variable out of range
		font = 1;
	}

	dotStyle = EEPROM.read(2);	//load dot style from EEPROM
	if (dotStyle < 0 || dotStyle > 2) 
	{
		//in case variable out of range
		dotStyle = 2;
	}

	showTemperature = EEPROM.read(3);	//load temperature time EEPROM
	if (showTemperature < 0 || showTemperature > 60) 
	{
		//in case variable out of range
		showTemperature = 0;
	}

	rotateFont1 = EEPROM.read(4);	//load rotate font 1 from EEPROM
	if (rotateFont1 < 0 || rotateFont1 > 1) 
	{
		//in case variable out of range
		rotateFont1 = 0;
	}

	rotateFont2 = EEPROM.read(5);	//load rotate font 2 from EEPROM
	if (rotateFont2 < 0 || rotateFont2 > 1) 
	{
		//in case variable out of range
		rotateFont2 = 0;
	}

	timeMode1224 = EEPROM.read(6);	//load 12/24 hour mode from EEPROM
	if (timeMode1224 < 0 || timeMode1224 > 1) 
	{
		//in case variable out of range
		timeMode1224 = 1;
	}

	showDate = EEPROM.read(7);	//load date time
	if (showDate < 0 || showDate > 60) 
	{
		//in case variable out of range
		showDate = 0;
	}

	upsideDown = EEPROM.read(8);	//load upsideDown state from EEPROM
	if (upsideDown < 0 || upsideDown > 1) 
	{
    //in case variable out of range
		upsideDown = 0;
	}

	verticalMode = EEPROM.read(9);	//load vertical mode from EEPROM
	if (verticalMode < 0 || verticalMode > 1) 
	{
		//in case variable out of range
		verticalMode = 0;
	}

	delay(10);	//just small delay...I thing I have had add it for correct function of display

	//SET ALL DISPLAYS
	for (byte address = 0; address<devices; address++) 
	{
		lc.shutdown(address, false);	//powersaving
		lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
		lc.clearDisplay(address);		//clear display
	}

	/*
	//INIT TIME SETTING
	//You can use this for first setup
	//!!! Do not forget deactivate it !!!
	SetRtc(15, 41, 8, 6, 30, 3, 18);	//sec, min, hour, dayOfWeek, dayOfMonth, month, year
	*/

	Intro();	//show LMS! and version
	Serial.println("Send '?' for help");

	GetTemperature();	//just for start
}

void loop() 
{
	//store input to variarbe (and nagete because pullups are used)
	presentButton1 = !digitalRead(BTN1);
	presentButton2 = !digitalRead(BTN2);
	//presentInput1 = !digitalRead(INPUT1);
	//presentInput2 = !digitalRead(INPUT2);

	//edge detection
	edgeButton1 = (presentButton1 ^ lastButton1) & !presentButton1;
	edgeButton2 = (presentButton2 ^ lastButton2) & !presentButton2;

	switch (systemState) 
	{
	case 0:
		//SHOW ACTUAL TIME
		presentTime = millis();

		//30 second timer
		if (presentTime - temperatureTime >= 30000) 
		{
			temperatureTime = presentTime;
			GetTemperature();
		}

		//0.5 second trigger
		if (presentTime - displayTime >= 500) 
		{
			displayTime = presentTime;
			GetRtc();		//get actual time

			ToggleMode();	//toggle mode time/date/temperature

			//show message or time
			if (presentInput1 && !presentInput2) 
			{
				//IN1 = 1, IN2 = 0
				//MESSAGE 1 - blank display
				showText = true;	//turn off colon, comma etc
				DrawSymbol(3, char_space);	//space
				DrawSymbol(2, char_space);	//space
				DrawSymbol(1, char_space);	//space
				DrawSymbol(0, char_space);	//space
			}
			else if (!presentInput1 && presentInput2) 
			{
				//IN1 = 0, IN2 = 1
				//MESSAGE 1 - LMS!
				showText = true;	//turn off colon, comma etc
				DrawSymbol(3, char_L);	//L
				DrawSymbol(2, char_M);	//M
				DrawSymbol(1, char_S);	//S
				DrawSymbol(0, char_exc);	//!
			}
			else if (presentInput1 && presentInput2) 
			{
				//IN1 = 1, IN2 = 1
				//MESSAGE 2 - version
				showText = true;	//turn off colon, comma etc
				DrawSymbol(3, char_v);	//v
				DrawSymbol(2, versionMajor + fontOffset);
				DrawSymbol(1, char_dot);	//.
				DrawSymbol(0, versionMinor + fontOffset);
			}
			else {
				//IN1 = 0, IN2 = 0
				//show time (or date, etc.)
				showText = false;	//turn on colon, comma etc
				WriteTime();	//write actual time (etc) to matrix display
			}
			
		}

		if (presentButton1 && presentButton2) 
		{
			systemState = 1;	//go to "premenu"
		}
		break;

	case 1:
		if (!presentButton1 && !presentButton2) {
			//NEXT
			GetRtc();		//get actual time (read time in 24h format - in menu is always 24h time format used)
			systemState++; //go to menu
			DrawSymbol(5, char_space);
			DrawSymbol(4, char_space);
			DrawSymbol(3, char_H);	//space
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, char_space);	//space
			DrawSymbol(0, char_space);	//H
		}
		break;

	case 2:
		//menu 1
		//set HOURS
		WriteTime();

		if (edgeButton1) 
		{
			//rising edge detected
			//NEXT
			systemState++;
			DrawSymbol(3, char_M);	//M
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, char_space);	//space
			DrawSymbol(0, char_space);	//space
		}

		if (edgeButton2) 
		{
			//rising edge detected
			//add hour
			hour++;
			if (hour > 23) {
				hour = 0;
			}
		}
		break;

	case 3:
		//menu 3
		//set MINUTES
		WriteTime();

		if (edgeButton1) {
			//rising edge detected
			//NEXT
			systemState++;
			DrawSymbol(3, char_y);	//y
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (year / 10) + fontOffset);	//actual year
			DrawSymbol(0, (year % 10) + fontOffset);	//actual year
		}

		if (edgeButton2) {
			//rising edge detected
			//add minutes
			minute++;
			if (minute > 59) {
				minute = 0;
			}
		}
		break;

	case 4:
		//menu 4
		//set YEAR
		if (edgeButton1) {
			//rising edge detected
			//NEXT
			systemState++;
			DrawSymbol(3, char_m);	//m month
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (month / 10) +  fontOffset);	//actual month
			DrawSymbol(0, (month % 10) +  fontOffset);	//actual month
		}

		if (edgeButton2) {
			//rising edge detected
			//add year
			year++;
			if (year > 50) {
				year = 19;
			}

			DrawSymbol(3, char_y);	//year
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (year / 10) + fontOffset);	//actual year
			DrawSymbol(0, (year % 10) + fontOffset);	//actual year

			delay(25);
		}
		break;

	case 5:
		//menu 5
		//set MONTH
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_d);	//d
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (dayOfMonth / 10) + fontOffset);	//actual day of month
			DrawSymbol(0, (dayOfMonth % 10) + fontOffset);	//actual day of month
		}

		if (edgeButton2) {
			//rising edge detected
			//add month
			month++;
			if (month > 12) {
				month = 1;
			}

			DrawSymbol(3, char_M);	//M
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (month / 10) + fontOffset);	//actual month
			DrawSymbol(0, (month % 10) + fontOffset);	//actual month

			delay(25);
		}
		break;

	case 6:
		//menu 6
		//set DAY
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_slash);	//12/24
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (timeMode1224 / 10) + fontOffset);	//actual time mode
			DrawSymbol(0, (timeMode1224 % 10) + fontOffset);	//actual time mode
		}

		if (edgeButton2) {
			//rising edge detected
			//add day of month
			dayOfMonth++;

			if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
				if (dayOfMonth > 31) {
					dayOfMonth = 1;
				}
			}

			if (month == 4 || month == 6|| month == 9 || month == 11) {
				if (dayOfMonth > 30) {
					dayOfMonth = 1;
				}
			}

			if (month == 2) {
				if (CheckLeapYear(year)) {
					//leap year
					if (dayOfMonth > 29) {
						dayOfMonth = 1;
					}
				}
				else {
					if (dayOfMonth > 28) {
						dayOfMonth = 1;
					}
				}
			}

			DrawSymbol(3, char_d);	//d
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (dayOfMonth / 10) + fontOffset);	//actual day of month
			DrawSymbol(0, (dayOfMonth % 10) + fontOffset);	//actual day of month

			delay(25);
		}
		break;

	case 7:
		//menu 7
		//set 12/24 MODE
		if (edgeButton1) 
		{
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_D);	//date
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (showDate / 10) + fontOffset);	//show date
			DrawSymbol(0, (showDate % 10) + fontOffset);	//show date
		}

		if (edgeButton2) 
		{
			//rising edge detected
			//set 12/24 mode
			timeMode1224++;
			if (timeMode1224 > 1) 
			{
				timeMode1224 = 0;
			}

			DrawSymbol(3, char_slash);	//12/24
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (timeMode1224 / 10) + fontOffset);	//actual time mode
			DrawSymbol(0, (timeMode1224 % 10) + fontOffset);	//actual time mode

			delay(25);
		}
		break;

	case 8:
		//menu 8
		//show show DATE
		if (edgeButton1) 
		{
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_t);	//t - temperature
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (showTemperature / 10) + fontOffset);	//actual show temp
			DrawSymbol(0, (showTemperature % 10) + fontOffset);	//actual show temp
		}

		if (edgeButton2) {
			//rising edge detected
			//set show date
			showDate++;
			if (showDate > 60) {
				showDate = 0;
			}

			DrawSymbol(3, char_D);	//date
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (showDate / 10) + fontOffset);	//actual show date
			DrawSymbol(0, (showDate % 10) + fontOffset);	//actual show date

			delay(25);
		}
		break;

	case 9:
		//menu 9
		//show TEMPERATURE
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_f);	//F
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (font / 10) + (font * 10) + fontOffset - 10);	//actual font
			DrawSymbol(0, (font % 10) + (font * 10) + fontOffset - 10);	//actual font
		}

		if (edgeButton2) {
			//rising edge detected
			//set show temperature
			showTemperature++;
			if (showTemperature > 60) {
				showTemperature = 0;
			}

			DrawSymbol(3, char_t);	//t - temperature
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (showTemperature / 10) + fontOffset);	//actual show temp
			DrawSymbol(0, (showTemperature % 10) + fontOffset);	//actual show temp

			delay(25);
		}
		break;

	case 10:
		//menu 10
		//set FONT
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_dot);	//:
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (dotStyle / 10) + fontOffset);	//actual dot style
			DrawSymbol(0, (dotStyle % 10) + fontOffset);	//actual dot style
		}

		if (edgeButton2) {
			//rising edge detected
			//set font
			font++;
			if (font > fontCount) {
				font = 1;
			}

			DrawSymbol(3, char_f);	//Font
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (font / 10) + (font * 10) + fontOffset - 10);	//actual font
			DrawSymbol(0, (font % 10) + (font * 10) + fontOffset - 10);	//actual font

			delay(25);
		}
		break;

	case 11:
		//menu 11
		//set DOT STYLE
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_b);	//Bright
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (bright / 10) + fontOffset);	//actual light intensity
			DrawSymbol(0, (bright % 10) + fontOffset);	//actual light intensity
		}

		if (edgeButton2) {
			//rising edge detected
			//set dot style
			dotStyle++;
			if (dotStyle > 2) {
				dotStyle = 0;
			}

			DrawSymbol(3, char_dot);	//:dotStyle
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (dotStyle / 10) + fontOffset);	//actual dot style
			DrawSymbol(0, (dotStyle % 10) + fontOffset);	//actual dot style

			delay(25);
		}
		break;

	case 12:
		//menu 12
		//set BRIGHTNES
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_R);	//R
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (rotateFont1 / 10) + fontOffset);	//actual rotate font 1
			DrawSymbol(0, (rotateFont1 % 10) + fontOffset);	//actual rotate font 1
		}

		if (edgeButton2) {
			//rising edge detected
			//add brightness
			bright++;
			if (bright > 15) {
				bright = 0;			
			}
				
			DrawSymbol(3, char_b);	//B
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (bright / 10) + fontOffset);	//actual light intensity
			DrawSymbol(0, (bright % 10) + fontOffset);	//actual light intensity

			for (byte address = 0; address<devices; address++) {
				lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
			}

			delay(25);
		}
		break;

	case 13:
		//menu 13
		//set rotate FONT 1
		//Rotate each symbol separately (vertical)
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_r);	//rotate
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (rotateFont2 / 10) + fontOffset);	//actual rotate font 2
			DrawSymbol(0, (rotateFont2 % 10) + fontOffset);	//actual rotate font 2
		}

		if (edgeButton2) {
			//rising edge detected
			//set font rotateing
			rotateFont1++;
			if (rotateFont1 > 1) {
				rotateFont1 = 0;
			}

			DrawSymbol(3, char_R);	//R
			DrawSymbol(2, 0);	//space
			DrawSymbol(1, (rotateFont1 / 10) + fontOffset);	//actual rotate font 1
			DrawSymbol(0, (rotateFont1 % 10) + fontOffset);	//actual rotate font 1

			delay(25);
		}
		break;

	case 14:
		//menu 14
		//set rotate FONT 2
		//Rotate all display (vertically)
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_U);	//U upsideDown
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (upsideDown / 10) + fontOffset);	//actual rotate font 2
			DrawSymbol(0, (upsideDown % 10) + fontOffset);	//actual rotate font 2
		}

		if (edgeButton2) {
			//rising edge detected
			//set font rotateing
			rotateFont2++;
			if (rotateFont2 > 1) {
				rotateFont2 = 0;
			}

			DrawSymbol(3, char_r);	//r rotateFont2
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (rotateFont2 / 10) + fontOffset);	//actual rotate font 2
			DrawSymbol(0, (rotateFont2 % 10) + fontOffset);	//actual rotate font 2

			delay(25);
		}
		break;

	case 15:
		//menu 15
		//set rotate upsidedown
		//Rotate all display (horizontaly)
		if (edgeButton1) {
			//rising edge detected

			//NEXT
			systemState++;
			DrawSymbol(3, char_v);	//v verticalMode
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (verticalMode / 10) + fontOffset);	//actual vertical mode
			DrawSymbol(0, (verticalMode % 10) + fontOffset);	//actual vertical mode
		}

		if (edgeButton2) {
			//rising edge detected
			//set font rotateing
			upsideDown++;
			if (upsideDown > 1) {
				upsideDown = 0;
			}

			DrawSymbol(3, char_U);	//U upsideDown
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (upsideDown / 10) + fontOffset);	//actual rotate font 2
			DrawSymbol(0, (upsideDown % 10) + fontOffset);	//actual rotate font 2

			delay(25);
		}
		break;

	case 16:
		//menu 15
		//set vertical mode
		//Rotate all display (horizontaly)
		if (edgeButton1) 
		{
			//rising edge detected

			//NEXT
			systemState++;
      
      DrawSymbol(4, char_S);  //S
			DrawSymbol(3, char_t);	//t
			DrawSymbol(2, char_o);	//o
			DrawSymbol(1, char_r);	//r
			DrawSymbol(0, char_e);	//e
		}

		if (edgeButton2) 
		{
			//rising edge detected
			//set font rotateing
			verticalMode++;
			if (verticalMode > 1) 
			{
				verticalMode = 0;
		  }

			DrawSymbol(3, char_v);	//v
			DrawSymbol(2, char_arrowL);	//space
			DrawSymbol(1, (verticalMode / 10) + fontOffset);	//actual vertical mode
			DrawSymbol(0, (verticalMode % 10) + fontOffset);	//actual vertical mode

			delay(25);
		}
		break;

	case 17:
		//menu 17
		//EXIT
		if (edgeButton1) {
			//rising edge detected
			SetRtc(0, minute, hour, dayOfWeek, dayOfMonth, month, year);	//set time and zero second

			EEPROM.write(0, bright);	//store actual light intensity to addr 0
			EEPROM.write(1, font);	//store actual font to addr 1
			EEPROM.write(2, dotStyle);	//store actual font to addr 2
			EEPROM.write(3, showTemperature);	//store temperature time to addr 3
			EEPROM.write(4, rotateFont1);	//store rotate font1 to addr 4
			EEPROM.write(5, rotateFont2);	//store rotate font2 to addr 5
			EEPROM.write(6, timeMode1224);	//store 12/24 mode to addr 6
			EEPROM.write(7, showDate);	//store date time to eeprom
			EEPROM.write(8, upsideDown);	//store upsideDown rotate to EEPROM
			EEPROM.write(9, verticalMode);	//store vertical mode to EEPROM

			systemState = 0;	//show actual time
		}
		break;
	}

	lastButton1 = presentButton1; //save current state to last state
	lastButton2 = presentButton2; //save current state to last state

	SerialComm();	//read data from PC
}

void WriteTime() {
	byte storedFont = font;	//store actual setting during MENU

	if (systemState > 0) 
	{
		//reserve font for menu
		font = 1;
	}

	//write hour to matrix display in menu
	if (systemState == 2) 
	{
		//show hours in 24h format in menu (set hours)
		DrawSymbol(0, (hour % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(1, (hour / 10) + (font * 10) + fontOffset - 10);

		showDots = false;	//hide dots (colon)
		pmDotEnable = false;	//hide PM dot
	}

	//write minute to matrix display in menu
	if (systemState == 3) 
	{
		//show minute in menu
		DrawSymbol(0, (minute % 10) + (font * 10) + fontOffset - 10);
		DrawSymbol(1, (minute / 10) + (font * 10) + fontOffset - 10);
	}

	if (systemState == 0) 
	{
		//show data in normal state
		switch (showMode) 
		{
		case 0:
			//time
			if (timeMode1224 == 0) 
			{
				//12h format
				if (hour >= 0 && hour <= 11) 
				{
					pmDotEnable = false;
				}

				if (hour == 12) 
				{
					pmDotEnable = true;
				}

				if (hour == 0) 
				{
					hour = 12;
				}

				if (hour > 12) 
				{
					hour -= 12;
					pmDotEnable = true;
				}
			}
			else 
			{
				//24h format
				pmDotEnable = false;
			}

			//show dots (colon)
			if (systemState == 0) 
			{
				//TIME
				switch (dotStyle) 
				{
				case 0:
					showDots = false;	//hide dots
					break;
				case 1:
					showDots = true;	//show dots
					break;
				case 2:
					showDots = !showDots;	//blinking
					break;
				}
			}

      	
			
			
			
			
//hour..............................................................
			DrawSymbol(3, (hour % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(4, (hour / 10) + (font * 10) + fontOffset - 10);
     

//dots..............................................................
if (second % 2 == 0)

      {
      DrawSymbol(2, (char_dot) );
      DrawSymbol(5, (char_arrowL) );
      }
else
      {
      DrawSymbol(2, (char_space) );
      DrawSymbol(5, (char_arrowR) );
      }
			
			
      
//minute...........................................................
			DrawSymbol(0, (minute % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(1, (minute / 10) + (font * 10) + fontOffset - 10);
     
			break;
		case 1:
//date..............................................................
//day (instead of hour)
			
			DrawSymbol(5, (char_space) );
			DrawSymbol(3, (dayOfMonth % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(4, (dayOfMonth / 10) + (font * 10) + fontOffset - 10);
      DrawSymbol(2, (char_slash) );
      
			//month (instead of minute)
			DrawSymbol(0, (month % 10) + (font * 10) + fontOffset - 10);
			DrawSymbol(1, (month / 10) + (font * 10) + fontOffset - 10);

			showDots = false;	//hide dots (colon)
			pmDotEnable = false;	//hide PM dot
			break;
		case 2:
			//temperatureIN
      
      DrawSymbol(5, (char_IN) );
      DrawSymbol(4, (char_space) );
			DrawSymbol(3, (currentTemperatureIN / 10) + (font * 10) + fontOffset - 10);
      DrawSymbol(2, (currentTemperatureIN % 10) + (font * 10) + fontOffset - 10);
/*
			switch (currentTemperatureOUT) 
			{
			case 255:
				DrawSymbol(1, 8 + (font * 10) + fontOffset - 10);
				break;
			case 128:
				DrawSymbol(1, 5 + (font * 10) + fontOffset - 10);
				break;
			case 64:
				DrawSymbol(1, 3 + (font * 10) + fontOffset - 10);
				break;
			default:
				DrawSymbol(1, 0 + (font * 10) + fontOffset - 10);
				break;
			}
    */ 
      DrawSymbol(1, char_degree);//...............
			DrawSymbol(0, char_C);	//draw "C" symbol
      
			showDots = false;	//hide dots (colon)
			pmDotEnable = false;	//hide PM dot
      break;
case 3:
      //temperatureOUT
      
      DrawSymbol(5, (char_Ex) );
      
      //DrawSymbol(4, (char_space) );
      if (currentTemperatureOUT < 0 )
      {
       DrawSymbol(4, (char_minus) ); 
      }
      else
      {
        DrawSymbol(4, (char_space) );
      }
      
      //DrawSymbol(3, (currentTemperatureOUT / 10) + (font * 10) + fontOffset - 10);
      if ((currentTemperatureOUT / 10)<1 )
      {
      DrawSymbol(3,(char_space) );
      }
      else
      {
      DrawSymbol(3, (currentTemperatureOUT / 10) + (font * 10) + fontOffset - 10);  
      }
      
      DrawSymbol(2, (currentTemperatureOUT % 10) + (font * 10) + fontOffset - 10);
      DrawSymbol(1, char_degree);//...............
      DrawSymbol(0, char_C);  //draw "C" symbol
			break;
		}

	}

	font = storedFont;
}

  void ToggleMode()  //Modul de comutare
{
	if (second == 0) 
	{
		//show time in 0 second
		showMode = 0;
	}
/*
	if (second == showTemperature && showTemperature > 0) 
	{
		//showtemp is now & showtemp is enabled 
		showMode = 2;
    //delay(1000);
    //showMode = 3;
	}
*/

if (second  >= 50 && second < 55 ) 
  {
   showMode = 2;//tempIN
  }

if (second  >= 55 && second  <= 59 ) 
  {
   showMode = 3;//tempOUT
  }


	if (second == showDate && showDate > 0) 
	{
		//showdate is now & showdate is enabled 
		//date has priority
		showMode = 1;
	}

	if (showTemperature == 60) 
	{
		//salways howtemp
		showMode = 2;
	}

	if (showDate == 60) 
	{
		//always show date
		//date has priority
		showMode = 1;
	}
}


void Intro() {
	DrawSymbol(4, char_S);	//S
	DrawSymbol(3, char_E);	//E
	DrawSymbol(2, char_B);	//B
	DrawSymbol(1, char_Y);	//Y
	delay(1500);

	//version of fw
	DrawSymbol(4, char_v);	                  //v
	DrawSymbol(3, versionMajor + fontOffset); //1
	DrawSymbol(2, char_onedot);	              //.
	DrawSymbol(1, versionMinor + fontOffset); //4
	delay(1500);
}

void DrawSymbol(byte adr, byte symbol) {
	//draw symbol
	//adr - used part of display
	if (rotateFont2 == 1) {
		adr = 3 - adr;
	}

	byte j = 0;	//variable for upside-down font turning

	for (byte i = 0; i < 8; i++) {
		j = i;
		if (upsideDown == 1) {
			//turn font upside down, if it's enabled
			j = 7 - i;
		}
		
		byte row = (symbols[symbol] >> i * 8) & 0xFF;	//just some magic - extract one row from all symbol

		if (verticalMode == 1) {
			//vertical mode
			lc.setColumn(adr, j, ByteRevers(row));

			//blinking dots on display
			//I have to draw "dots" during draw symbol. In other case it's blinking.
			//all "dots" are disabled in show text mode
			//Better variant would update symbol before draw - before FOR structure. Maybe in next version :)
			if (adr == 2 && dotStyle > 0 && i == 7 && !showText) {
				//colon
				lc.setLed(adr, 1, 7, showDots);  //addr, row, column
				lc.setLed(adr, 2, 7, showDots);
				lc.setLed(adr, 5, 7, showDots);
				lc.setLed(adr, 6, 7, showDots);
			}

			if (adr == 2 && systemState == 0 && (showMode == 1 || showMode == 2) && !showText) {
				//date and temperature point
				lc.setLed(adr, 1, 7, false);
				lc.setLed(adr, 2, 7, false);
			}
		}
		else {
			//horizontal mode
			lc.setRow(adr, j, ByteRevers(row));

			//blinking dots on display
			//I have to draw "dots" during draw symbol. In other case it's blinking.
			//all "dots" are disabled in show text mode
			//Better variant would update symbol before draw - before FOR structure. Maybe in next version :)
			if (adr == 2 && dotStyle > 0 && !showText) {
				//colon
				if (i == 1) lc.setLed(adr, 1, 7, showDots);  //addr, row, column
				if (i == 2) lc.setLed(adr, 2, 7, showDots);
				if (i == 5) lc.setLed(adr, 5, 7, showDots);
				if (i == 6) lc.setLed(adr, 6, 7, showDots);
       


       
			}

			if (adr == 2 && systemState == 0 && (showMode == 1 || showMode == 2) && !showText) {
				//date and temperature point
				if (i == 5) lc.setLed(adr, 5, 7, false);
				if (i == 6) lc.setLed(adr, 6, 7, false);
			}
		}
		
		if (adr == 0 && !showText) 
		{
			//PM point
			lc.setLed(0, 7, 7, pmDotEnable);
		}
	}
}

byte ByteRevers(byte in) {
	//font rotating
	if ((rotateFont1 == 1 && verticalMode == 0) || (rotateFont1 == 0 && verticalMode == 1)) {
		//do not rotate
		return(in);
	}

	byte out;
	out = 0;
	if (in & 0x01) out |= 0x80;
	if (in & 0x02) out |= 0x40;
	if (in & 0x04) out |= 0x20;
	if (in & 0x08) out |= 0x10;
	if (in & 0x10) out |= 0x08;
	if (in & 0x20) out |= 0x04;
	if (in & 0x40) out |= 0x02;
	if (in & 0x80) out |= 0x01;
	return(out);
}

//Set RTC
void SetRtc(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) {	
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); //set 0 to first register

	Wire.write(decToBcd(second)); //set second
	Wire.write(decToBcd(minute)); //set minutes 
	Wire.write(decToBcd(hour)); //set hours
	Wire.write(decToBcd(dayOfWeek)); //set day of week (1=su, 2=mo, 3=tu) 
	Wire.write(decToBcd(dayOfMonth)); //set day of month
	Wire.write(decToBcd(month)); //set month
	Wire.write(decToBcd(year)); //set year
	Wire.endTransmission();
}

//read RTC
void GetRtc() {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); //write "0"
	Wire.endTransmission();

	Wire.requestFrom(DS3231_I2C_ADDRESS, 7);	//request - 7 bytes from RTC
	second = bcdToDec(Wire.read() & 0x7f);
	minute = bcdToDec(Wire.read());
	hour = bcdToDec(Wire.read() & 0x3f);
	dayOfWeek = bcdToDec(Wire.read());
	dayOfMonth = bcdToDec(Wire.read());
	month = bcdToDec(Wire.read());
	year = bcdToDec(Wire.read());
}

void GetTemperature() 
{
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);  //request - 2 bytes from RTC
  
	ThermValue = analogRead(ThermPin);  
  double TempVal = 1/((1/298.00)+(1/3410.00)*log(1050.00/ThermValue-1.00));//change 1050 value to calibrate 
  TempVal = TempVal - 273.00;
   //if (Wire.available()) 
   //{
     currentTemperatureIN = round(TempVal);
   //}
	
	if ( radiohf.available() )
      {
       delay(50);
       radiohf.read(&temp2, sizeof(temp2));//view themperature from sensor 1
       currentTemperatureOUT = round(temp2);;
      }
	
}

//conversion Dec to BCD 
byte decToBcd(byte val) {
	return((val / 10 * 16) + (val % 10));
}

//conversion BCD to Dec 
byte bcdToDec(byte val) {
	return((val / 16 * 10) + (val % 16));
}

//serial communication with PC
void SerialComm() {
	//first char - type of data
	//second and third char - data
	//there are used only "printable" characters

	if (Serial.available() > 0) {
		byte receivedCommand;
		receivedCommand = Serial.read();	//read first char

		delay(10);	//wait for other char

		byte receivedDataTens;
		receivedDataTens = Serial.read();
		receivedDataTens -= 48;	// ASCII code for "0" is 48

		byte receivedDataOnes;
		receivedDataOnes = Serial.read();
		receivedDataOnes -= 48;	// ASCII code for "0" is 48

		byte receivedData;
		receivedData = (receivedDataTens * 10) + receivedDataOnes;
		if (receivedData > 99) {
			//maximal value is 99
			receivedData = 0;	//value is out of range
		}

		switch (receivedCommand) {
		case 47:
			//12/24 47 = /
			if (receivedData > 1) {
				receivedData = 1;
			}
			timeMode1224 = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(6, timeMode1224);	//save
			break;
		case 58:
			//dot style 58 = :
			if (receivedData > 2) {
				receivedData = 2;
			}
			dotStyle = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(2, dotStyle);	//save
			break;
		case 68:
			//show date 68 = D
			if (receivedData > 60) {
				receivedData = 0;
			}
			showDate = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(7, showDate);	//save
			break;
		case 72:
			//hour 72 = H
			if (receivedData > 23) {
				receivedData = 23;
			}
			hour = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 77:
			//minute 77 = M
			if (receivedData > 59) {
				receivedData = 59;
			}
			minute = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 83:
			//second 83 = S
			if (receivedData > 59) {
				receivedData = 59;
			}
			second = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 82:
			//Rotate font 1 82 = R
			if (receivedData > 1) {
				receivedData = 0;
			}
			rotateFont1 = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(4, rotateFont1);	//save
			break;
		case 85:
			//UpsideDown 1 85 = U
			if (receivedData > 1) {
				receivedData = 0;
			}
			upsideDown = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(8, upsideDown);	//save
			break;
		case 98:
			//brightness 98 = b
			if (receivedData > 15) {
				receivedData = 7;
			}
			bright = receivedData;

			for (byte address = 0; address<devices; address++) {
				lc.setIntensity(address, bright);	//set light intensity 0 - min, 15 - max
			}

			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(0, bright);	//save
			break;
		case 100:
			//dayOfMonth 100 = d
			if (receivedData > 31) {
				receivedData = 1;
			}
			dayOfMonth = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//save
			break;
		case 102:
			//font 102 = f
			if (receivedData > 5) {
				receivedData = 1;
			}

			if (receivedData == 0) {
				receivedData = 1;
			}
			font = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(1, font);	//save
			break;
		case 109:
			//month 109 = m
			if (receivedData > 12) {
				receivedData = 1;
			}
			month = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 114:
			//rotate 2 114 = r
			if (receivedData > 1) {
				receivedData = 0;
			}
			rotateFont2 = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(5, rotateFont2);	//save
			break;
		case 116:
			//temperature time 116 = t
			if (receivedData > 60) {
				receivedData = 0;
			}
			showTemperature = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(5, showTemperature);	//save
			break;
		case 118:
			//verticalMode 118 = v
			if (receivedData > 1) {
				receivedData = 0;
			}
			verticalMode = receivedData;
			lc.setLed(3, 7, 0, true);	//show setting dot
			EEPROM.write(9, verticalMode);	//save
			break;
		case 119:
			//dayofWeek 119 = w
			if (receivedData > 6) {
				receivedData = 1;
			}
			dayOfWeek = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 121:
			//year 121 = y
			year = receivedData;
			SetRtc(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
			lc.setLed(3, 7, 0, true);	//show setting dot
			break;
		case 63:
			//get data ? = 63
			Serial.println("");
			Serial.println("https://github.com/mylms/Arduino-Matrix-Clock");
			Serial.print("v");
			Serial.print(versionMajor);
			Serial.print(".");
			Serial.print(versionMinor);
			Serial.println("");
			Serial.println("");
			Serial.println("Date (y/m/d): ");
			Serial.println(year);
			Serial.println(month);
			Serial.println(dayOfMonth);
			Serial.println(dayOfWeek);

			Serial.println("");
			Serial.println("Time (H/M/S): ");
			Serial.println(hour);
			Serial.println(minute);
			Serial.println(second);

			Serial.println("");
			Serial.println("TemperatureIN (read only): ");
			Serial.print(currentTemperatureIN);

      Serial.println("");
      Serial.println("TemperatureOUT (read only): ");
      Serial.print(currentTemperatureOUT);

   /*  
			switch (currentTemperatureOUT) 
			{
			case 255:
				Serial.println(",75");
				break;
			case 128:
				Serial.println(",5");
				break;
			case 64:
				Serial.println(",25");
				break;
			default:
				Serial.println(",0");
				break;
			}
*/
			Serial.println("");
			Serial.println("Show date (D 00-60): ");
			Serial.println(showDate);

			Serial.println("");
			Serial.println("Show temperature (t 00-60): ");
			Serial.println(showTemperature);

			Serial.println("");
			Serial.println("Font rotate (R 00-01): ");
			Serial.println(rotateFont1);

			Serial.println("");
			Serial.println("Display rotate (r 00-01): ");
			Serial.println(rotateFont2);

			Serial.println("");
			Serial.println("UpsideDown rotate (U 00-01): ");
			Serial.println(upsideDown);

			Serial.println("");
			Serial.println("Brightness (b 00-15): ");
			Serial.println(bright);

			Serial.println("");
			Serial.println("Font (f 00-05): ");
			Serial.println(font);

			Serial.println("");
			Serial.println("12/24h mode (/ 00-01): ");
			Serial.println(timeMode1224);

			Serial.println("");
			Serial.println("Dot style (: 00-02): ");
			Serial.println(dotStyle);

			Serial.println("");
			Serial.println("Vertical mode (v 00-01): ");
			Serial.println(verticalMode);
			break;
		}
		//flush serial data
		Serial.flush();
	}
}

bool CheckLeapYear(int _year) {
	//return true if leap year
	if (_year % 4 != 0) return false;
	if (_year % 100 == 0 && _year % 400 != 0) return false;
	if (_year % 400 == 0) return true;
	return true;
}
