/*
  Sensored Watering
  Checks sensors for soil moisture, operates a stepper for a valve and turns a pump on/off 
*/

#include <CheapStepper.h>

const int sensPin = 6; // pin number of the sensor
const int outPin = 4; // pump switch pin

const int homePin = 3; // pin for checking if home position is reached

const bool CWise = false;

CheapStepper stepper(15,14,16,10);

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(outPin, OUTPUT);
  pinMode(sensPin, INPUT);
  pinMode(homePin, INPUT);

  stepper.setRpm(15);
  home();
}

// the loop function runs over and over again forever
void loop() {
  checkSensor(sensPin, 30);
//  checkSensor(sensPin, 90);
//  checkSensor(sensPin, 150);
//  checkSensor(sensPin, 210);
//  checkSensor(sensPin, 270);
  delay(1000);
}

void home() {
  while(digitalRead(homePin) == LOW) {
    stepper.stepCCW();
  }
  stepper.stop();
  
  while(digitalRead(homePin) == HIGH) {
    stepper.stepCW();
  }  
  stepper.stop();
  
}

void powerDown() {
  digitalWrite(15, LOW);
  digitalWrite(14, LOW);
  digitalWrite(16, LOW);
  digitalWrite(10, LOW);
}

void checkSensor(int pin, int angle) {
  if (digitalRead(pin) == HIGH) {
    stepper.moveDegrees(CWise, angle); // rotate the valve
    digitalWrite(outPin, HIGH);   // turn the pump output on
    while(digitalRead(pin) == HIGH) {  // keep checking the sensor
      delay(1000);
    }
    digitalWrite(outPin, LOW);    // turn the pump output off
    home(); // reset valve
    powerDown();
  }
}

