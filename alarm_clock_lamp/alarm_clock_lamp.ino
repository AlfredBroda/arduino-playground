/*
  DS3231 + TM1637 Real-Time Clock (with temperature display and alarm)
  Based on http://www.jarzebski.pl/arduino/komponenty/zegar-czasu-rzeczywistego-rtc-ds3231.html
  Libraries used:
  SevenSegmentTM1637 https://github.com/bremme/arduino-tm1637
  DS3231 https://github.com/jarzebski/Arduino-DS3231
  (c) 2016 by Alfred Broda
*/

#include <Wire.h>
#include <DS3231.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>

// RTC clock uses pins 2 DIO and 3 for CLK as default for Wire library
DS3231 clock;
RTCDateTime dt;

// Display module connection pins (Digital Pins)
#define DISP_CLK 5
#define DISP_DIO 4
SevenSegmentExtended display(DISP_CLK, DISP_DIO);

// For switching displayed information
#define TEMP_INTERVAL 5
int tempCounter = 0;

// Alarm digital pin for starting the alarm function
#define ALARM_PIN 6
#define ALARM_TIME 1800000
bool alarmTriggered = false;
int alarmLeft = ALARM_TIME;

void setup()
{
//  Serial.begin(9600);

  // Initialize DS3231
  clock.begin();

  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  // Set Alarm - Every 05h:30m:00s each day
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
//  clock.setAlarm1(0, 5, 30, 00, DS3231_MATCH_H_M_S);

  clock.setAlarm1(0, 22, 16, 00, DS3231_MATCH_H_M_S);
  clock.armAlarm1(true);

  // Initialize display
  display.begin();
  display.on();
  display.setBacklight(30);
  display.setPrintDelay(300);

  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);
}

void loop()
{
  dt = clock.getDateTime();

  if (tempCounter > TEMP_INTERVAL) //display temperature
  {
    clock.forceConversion();

    int temp = clock.readTemperature();
    uint8_t seg_temp[] = {
      display.encode(temp / 10),
      display.encode(temp % 10),
      B01100011, // degree sign
      TM1637_CHAR_C
    };

    display.setColonOn(false);
    display.printRaw(seg_temp);

    tempCounter = 0;
  } else { //display clock
    tempCounter++;
    display.printTime(dt.hour, dt.minute, true);

    // Call isAlarm1(false) if you want clear alarm1 flag manualy by clearAlarm1();
    if (clock.isAlarm1())
    {
      startAlarm();
    }
  
    // Call isAlarm2(false) if you want clear alarm1 flag manualy by clearAlarm2();
    if (clock.isAlarm2())
    {
      startAlarm();
    }
  }
  if (alarmTriggered && alarmLeft > 0) {
    runAlarm(alarmLeft);
    alarmLeft--;
  } else if (alarmLeft > 0) { // reset alarm counter
    alarmTriggered = false;
    alarmLeft = ALARM_TIME;
  }

  delay(1000);
}

void startAlarm() {
  alarmTriggered = true;
  display.blink();
  digitalWrite(ALARM_PIN, LOW);
}

void runAlarm(int left) {
  int progress = (left / ALARM_TIME) * 255;

  analogWrite(ALARM_PIN, progress);
}
