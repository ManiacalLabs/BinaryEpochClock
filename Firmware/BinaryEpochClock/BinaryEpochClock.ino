/*
Version 1.0
 Copyright (c) 2013 Adam Haile & Dan Ternes.  All right reserved.
 http://ManiacalLabs.com
 */

/*
Pinout Information
 PC0 - PC3: Common cathode Outputs for display
 PINB0-PINB5,PIND6-PIND7: Display row anodes
 PC4 & PC5: SDA * SCL for I2C to RTC 
 PIND0 & PIND1: RX & TX for Serial
 PIND2 & PIND3: Mode & Change buttons
 PIND5: Reset Enable/Disable
 */

#include <Wire.h>
#include "RTClib.h"
#include "EEPROM.h"

#include "globals.h"

//Sets the individual digits of a number 0-99
static void setTimeVals(int in, uint8_t idx) {
  setValues[idx    ] = in / 10;
  setValues[idx + 1] = in - (setValues[idx] * 10);
}

/*
Moves forward the value at curSet and constrains
 to proper date/time conventions including months,
 leap years, etc.
 Much of this borrowed from: 
 https://github.com/adafruit/TIMESQUARE-Watch/blob/master/examples/Watch/Set.pde
 */
inline void incrementSetValue()
{
  uint8_t upper, lower;
  upper = pgm_read_byte(&limit[curSet]);

  if(((curSet == SET_MO2) || (curSet == SET_D2)) && (setValues[curSet - 1] == 0)) {
    lower = 1;
  }
  else {
    lower = 0;

    if(curSet == SET_MO2) {
      if(setValues[SET_MO1] == 1) upper = 2;
    } 
    else if(curSet == SET_D2) {
      // Cap most months at 30 or 31 days as appropriate
      if(setValues[SET_D1] == 3) {
        uint8_t m = setValues[SET_MO1] * 10 + setValues[SET_MO2];
        upper = pgm_read_byte(&daysInMonth[m - 1]) % 10;
        // Finally, the dreaded leap year...
      } 
      else if((setValues[SET_D1] == 2) && (setValues[SET_MO1] == 0) &&
        (setValues[SET_MO2] == 1)) {
        uint8_t y = setValues[SET_Y1] * 10 + setValues[SET_Y2];
        if((y == 0) || (y & 3)) upper = 8; // non-leap year
      }
    } 
    else if(curSet == SET_H2) {
      if(setValues[SET_H1] == 2) upper = 3;
    }

    if(upper == lower) return;

  }

  if(++setValues[curSet] > upper) setValues[curSet] = lower;

  uint8_t m, d;
  switch(curSet) {
  case SET_MO1:
    if((setValues[SET_MO1] == 1) && (setValues[SET_MO2] > 2))
      setValues[SET_MO2] = 0;
    // Lack of break is intentional...all subsequent constraints apply.
  case SET_MO2:
    m = setValues[SET_MO1] * 10 + setValues[SET_MO2];
    if(m < 1) setTimeVals(1, SET_MO1);
  case SET_D1:
    m     = setValues[SET_MO1] * 10 + setValues[SET_MO2];
    d     = setValues[SET_D1] * 10 + setValues[SET_D2];
    upper = pgm_read_byte(&daysInMonth[m - 1]);
    if(m == 2) { // is February
      uint8_t y = setValues[SET_Y1] * 10 + setValues[SET_Y2];
      if((y > 0) && !(y & 3)) upper = 29; // is leap year
    }
    if(d > upper)  setTimeVals(upper, SET_D1);
    else if(d < 1) setTimeVals(1, SET_D1);
  case SET_H1:
  case SET_H2:
    if((setValues[SET_H1] * 10 + setValues[SET_H2]) > 23)
      setTimeVals(23, SET_H1);
  }

  timeUpdated = setChanged = true;
}

//move to next set value and wrap if needed
inline void nextSetValue()
{
  curSet++;
  if(curSet > SET_MN2) curSet = SET_Y1;
  setChanged = true;
}

//Populate setValues with the current time
inline void loadSetVals()
{
  setValues[SET_Y1] = (dt_now.year() - 2000) / 10;
  setValues[SET_Y2] = (dt_now.year() - 2000) % 10;
  setValues[SET_MO1] = dt_now.month() / 10;
  setValues[SET_MO2] = dt_now.month() % 10;
  setValues[SET_D1] = dt_now.day() / 10;
  setValues[SET_D2] = dt_now.day() % 10;
  setValues[SET_H1] = dt_now.hour() / 10;
  setValues[SET_H2] = dt_now.hour() % 10;
  setValues[SET_MN1] = dt_now.minute() / 10;
  setValues[SET_MN2] = dt_now.minute() % 10;
}

//push setValues data to the RTC
inline void adjustFromSetVal()
{
  DateTime set = DateTime(
  (setValues[SET_Y1] * 10) + setValues[SET_Y2] + 2000,
  (setValues[SET_MO1] * 10) + setValues[SET_MO2],
  (setValues[SET_D1] * 10) + setValues[SET_D2],
  (setValues[SET_H1] * 10) + setValues[SET_H2],
  (setValues[SET_MN1] * 10) + setValues[SET_MN2] ,
  0
    );

  RTC->adjust(set);
}

//Output current DateTime to serial debug
//Generally only used during testing since 
//serial is normally turned off
/*
void PrintDate(const DateTime& dt)
 {
 OD(dt.year()); 
 OS(" "); 
 OD(dt.month()); 
 OS(" ");
 OD(dt.day()); 
 OS(" - ");
 OD(dt.hour()); 
 OS(":"); 
 OD(dt.minute()); 
 OS(":"); 
 OD(dt.second());
 Serial.println();
 }
 */

/*
Keeping the EEPROM stuff simple. 
 Only thing to save is the PWM level so 
 we just read it in the 0th byte of the 
 EEPROM register and then constrain to max/min
 */
inline void ReadPwmLevel()
{
  pwmLevel = EEPROM.read(0);
  if(pwmLevel <= 0 || pwmLevel > pwmMax)
    pwmLevel = pwmMax;
}

void setup()
{
  ReadPwmLevel();  

  /*
  Support for multiple RTC chip types including software only.
   RTC_METHOD is set in globals.h
   The DS3231 has nearly identical registers to the DS1307 so if using that chip there
   is no real need to update RTC_METHOD, it will just work. Besides the current hardware has 
   no place for the DS3231 (Chronodot)... maybe in the future.
   You can, however, leave out the DS1307 chip and set RTC_METHOD to USE_SOFTRTC/
   But note that there is no battery backup in this case and you will loose time if you loose power.
   */
  if(RTC_METHOD == USE_DS1307)
    RTC = new RTC_DS1307(); 
  else if(RTC_METHOD == USE_DS3231)
    RTC = new RTC_DS3231(); 
  else
    RTC = new RTC_SOFT(DateTime(__DATE__, __TIME__));

  //init reference for time delays. used with TimeElapsed()  calls.
  //set to 0 instead of millis() so it always triggers right away the first time
  timeRef = 0;

  //Setup common cathodes as outputs
  DDRC |= (_BV(PINC0) | _BV(PINC1) | _BV(PINC2) | _BV(PINC3));
  //Setup rows as outputs
  DDRB |= (_BV(PINB0) | _BV(PINB1) | _BV(PINB2) | _BV(PINB3) | _BV(PINB4) | _BV(PINB5));
  DDRD |= (_BV(PIND6) | _BV(PIND7));

  //Set PORTD2 and PORTD3 as inputs
  DDRD &= ~(_BV(PIND2) | _BV(PIND3));
  //Enable set/change button pullups
  PORTD |= (_BV(PIND2) | _BV(PIND3));
  //Enable I2C pullups - probably not really necessary since twi.h does this for you
  PORTC |= (_BV(PINC4) | _BV(PINC5));

  //see setResetDisable(bool) for info
  DDRD &= ~(_BV(PIND5)); //Set PIND5 as input initially
  PORTD |= (_BV(PIND5)); //Set high

  //check for buttons held down at power up
  bSave = BUTTON_STATE;

  //init global time value
  unix = 0;

  Wire.begin();

  //Grab current time and set to compile time if RTC is uniitialized
  dt_now = RTC->now();
  if(dt_now.unixtime() <= SECONDS_FROM_1970_TO_2000 + 60) //RTC not initialized and oscillator disabled
  {
    dt_now = DateTime(__DATE__, __TIME__);
    RTC->adjust(dt_now);
  }
  unix = dt_now.unixtime();

  if(bSave != BUTTON_MASK && bSave & ~BUTTON_B)
  {
    //check again just to be sure. RTC-now() has given a bit of a delay
    bSave = BUTTON_STATE;
    if(bSave != BUTTON_MASK && bSave & ~BUTTON_B)
    {
      curState = STATE_PAUSE;
      unix = 0xFFFFFFFF; 
      timeOutRef = millis();
      holdFlag = true;
    }
  }

  setInterrupts();
}

/*
  Used for software disable of reset on Serial connection.
 Setting PIND5 to an output through a 110 ohm resistor 
 later will place 5V at low impedence on the reset pin, 
 preventing it from reseting when RTS is pulsed on connection.
 */
inline void setResetDisable(bool state)
{
  if(state)
    DDRD |= (_BV(PIND5));
  else
    DDRD &= ~(_BV(PIND5));  
}  

//Setup all things interrupt related
inline void setInterrupts()
{
  //disable interrupt  timers
  cli();
  // Set up interrupt-on-change for buttons.
  EICRA = _BV(ISC10)  | _BV(ISC00);  // Trigger on any logic change
  EIMSK = _BV(INT1)   | _BV(INT0);   // Enable interrupts on pins
  bSave = BUTTON_STATE; // Get initial button state

  //Setup Display Refresh Interrupt
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0

    // set compare match register for 6400hz (800hz screen refresh) increments
  OCR1A = 2500;// = (16*10^6) / (1*6400) - 1 
  // turn on CTC mode
  TCCR1B |= _BV(WGM12);

  TCCR1B |= PRESCALE1_1;  
  // enable timer compare interrupt
  TIMSK1 |= _BV(OCIE1A);

  //Setup Timer2 interrupts for button handling
  //Runs at about 60Hz, which is as slow as we can go
  TCCR2A = 0;// set entire TCCR1A register to 0
  TCCR2B = 0;// same for TCCR1B
  TCNT2  = 0;//initialize counter value to 0

    // set compare match register for max
  OCR2A = 255;
  // turn on CTC mode
  TCCR2B |= _BV(WGM21);

  TCCR2B |= PRESCALE2_1024;  
  // enable timer compare interrupt
  TIMSK2 |= _BV(OCIE2A);

  //enable interrupt timers
  sei();
}

inline void endSerialSet()
{
	Serial.end();
    setResetDisable(false);
    curState = STATE_NONE;
}

//Timer2 interrupt for handling button presses
ISR(TIMER2_COMPA_vect)
{
  // Check for button 'hold' conditions
  if(bSave != BUTTON_MASK) 
  { // button(s) held
    if(bCount >= holdMax && !holdFlag) 
    { //held passed 1 second
      holdFlag = true;
      bCount = 0;
      if(bSave & ~BUTTON_A)
      {
        if(curState == STATE_NONE)
        {
          timeReady = false;
          setCancel = false;
          curState = STATE_MANUAL_SET;
          curSet = SET_Y1;
          loadSetVals();
          setChanged = true;
        }
        else if(curState == STATE_MANUAL_SET)
        {
          //set to new time
          timeReady = true;
        }
        else if(curState == STATE_PAUSE)
        {
          curState = STATE_NONE;
          ReadPwmLevel(); 
        }
		else if(curState == STATE_SERIAL_SET)
        {
			endSerialSet();
        }
      }
      else if(bSave & ~BUTTON_B)
      {
        if(curState == STATE_NONE)
        {
          curState = STATE_PAUSE;  
          timeOutRef = millis();  
        }
        else if(curState == STATE_PAUSE)
        {
          curState = STATE_NONE;
          ReadPwmLevel();   
        }
        else if(curState == STATE_MANUAL_SET)
        {
          setCancel = true;
        }
		else if(curState == STATE_SERIAL_SET)
        {
			endSerialSet();
        }
      }
      else
      {
        if(curState == STATE_NONE)
        {
          setResetDisable(true);
          Serial.begin(SERIAL_BAUD);
          _serialScan = 16; //start in middle
          _serialScanDir = false;
          timeOutRef = millis();
          curState = STATE_SERIAL_SET;
        }
        else if(curState == STATE_SERIAL_SET)
        {
			endSerialSet();
        }
        else if(STATE_PAUSE)
        {
          curState = STATE_NONE;
          ReadPwmLevel();  
        }
      }
    } 
    else bCount++; // else keep counting...
  } 
}

//Button external interrupts
ISR(INT0_vect) {
  uint8_t state = BUTTON_STATE;
  if(state == BUTTON_MASK) //both are high meaning they've been released
  {
    if(holdFlag)
    {
      holdFlag = false;
      bCount = 0;
    }
    else if(bCount > 3) //past debounce threshold
    {
      if(bSave & ~BUTTON_A)
      {
        if(curState == STATE_MANUAL_SET)
        {
          nextSetValue();
        }
      }
      else if(bSave & ~BUTTON_B)
      {
        if(curState == STATE_NONE)
        {
          //Change PWM level
          pwmLevel--;
          if(pwmLevel <= 0 || pwmLevel > pwmMax)
            pwmLevel = pwmMax;

          EEPROM.write(0, pwmLevel);
        }
        else if(curState == STATE_MANUAL_SET)
        {
          incrementSetValue();
        }
      }
    }
    bCount = 0;
  }
  else if(state != bSave) {
    bCount = 0; 
  }

  bSave = state;
}

//Use the same handler for both INT0 and INT1 interrupts
ISR(INT1_vect, ISR_ALIASOF(INT0_vect));

/*
Where the magic happens. All the multiplexing is done here.
 We start by disabling all the "columns" which actually means turning them to high
 since this is common ground. 
 If in serial set mode we do a larson scanner instead of show the time.
 Otherwise set the states of each of the 4 LEDs in each row and then finally
 re-enable the column for this pass through the loop.
 It requires 8 passes (1 per column) through this to update the display once.
 As this is called at 6400Hz, we update the display fully at 800Hz.
 */
volatile uint8_t col = 0, row = 0;
volatile uint8_t scan_low = 0, scan_high = 0;
#define SCAN_WIDTH 2
ISR(TIMER1_COMPA_vect)
{
  //Turn all columns off (High is off in this case since it's common cathode)
  PORTB |= (_BV(PINB0) | _BV(PINB1) | _BV(PINB2) | _BV(PINB3) | _BV(PINB4) | _BV(PINB5));
  PORTD |= (_BV(PIND6) | _BV(PIND7));

  if(curState == STATE_SERIAL_SET)
  {
    scan_low = _serialScan >= SCAN_WIDTH ? _serialScan - SCAN_WIDTH : 0;
    scan_high = _serialScan <= (31 - SCAN_WIDTH) ? _serialScan + SCAN_WIDTH : 31;
    byte _scanOffset = 0;
    byte _curStep = 0; 

    //set the 4 rows
    for(row=0; row<4; row++)
    {
      _curStep = (row + (col * 4));

      if(_curStep < _serialScan) _scanOffset = (_serialScan - _curStep);
      else if(_curStep > _serialScan) _scanOffset = (_curStep - _serialScan);
      else _scanOffset = 0;

      if((_curStep >= scan_low && _curStep <= scan_high) &&
        (_serialScanStep < scanLevels[_scanOffset]))
        PORTC |= _BV(row);
      else
        PORTC &= ~_BV(row);
    }

    //Enable the current column
    if(col < 6)
      PORTB &= ~_BV(col);
    else
      PORTD &= ~_BV(col);
  }
  else
  {
    if(pwmStep < pwmLevel)
    {
      //set the 4 rows
      for(row=0; row<4; row++)
      {
        if(unix & (1UL << (row + (col * 4))))
          PORTC |= _BV(row);
        else
          PORTC &= ~_BV(row);
      } 

      //Enable the current column
      if(col < 6)
        PORTB &= ~_BV(col);
      else
        PORTD &= ~_BV(col);
    }
  }

  col++;
  if(col == 8)
  { 
    col = 0;
    pwmStep++;
    if(pwmStep == pwmMax)
      pwmStep = 0;

    if(curState == STATE_SERIAL_SET)
    {
      _serialScanStep++;
      if(_serialScanStep == 10) _serialScanStep = 0; 
    }
  }
}

//Helper for time delays without actually pausing execution
bool TimeElapsed(unsigned long ref, unsigned long wait)
{
  unsigned long now = millis();

  if(now < ref || ref == 0) //for the 50 day rollover or first boot
    return true;  

  if((now - ref) > wait)
    return true;
  else
    return false;
}

/*
Get time from serial connection
 Time data is just the letter 't' followed by 4 bytes 
 representing a unix epoch 32-bit time stamp. 
 The time is assumed to already be adjusted for local 
 time zone and DST by the sending computer as RTC does not 
 store time as UTC.
 */
bool getPCTimeSync()
{
  bool result = false;
  if(Serial.available() >= SYNC_LEN)
  {
    byte buf[5];
    memset(buf, 0, 5);
    Serial.readBytes((char *)buf, 5);
    if(buf[0] == SYNC_HEADER)
    {
      uint32_t t = 0;
      memcpy(&t, buf+1, 4);
      RTC->adjust(DateTime(t));
      Serial.write(42); //Writes out to confirm we got it (sends as a *)
      Serial.flush();
      result = true;
      while(Serial.available()) Serial.read(); //clear the bufer
      Serial.end();
      setResetDisable(false);
    }
    else if(buf[0] == GET_HEADER)
    {
      dt_now = RTC->now();
      uint32_t time = dt_now.unixtime();
      memset(buf, 0, 5);
      buf[0] = SYNC_HEADER;
      memcpy(buf+1, &time, 4);
      Serial.write(buf, 5);
      Serial.flush();
      result = true;
    }
  }

  return result;
}

/*
The main program state machine.
 Most of the time this will just be updating the time every 500ms
 (1000ms can cause noticable jumps if program execution takes longer than normal).
 */
void loop()
{

  /* if(curState == STATE_NONE) //commented out to try new method of reading clock
  {
    if(TimeElapsed(timeRef, 500))
    {
      timeRef = millis();
      dt_now = RTC->now();
      unix = dt_now.unixtime();
    }
  }
  */
  
  if(curState == STATE_NONE)
  {
    if(TimeElapsed(timeRef, 500))
    {
      timeRef = millis();
      dt_now = RTC->now();
      
      year = dt_now.year - 2000;

	valcheck = 32;
	for (y=31; y>=26; y--)
	{
	if (year >= valcheck) {
		unix |= (1UL << y);
		year = year - valcheck;
		}
	else {
		unix &= ~(1UL << y);
		}
	valcheck = valcheck / 2;
	}

	month = dt_now.month;

	valcheck = 8;
	for (m=25; m>=22; m--)
	{
	if (month >= valcheck) {
		unix |= (1UL << m);
		month = month - valcheck;
		}
	else {
		unix &= ~(1UL << m);
		}
	valcheck = valcheck / 2;
	}

	day = dt_now.day;

	valcheck = 16;
	for (d=21; d>=17; d--)
	{
	if (day >= valcheck) {
		unix |= (1UL << d);
		day = day - valcheck;
		}
	else {
		unix &= ~(1UL << d);
		}
	valcheck = valcheck / 2;
	}


	hour = dt_now.hour;

	valcheck = 16;
	for (h=16; h>=12; h--)
	{
	if (hour >= valcheck) {
		unix |= (1UL << h);
		hour = hour - valcheck;
		}
	else {
		unix &= ~(1UL << h);
		}
	valcheck = valcheck / 2;
	}

	minute = dt_now.minute;

	valcheck = 32;
	for (mm=11; mm>=6; mm--)
	{
	if (minute >= valcheck) {
		unix |= (1UL << mm);
		minute = minute - valcheck;
		}
	else {
		unix &= ~(1UL << mm);
		}
	valcheck = valcheck / 2;
	}

	second = dt_now.second;

	valcheck = 32;
	for (s=5; s>=0; s--)
	{
	if (second >= valcheck) {
		unix |= (1UL << s);
		second = second - valcheck;
		}
	else {
		unix &= ~(1UL << s);
		}
	valcheck = valcheck / 2;
	}
}
}
  
  else if(curState == STATE_PAUSE)
  {
    if(TimeElapsed(timeOutRef, 600000))
    {
      curState = STATE_NONE;
    }
    else
    {
      if(TimeElapsed(timeRef, 50))
      {
        timeRef = millis();
        if(pwmLevel == pwmMax) fadeDir = false;
        else if(pwmLevel == 1) fadeDir = true;
        pwmLevel += (fadeDir ? 1 : -1);
      }
    }
  }
  else if(curState == STATE_MANUAL_SET)
  {
    if(timeReady)
    {
      adjustFromSetVal();
      curState = STATE_NONE; 
    }
    else if(setChanged)
    {
      setChanged = false;
      uint32_t temp = 0;
      if(setValues[curSet])
        temp += ((1UL << setValues[curSet]) - 1);

      temp |= (1UL << (31 - curSet));   
      unix = temp;
    }
    else if(setCancel)
    {
      curState = STATE_NONE;
    }

  }
  else if(curState == STATE_SERIAL_SET)
  {
    if(TimeElapsed(timeOutRef, 30000))
    {
      Serial.end();
      setResetDisable(false);
      curState = STATE_NONE;
    }
    else
    {
      if(TimeElapsed(timeRef, 40))
      {
        timeRef = millis();
        if(_serialScan == 31) _serialScanDir = false;
        else if(_serialScan == 0) _serialScanDir = true;
        _serialScan += (_serialScanDir ? 1 : -1);
      }

      //check for serial data for time sync
      if(getPCTimeSync())
        curState = STATE_NONE;
    }
  }
}
































