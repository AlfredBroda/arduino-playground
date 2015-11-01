#include <TimerOne.h>

volatile unsigned long blinkCount = 0; // use volatile for shared variables
volatile unsigned long revs = 0;

unsigned int rpm = 0;

int ledPin = 17;

// The interrupt will blink the LED, and keep
// track of how many times it has blinked.
int ledState = LOW;

void setup()
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);

  attachInterrupt(2, countRPM, RISING);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(blinkLED); // blinkLED to run every 1.0 seconds
}

void loop()
{
  unsigned long blinkCopy;  // holds a copy of the blinkCount
  unsigned long revsRead;  // holds a copy of revs

  // to read a variable which the interrupt code writes, we
  // must temporarily disable interrupts, to be sure it will
  // not change while we are reading.  To minimize the time
  // with interrupts off, just quickly make a copy, and then
  // use the copy while allowing the interrupt to keep working.

  noInterrupts();
  blinkCopy = blinkCount;
  revsRead = revs;
  revs = 0;
  interrupts();

  // as we count revolutions per second, the math is simple
  rpm = 60*1000*revsRead;

  Serial.print("second: ");
  Serial.print(blinkCopy);
  Serial.print(" revs: ");
  Serial.print(revsRead);
  Serial.print(" rpm: ");
  Serial.println(rpm, DEC);
  delay(1000);
}

void blinkLED(void)
{
  if (ledState == LOW) {
    ledState = HIGH;
    blinkCount++;  // increase when LED turns on
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}

void countRPM()
{
  revs++;
}

