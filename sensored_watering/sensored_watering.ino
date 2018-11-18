/*
  Sensored Watering
  Checks sensors for soil moisture, operates a stepper for a valve and turns a pump on/off 
*/

#include <CheapStepper.h>

const int sensPin = 6; // pin number of the sensor
const int outPin = 4; // pump switch pin

const int homeSwitchPower = 2; // power for home/endstop switch
const int homeSwitchPin = 3; // pin for checking if home position is reached

const bool CWise = true;

CheapStepper stepper (15,14,16,10);

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(outPin, OUTPUT);
  pinMode(sensPin, INPUT);

  pinMode(homeSwitchPower, OUTPUT);
  digitalWrite(homeSwitchPower, HIGH);

  pinMode(homeSwitchPin, INPUT);

  stepper.setRpm(10);
  home();
}

// the loop function runs over and over again forever
void loop() {
  checkSensor(sensPin, 90);
  delay(100);
}

void home() {
  while(digitalRead(homeSwitchPin) == LOW) {
    stepper.stepCCW();
  }
  stepper.stop();
  while(digitalRead(homeSwitchPin) == HIGH) {
    stepper.stepCW();
  }  
  stepper.stop();
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
  }
}

