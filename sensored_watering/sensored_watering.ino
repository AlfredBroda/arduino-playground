/*
  TimedWatering
  Turns a pump on for one minute per hour, repeatedly.

  http://www.arduino.cc/en/Tutorial/Blink
*/

// this constant won't change. It's the pin number of the sensor's output:
const int outPin = 4;
const int sensPin = 6;
int state = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(outPin, OUTPUT);
  pinMode(sensPin, INPUT);
}

// the loop function runs over and over again forever
void loop() {
  state = digitalRead(sensPin);
  if (state == HIGH) {
    digitalWrite(outPin, HIGH);   // turn the output on (HIGH is the voltage level)
    delay(5000);
    digitalWrite(outPin, LOW);    // turn the output off by making the voltage LOW
  }
  delay(1000);

  //digitalWrite(outPin, HIGH);   // turn the output on (HIGH is the voltage level)
  //delay(60000);                 // turn on for 1 minutes
  //digitalWrite(outPin, LOW);    // turn the output off by making the voltage LOW
  //delay(3540000);               // wait for 59 minutes
}
