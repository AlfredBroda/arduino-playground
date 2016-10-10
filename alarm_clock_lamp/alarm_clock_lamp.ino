/*
  DS3231: Real-Time Clock (with alarm)
  Based on: www.jarzebski.pl/arduino/komponenty/zegar-czasu-rzeczywistego-rtc-DS3231.html
  (c) 2016 by Alfred Broda
*/

#include <Wire.h>
#include <DS3231.h>
#include <TM1637Display.h>

DS3231 clock;
RTCDateTime dt;

// Module connection pins (Digital Pins)
#define CLK 5
#define DIO 4
TM1637Display clockDisplay(CLK, DIO);
bool clockDot = false;

#define LAMP 6

void setup()
{
  Serial.begin(9600);
  
  // Initialize DS3231
  Serial.println("Initialize DS3231");;
  clock.begin();

  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();
 
  // Set Alarm - Every 05h:30m:00s in each day
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  clock.setAlarm1(0, 5, 30, 00, DS3231_MATCH_H_M_S);

  // Initialize display
  clockDisplay.setBrightness(0x08);
  
  pinMode(LAMP, OUTPUT);
  digitalWrite(LAMP, LOW);
}

void loop()
{
  dt = clock.getDateTime();

  Serial.println(clock.dateFormat("d-m-Y H:i:s - l", dt));
  
  clockDisplay.showNumberDec((dt.hour * 100) + dt.minute);
  if(clockDot) 
  {
//    clockDisplay.point(1);
    clock.forceConversion();

    Serial.print("Temperature: ");
    int temp = clock.readTemperature();
    Serial.println(temp);
    clockDisplay.showNumberDec(temp);
  } else {
//    clockDisplay.point(0);
  }
  clockDot = !clockDot;

  // Call isAlarm1(false) if you want clear alarm1 flag manualy by clearAlarm1();
  if (clock.isAlarm1())
  {
    Serial.println("ALARM 1 TRIGGERED!");
    digitalWrite(LAMP, HIGH);
  }

  // Call isAlarm2(false) if you want clear alarm1 flag manualy by clearAlarm2();
  if (clock.isAlarm2())
  {
    Serial.println("ALARM 2 TRIGGERED!");
    digitalWrite(LAMP, HIGH);
  }
 
  delay(1000);
}

