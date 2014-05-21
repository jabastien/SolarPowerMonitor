/*******************************
* Solar Power                  *
* Voltage and Current Readings *
* Version 0.1                  *
* Rob Westbrook                *
* Start Date: Feb 04 2014      *
* Last Modified: May 08 2014   *
********************************/

#include <Wire.h>              // include wire library for I2C
#include "RTClib.h"            // include library for real time clock
RTC_DS1307 rtc;                // create time object

/***********************************
* LCD is a 20 x 4 character display*
* LCD circuit:                     *
* LCD RS pin to digital pin 12     *
* LCD Enable pin to digital pin 11 *
* LCD D4 pin to digital pin 10     *
* LCD D5 pin to digital pin 9      *
* LCD D6 pin to digital pin 8      *
* LCD D7 pin to digital pin 7      *
* LCD R/W pin to ground            *
* 10K resistor:                    *
* end pins to +5V and ground       *
* wiper to LCD VO pin (pin 3)      *
************************************/

#include <LiquidCrystal.h>              // include the LCD library code:
LiquidCrystal lcd(12, 11, 10, 9, 8, 7); // initialize the LCD library with the numbers of the interface pins

int sample = 0;                 // track number of sample loops

/*** time variables ***/
unsigned long startTime = 0;    // unix start time when program begins
unsigned long nowTime = 0;      // unix time at time of loop
unsigned long onTime = 0;       // difference between start time and now time
String clockHour;               // store time hour for lcd display
String clockMin;                // store time minute for lcd display
int clockHourLen = 0;           // store length of hour string
int clockMinLen = 0;            // store length of minute string
char amPm[1];                   // array to store whether time is am or pm

/*** amp variables ***/
float currentValue = 0.0;       // amps analog input value
float loopCurrent = 0.0;        // amps value for one loop
float averageA = 0.0;           // amps average during loops
float totalAmps = 0.0;          // total amps generated
float averageAmps = 0.0;        // average amps - average = total divided by number of samples
float ampSeconds = 0.0;         // track amp-seconds
float ampHours = 0.0;           // total amp hours

/*** voltage variables ***/
float voltageValue = 0.0;       // voltage analog input value
float loopVoltage = 0.0;        // voltage value for one loop
float averageV = 0.0;           // voltage average during loops

/*** watts variables ***/
float totalWatts = 0.0;         // total watts generated
float averageWatts = 0.0;       // average watts - average = total divided by number of samples
float wattSeconds = 0.0;        // track watt-seconds
float wattHours = 0.0;          // total watt hours

void setup(){                                 // run once on start up
  
  Wire.begin();                               // start up I2C
  rtc.begin();                                // start up RTC clock
  if (! rtc.isrunning()) {                    // to reset time remove the "!" for one cycle
    rtc.adjust(DateTime(__DATE__, __TIME__)); // sets the RTC to the date & time this sketch was compiled
  }
  
  lcd.begin(20, 4);             // set up the LCD's number of columns and rows
  
  Serial.begin(9600);           // begin serial output
}                               // end setup
/************************************
*             END SETUP             *
*                                   *
*            BEGIN LOOP             *
************************************/

void loop(){

/******** Begin Time Routines *********/
  DateTime now = rtc.now();               // get current time
  doCurrentTime();                        // do routine to display current tim
  
  /*** do at midnight to reset everything ***/
  if(now.hour() == 0 && now.minute() == 0 && now.second() < 5) { // if new day reset everything
    newDayReset();                        // do routine to reset variables for new day
  }
  
  /*** do on startup ***/
  if(sample == 0) {
    startTime = now.unixtime();         // if this is the first loop then set start time
    nowTime = startTime;                // if this is first loop then set now time
    Serial.print("Sample Number = ");
    Serial.print(sample);
    Serial.print(" - Unix Time = ");
    Serial.println(startTime);
  }
/******** End Time Routines ********/

/****************************************************************
* get current reading                                           *
* how to get this formula:                                      *
*                                                               *
*           (.0049 x counts) - 2.5                               *
* current = ----------------------                                 *
*                   .066                                        *
*                                                               *
* 5 volts is max input to analog input                          *
* 1024 counts is the highest count for analog input             *
* 5 volts divided by 1024 counts equals .0049 volts per count   *
* Since this sensor reads both negative and positive current    *
* subtract 2.5v, the mid-point of 5v and half the input.        *
* 2.5v midpoint calibrated to 2.501 for 0.                      *
* .066 is the voltage sensitivity of the 30a ACS712             *
*****************************************************************/
  averageA = 0.0;                       // zero out average before the loop
  loopCurrent = 0.0;                    // zero out before the loop
  for(int x = 0; x < 100; x++){         // take 100 samples for average current reading
    currentValue = analogRead(A1);      // read current sensor
    loopCurrent = ((.0049 * currentValue) - 2.501)/.066; // convert to current reading
    averageA = averageA + loopCurrent;  // calculate average current in this loop
    delay(10);                          // 10 mSec between each reading
  }
/****** end current sensor routine ******/
  
/**************************************************************
* Voltage Divider reads 0 to 80 V                             *
* 80V = 5V in analog input                                    *
* 5V divided by total of 1024 counts = .00488281 volts/count  *
* Voltage divider is 15 to 1 divider so the                   *
* Count is 16 times smaller than actual reading               *
* Multiply voltage by 16 for correct voltage                  *
***************************************************************/
  averageV = 0.0;                   // zero out average before the loop
  loopVoltage = 0.0;                // zero out before loop
  for(int y = 0; y < 100; y++){     // take 100 samples for average voltage reading
    voltageValue = analogRead(0);   // read voltage sensor
    loopVoltage = (voltageValue * .00488281) * 16;  // convert to voltage 
    averageV = averageV + loopVoltage; // calculate average voltage in this loop
    delay(10);                         // 10 mSec delay between each reading
  }
/****** end voltage sensor routine ******/
  
  /* calculate amps, volts, and watts for this loop */
  float finalA = averageA/100;    // divide by 100, the number of samples taken
  float finalV = averageV/100;    // divide by 100, the number of samples taken
  float finalW = finalV * finalA; // watts = volts x amps
  
  /* Calculate amp hours and watt hours */
  sample = sample + 1;                           // track number of loops
  nowTime = now.unixtime();                      // get current unix time
  onTime = nowTime - startTime;                  // on time = the time now minus the start time
  
  totalAmps = totalAmps + finalA;                // track total amps
  averageAmps = totalAmps/sample;                // average the amps
  ampSeconds = abs(averageAmps * float(onTime)); // convert to amp seconds
  ampHours = ampSeconds/3600;                    // convert to amp hours
  
  totalWatts = totalWatts + finalW;                 // track total watts
  averageWatts = totalWatts/sample;                 // average the watts
  wattSeconds = abs(averageWatts * float(onTime));  // convert to watt seconds
  wattHours = wattSeconds/3600;                     // convert to watt hours

  Serial.println("////////////////////////////////");
  Serial.print("Sample No: ");
  Serial.println(sample);
  Serial.println("////////////////////////////////");
  Serial.println("Voltage Values");
  Serial.print("Voltage Value at Analog Input: ");
  Serial.print(voltageValue);
  Serial.print(" - Volts = ");
  Serial.print(finalV); 
  Serial.println(" V");
  Serial.println("////////////////////////////////");
  Serial.println("Current Values");
  Serial.print("Current Value at Analog Input: ");
  Serial.print(currentValue);
  Serial.print(" - Amps = ");
  Serial.print(finalA);
  Serial.print(" - Total Amps: ");
  Serial.print(totalAmps);
  Serial.print(" - Average Amps = ");
  Serial.print(averageAmps);
  Serial.print(" - Amp Seconds: ");
  Serial.print(ampSeconds);
  Serial.print(" - Amp Hours: ");
  Serial.println(ampHours);
  Serial.println("////////////////////////////////");
  Serial.println("Watt Values");
  Serial.print("Power: ");
  Serial.print(finalW);
  Serial.print(" - Total Watts: ");
  Serial.print(totalWatts);
  Serial.print(" - Average Watts: ");
  Serial.print(averageWatts);
  Serial.print(" - Watt Seconds: ");
  Serial.print(wattSeconds);
  Serial.print("  - WattHours = ");
  Serial.println(wattHours);
  Serial.println("////////////////////////////////");
  Serial.println("Time Values");
  Serial.print("OnTime = ");
  Serial.print(onTime);
  Serial.print(" Sec - ");
  Serial.print("Time: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  
  Serial.println(" ");
  Serial.println(" ");
  
  /*** avoid negatives ***/
  finalV = abs(finalV);
  finalA = abs(finalA);
  finalW = abs(finalW);
  
  /*** print values to lcd ***/
  displayVolts(finalV);         // run function to print current volts to lcd
  displayAmps(finalA);          // run function to print current amps to lcd
  displayAmpHours(ampHours);    // run function to print amp hours to lcd
  displayWatts(finalW);         // run function to print current watts to lcd
  displayWattHours(wattHours);  // run function to print watt hours to lcd
  
  delay(1000);        // wait 1 second then do loop again
}
/************************************
*             END LOOP              *
*                                   *
*         BEGIN FUNCTIONS           *
************************************/


/**** print current time to lcd ****/
void doCurrentTime() {
  DateTime now = rtc.now();                 // get current time
  int nowHour = now.hour();                 // hour to integer
  int nowMinute = now.minute();             // minute to integer
  if (nowHour == 0) {                       // if it's the midnight hour
    int midnightHour = 12;
    clockHour = String(midnightHour);       // set hour as 12
  } else if(nowHour > 0 && nowHour < 10) {  // if it's between 1:00 am and 9:59 am
    clockHour = ' ' + String(nowHour);      // add leading space
  } else if(nowHour > 12) {                 // if it's after 12:00 noon
    int eveningHour = nowHour - 12;         // convert to pm hour
    if(eveningHour < 10) {
      clockHour = ' ' + String(eveningHour);
    } else {
      clockHour = String(eveningHour);
    }
  } else {
    clockHour = String(nowHour);            // can only be 10 or 11 am
  }

  if(nowMinute < 10) {                      // add leading zero to minute
      clockMin = '0' + String(nowMinute);   // if minute is less than 10
  } else {
      clockMin = String(nowMinute);
  }
  
  if(nowHour < 12) {                        // set am or pm
    amPm[0] = 'a';
  } else {
    amPm[0] = 'p';
  }
  lcd.setCursor(0, 0);
  lcd.print("  Solar to Battery");          // print screen title
  lcd.setCursor(9,1);
  lcd.print("Time:");
  lcd.setCursor(14,1);
  lcd.print(clockHour);
  lcd.setCursor(16,1);
  lcd.print(":");
  lcd.setCursor(17,1);
  lcd.print(clockMin);
  lcd.setCursor(19,1);
  lcd.print(amPm[0]);
}
/**** end print current time to lcd ****/

/**** reset everything at beginning of new day ****/
void newDayReset() {
  sample = 0;             // reset all variables to zero
  totalAmps = 0.0;
  totalWatts = 0.0;
  averageAmps = 0.0;
  averageWatts = 0.0;
  ampSeconds = 0.0;
  wattSeconds = 0.0;
  ampHours = 0.0;
  wattHours = 0.0;
  lcd.setCursor(14, 2);   
  lcd.print("      ");    // clear amp hour on lcd display
  lcd.setCursor(14, 3);
  lcd.print("      ");    // clear watt hour on lcd display
}
/**** end reset everything at beginning of new day ****/

/*** print voltage to lcd ***/
void displayVolts(float finalV) {
  lcd.setCursor(0, 1);
  lcd.print("V:");
  lcd.setCursor(3, 1);
  if(finalV < 10) {           // right justify
    lcd.print(" ");
    lcd.setCursor(4, 1);
    lcd.print(finalV, 2);
  } else {
    lcd.print(finalV, 2);
  }
}
/*** end print voltage to lcd ***/

/*** print amps to lcd ***/
void displayAmps(float finalA) {
  lcd.setCursor(0, 2);
  lcd.print("A:");
  lcd.setCursor(3, 2);
  if(finalA < 10) {           // right justify
    lcd.print(" ");
    lcd.setCursor(4, 2);
    lcd.print(finalA, 2);
  } else {
    lcd.print(finalA, 2);
  }
}
/*** end print amps to lcd ***/

/*** print watts to lcd ***/
void displayWatts(float finalW) {
  lcd.setCursor(0, 3);
  lcd.print("W:");
  lcd.setCursor(2, 3);
  if(finalW < 10) {                       // right justify
    lcd.print("  ");
    lcd.setCursor(4, 3);
    lcd.print(finalW, 2);
  } else if(finalW > 9 && finalW < 99) {
    lcd.print(" ");
    lcd.setCursor(3, 3);
    lcd.print(finalW, 2);
  } else {
    lcd.print(finalW, 2);
  }
}
/*** end print watts to lcd ***/

/*** print amp hours to lcd ***/
void displayAmpHours(float ampHours) {
  lcd.setCursor(11, 2);
  lcd.print("AH:");
  lcd.setCursor(15, 2);
  if(ampHours < 10) {                        // right justify
    lcd.print("  ");
    lcd.setCursor(17,2);
    lcd.print(ampHours, 1);
  } else if(ampHours > 9 && ampHours < 99) {
    lcd.print(" ");
    lcd.setCursor(16,2);
    lcd.print(ampHours, 1);
  } else {
    lcd.print(ampHours, 1);
  }
}
/*** end print amp hours to lcd ***/

/*** print watt hours to lcd ***/
void displayWattHours(float wattHours) {
  lcd.setCursor(11, 3);
  lcd.print("WH:");
  lcd.setCursor(15, 3);
  if(wattHours < 10) {                        // right justify
    lcd.print("  ");
    lcd.setCursor(17, 3);
    lcd.print(wattHours, 1);
  } else if(wattHours > 9 && wattHours < 99) {
    lcd.print(" ");
    lcd.setCursor(16, 3);
    lcd.print(wattHours, 1);
  } else {
    lcd.print(wattHours, 1);
  }
}
/*** end print watt hours to lcd ***/
