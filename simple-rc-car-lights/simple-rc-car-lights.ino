#include <PinChangeInterrupt.h>

/*
 * Define pins used to provide RC PWM signal to Arduino
 * Pins 8, 9 and 10 are used since they work on both ATMega328 and 
 * ATMega32u4 board. So this code will work on Uno/Mini/Nano/Micro/Leonardo
 * See PinChangeInterrupt documentation for usable pins on other boards
 */
const byte channel_pin[] = {2, 8};
volatile unsigned long rising_start[] = {0, 0};
volatile long channel_length[] = {0, 0};

const int leftSignal = 7;
const int rightSignal = 6;
const int reverseLight = 5;
const int stopLight = 4;

void setup() {
  Serial.begin(57600);

  pinMode(channel_pin[0], INPUT);
  pinMode(channel_pin[1], INPUT);

  pinMode(leftSignal, OUTPUT);
  digitalWrite(leftSignal,LOW);
  pinMode(rightSignal, OUTPUT);
  digitalWrite(rightSignal,LOW);
  pinMode(reverseLight, OUTPUT);
  digitalWrite(reverseLight,LOW);
  pinMode(stopLight, OUTPUT);
  digitalWrite(stopLight,LOW);

  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(channel_pin[0]), onRising0, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(channel_pin[1]), onRising1, CHANGE);
}

void processPin(byte pin) {
  uint8_t trigger = getPinChangeInterruptTrigger(digitalPinToPCINT(channel_pin[pin]));

  if(trigger == RISING) {
    rising_start[pin] = micros();
  } else if(trigger == FALLING) {
    channel_length[pin] = micros() - rising_start[pin];
  }
}

void onRising0(void) {
  processPin(0);
}

void onRising1(void) {
  processPin(1);
}


void printValues() {
  Serial.print(channel_length[0]);
  Serial.print(" | ");
  Serial.println(channel_length[1]);
}

void loop(){
  processTurnSignal(channel_length[0]);
  processThrottle(channel_length[1]);
  delay(100);
}

void processTurnSignal(int turnSignal) {
  if (turnSignal > 1600) {
    digitalWrite(rightSignal,HIGH);
    digitalWrite(leftSignal,LOW);
  } else if (turnSignal < 1400){
    digitalWrite(leftSignal,HIGH);
    digitalWrite(rightSignal,LOW);
  } else {
    digitalWrite(rightSignal,LOW);
    digitalWrite(leftSignal,LOW);
  }
}

void processThrottle(int throttle) {
  if ((throttle > 1450) && (throttle < 1550)) {
    digitalWrite(stopLight,HIGH);
    digitalWrite(reverseLight,LOW);
  } else if (throttle < 1450){
    digitalWrite(reverseLight,HIGH);
    digitalWrite(stopLight,LOW);
  } else {
    digitalWrite(reverseLight,LOW);
    digitalWrite(stopLight,LOW);
  }
}
