/*
  Based on ReadAnalogVoltage
 Reads an analog input on tempPin, converts it to teperature in Celcius, and prints the result to the serial monitor.
 Attach the center pin of the LM35 tempPin, and the outside pins to +5V and ground.

 This code is in the public domain.
 */

int tempPin = A3;

float temp;
float oldTemp;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  oldTemp = readSensor(tempPin);
  Serial.println(oldTemp);
}

// the loop routine runs over and over again forever:
void loop() {
  temp = readSensor(tempPin);
  if (temp != oldTemp) {
    Serial.println(temp);
    oldTemp = temp;
  }

  delay(100);
}

float readSensor(int pin) {
  // read the input on analog pin 0:
  int sensorValue = 0;
  int loops = 5;
  while (loops > 0) {
    sensorValue += analogRead(pin);
    loops--;
    delay(100);
  }
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  return (sensorValue * (5.0 / 1023.0) / 0.05) -2;
}
