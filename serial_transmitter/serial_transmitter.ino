/**
 * SerialReflector
 */
// #include <SoftwareSerial.h>

// SoftwareSerial device(2,3); // Rx, Tx

int LED = 17;

void setup() {
  Serial.begin(57600);   // Serial port for connection to host
  while(!Serial) {
    delay(10);
  }
  Serial.println("Serial on-line!");

  //  device.begin(9600);  // Serial port for connection to serial device
  Serial1.begin(57600);
  while(!Serial1) {
    delay(10);
  }
  Serial.println("Serial1 on-line!");
}

void loop() {
  if(Serial1.available()) {
    Serial.write(Serial1.read());
  }

  if(Serial.available()) {
    char mesg = Serial.read();
//    Serial.write(mesg);
    Serial1.write(mesg);
  }
}


