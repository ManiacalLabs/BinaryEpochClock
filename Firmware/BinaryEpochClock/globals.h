
#ifndef __GLOBALS__
#define __GLOBALS__

#include "Arduino.h"
#include "RTCLib.h"

volatile uint32_t dispBuf;
unsigned long timeRef;
unsigned long timeOutRef;
uint8_t pwmStep = 0;
const uint8_t pwmMax = 10;
volatile uint8_t pwmLevel = 10; //range 5 - 10 (50% - 100%)
bool fadeDir = false;

//RTC Keeping method
#define USE_DS1307   0
#define USE_DS3231   1
#define USE_SOFTRTC  2

//Change this to one of the three above to change which chip (or software) is used
#define RTC_METHOD USE_DS1307

RTC_Base *RTC = NULL;

volatile uint8_t bSave = 0;
volatile uint16_t bCount = 0;
volatile boolean holdFlag = false;
uint16_t holdMax = 60;

//Valid state machine states
#define STATE_FULL_BINARY       0
#define STATE_SUB_BINARY		1
#define STATE_PONG				2
#define STATE_MANUAL_SET		3
#define STATE_SERIAL_SET		4
#define STATE_PAUSE				5

volatile uint8_t curState = STATE_FULL_BINARY;
volatile uint8_t prevState = STATE_FULL_BINARY;

//For manually setting the time and used with curSet
#define SET_Y1 		0
#define SET_Y2 	 	1
#define SET_MO1 	2
#define SET_MO2	        3
#define SET_D1		4
#define SET_D2		5
#define SET_H1		6
#define SET_H2		7
#define SET_MN1	        8
#define SET_MN2	        9

//helpers for button handling
#define BUTTON_A _BV(PIND2)
#define BUTTON_B _BV(PIND3)
#define BUTTON_MASK (BUTTON_A | BUTTON_B)
#define BUTTON_STATE PIND & BUTTON_MASK

//Who can ever remember what the prescaler combinations are?
//These are for Timer0
#define PRESCALE0_1 _BV(CS00)
#define PRESCALE0_8 _BV(CS01)
#define PRESCALE0_64 (_BV(CS01) | _BV(CS00))
#define PRESCALE0_256 _BV(CS02)
#define PRESCALE0_1024 (_BV(CS02) | _BV(CS00))

//These are for Timer1
#define PRESCALE1_1 _BV(CS10)
#define PRESCALE1_8 _BV(CS11)
#define PRESCALE1_64 (_BV(CS11) | _BV(CS10))
#define PRESCALE1_256 _BV(CS12)
#define PRESCALE1_1024 (_BV(CS12) | _BV(CS10))

//These are for Timer2
#define PRESCALE2_1 _BV(CS20)
#define PRESCALE2_8 _BV(CS21)
#define PRESCALE2_32 (_BV(CS21) | _BV(CS20))
#define PRESCALE2_64 _BV(CS22)
#define PRESCALE2_128 (_BV(CS20) | _BV(CS22))
#define PRESCALE2_256 (_BV(CS22) | _BV(CS21))
#define PRESCALE2_1024 (_BV(CS22) | _BV(CS21) | _BV(CS20))

volatile uint8_t curSet = SET_Y1;
volatile uint8_t setValues[10] = {
	0,0,0,0,0,0,0,0,0,0};
DateTime dt_now = DateTime();
bool setChanged = false;
bool timeUpdated = false;
bool timeReady = false;
bool setCancel = false;

//sub-binary globals
uint8_t sbCurBit;
uint8_t sbOffsets[] = {0, 6, 12, 17, 22, 26};
uint8_t sbBits[] = {5, 5, 4, 4, 3, 5};
uint8_t sbVals[6];


//Limits for each field value and for months
//Store in progmem since they are rarely used.
PROGMEM uint8_t
	//Y   Y . M   M . D  D   H   H : M   M
	limit[]       = {  
		9,  9,  1,  9,  3,  9,  2,  9,  5,  9}
,
	daysInMonth[] = { 
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

//Used for the larson scanner effect during Serial Set mode.
volatile byte _serialScan = 0;
volatile byte _serialScanStep = 0;
bool _serialScanDir = false;
const uint8_t scanLevels[] = {10,3,1}; 

uint8_t _pongBall = 16;
uint8_t _pongLScore = 0;
uint8_t _pongRScore = 0;
bool _pongDir = false;
uint32_t _pongPaddles = 0;
bool _pongShowScore = false;
#define PONG_TIMEOUT 100
uint16_t _pongTimeout = PONG_TIMEOUT;

//For getting the time over serial connection
#define SYNC_LEN 5  // time sync is 1 byte header + 4 byte time_t
#define SYNC_HEADER 't'  // header for time sync
#define GET_HEADER 'g'

//Change this to whatever your computer serial connection is set to
//TODO: Change this to whatever the arduino drivers default to.
#define SERIAL_BAUD 115200

//Helpers for Serial printing
#define OD(x) Serial.print(x, DEC)
#define OS(x) Serial.print(x)
#endif //__GLOBALS__
