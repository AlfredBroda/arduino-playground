#include <TM1637Display.h>

#define BAUD (9600)
#define DEFDELAY 100
#define ENABLE 8     // all axis enable pin
#define MMSTEPSZ 320 // 320 steps is 1mm by default on TMC chips

// Z axis stepper driver pins
#define STEPZ 7
#define DIRZ 4
#define ENDSTOPZ 11
#define POSZ A6

int x; 
int step_delay_z;
bool current_dir; // forward or reverse
bool error;

bool run_z;
int pos_z;
int dest_z;

TM1637Display display(A5, A4);

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
  };
const uint8_t SEG_ERRZ[] = {
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_E | SEG_G,                                   // r
  SEG_E | SEG_G,                                   // r
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G            // Z
  };
const uint8_t SEG_HOMZ[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,           // H
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  0x00,                                            // _
  SEG_A | SEG_B | SEG_D | SEG_E | SEG_G            // Z
  };

void setup() 
{
  Serial.begin(BAUD);

  display.clear();
  display.setBrightness(0x01);

  pinMode(ENABLE, OUTPUT); // Enable
  pinMode(STEPZ, OUTPUT); // Step
  pinMode(DIRZ, OUTPUT); // Dir
  pinMode(ENDSTOPZ, INPUT_PULLUP); // Endstop default closed

  error = 0;
  pos_z = 0;
  step_delay_z = DEFDELAY;
  current_dir = true;
  x = 0;

  homeZAxis();
}

void loop() 
{
  if (!error) {
    // Loop
    while(run_z)
    {
      if (checkEndstopZ()) {
        run_z = false;
        serialPrint("Stopped Z");
        exit;
      }

      // get destination
      readPosSetZ();
      // check where we are
      if (pos_z < dest_z) {
        doStepZ(true);
      } else if (pos_z > dest_z) {
        doStepZ(false);
      }
      updateCounter();
    }
  }
}

void updateCounter() {
    display.showNumberDec((pos_z*10)/(MMSTEPSZ), false);
}
void doStepZ(const bool dir) 
{
    current_dir = dir;
    digitalWrite(DIRZ, dir); // Set Dir
    digitalWrite(STEPZ, HIGH); // Output high
    delayMicroseconds(100); // Wait
    digitalWrite(STEPZ, LOW); // Output low
    delayMicroseconds(step_delay_z); // Wait

    if (dir) {
      pos_z += 1;
    } else {
      pos_z -= 1;
    }
}

void homeZAxis() {
  serialPrint("Homing Z axis...");
  display.setSegments(SEG_HOMZ);
  current_dir = false;

  digitalWrite(ENABLE, LOW); // enable steppers
  while (digitalRead(ENDSTOPZ) == LOW) {
    // go home
    doStepZ(current_dir);
  }
  // back off
  reverseDir();
  while (digitalRead(ENDSTOPZ) == HIGH) {
    doStepZ(current_dir);
  }
  display.setBrightness(0x0f);
  serialPrint("Homing Z axis done.");
  display.setSegments(SEG_DONE);
  delay(2000);
  display.setBrightness(0x01);
}

void reverseDir() {
  if (current_dir) {
    current_dir = false;
  } else {
    current_dir = true;
  }
}

bool checkEndstopZ() {
  if (digitalRead(ENDSTOPZ) == HIGH) {
      // stop at once if endstop reached, back off until cleared
      serialPrint("Z axis endstop reached!");
      display.setBrightness(0x0f);
      display.setSegments(SEG_ERRZ); 
      reverseDir();
      while (digitalRead(ENDSTOPZ) == HIGH) {
        doStepZ(current_dir);
      }
      serialPrint("Z axis endstop cleared.");
      // disable steppers, wait for reset
      digitalWrite(ENABLE, HIGH);
      error = 1;
      return true;
  }
  return 0;
}

int readPosSetZ() {
  int dest_mm = analogRead(A6);
  dest_z = dest_mm * MMSTEPSZ;
}

void serialPrint(const String &text) {
    Serial.print(millis());
    Serial.print(": ");
    Serial.println(text);
}
