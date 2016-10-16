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
bool displayTemp = false;

// Alarm digital pin for starting the alarm function
#define ALARM 6

void setup()
{
//  Serial.begin(9600);

  // Initialize DS3231
//  Serial.println("Initialize DS3231");;
  clock.begin();

  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  // Set Alarm - Every 05h:30m:00s each day
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  clock.setAlarm1(0, 5, 30, 00, DS3231_MATCH_H_M_S);

  // Initialize display
  display.off();
  display.setBacklight(50);
  display.on();

  pinMode(ALARM, OUTPUT);
  digitalWrite(ALARM, LOW);
}

void loop()
{
  dt = clock.getDateTime();

  Serial.println(clock.dateFormat("d-m-Y H:i:s - l", dt));

  if (displayTemp)
  {
    clock.forceConversion();

    Serial.print("Temperature: ");
    int temp = clock.readTemperature();
    Serial.print(temp);
    Serial.println(" deg C");

    uint8_t seg_temp[] = {
      display.encode(temp / 10),
      display.encode(temp % 10),
      TM1637_CHAR_STAR,        
      TM1637_CHAR_C
    };
    display.printRaw(seg_temp);
  } else {
    display.setColonOn(true);
    display.printTime(dt.hour, dt.minute, true);
  }
  displayTemp = !displayTemp;

  // Call isAlarm1(false) if you want clear alarm1 flag manualy by clearAlarm1();
  if (clock.isAlarm1())
  {
    Serial.println("ALARM 1 TRIGGERED!");
    digitalWrite(ALARM, HIGH);
  }

  // Call isAlarm2(false) if you want clear alarm1 flag manualy by clearAlarm2();
  if (clock.isAlarm2())
  {
    Serial.println("ALARM 2 TRIGGERED!");
    digitalWrite(ALARM, HIGH);
  }

  delay(1000);
}
