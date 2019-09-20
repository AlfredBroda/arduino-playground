#include <TM1637Display.h>

#define BAUD (9600)
#define DEBUG false

#define DEFDELAY 100
#define MMSTEPSZ 320 // 320 steps is 1mm by default on TMC chips
#define MAXZ     300 // 30cm is the maximum z axis displacement
#define MMRATIO   0.2932 // (float) MAXZ/1023
#define STEPRATIO 94     // (int) MMSTEPSZ*MAXZ/1023 

#define RESUME A2
// Z axis stepper driver pins
#define ENABLE 8 // shared by all axis
#define STEPZ  7
#define DIRZ   4
#define ENDSTOPZ 11
#define POSZ   A6

int step_delay_z;
bool current_dir; // forward or reverse
bool error;

bool run_z;
int destz_Read;
long pos_z;
long dest_z;
float dest_z_mm;

int disp_timer;

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
const uint8_t SEG_HOME[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,           // H
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F,           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};
const uint8_t SEG_STOP[] = {
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,           // S
  SEG_A | SEG_B | SEG_C,                           // T
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G            // P
};
const uint8_t SEG_START[] = {
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // R
  SEG_C | SEG_D | SEG_E,                           // u
  SEG_C | SEG_E | SEG_G,                           // n
  0x00                                             // _
};

void setup()
{
  if (DEBUG) {
    Serial.begin(BAUD);
  }

  display.clear();
  display.setBrightness(0x01);

  pinMode(RESUME, INPUT_PULLUP);  // Resume/Pause button
  // stepper pins
  pinMode(ENABLE, OUTPUT); // Enable
  pinMode(STEPZ, OUTPUT);  // Step
  pinMode(DIRZ, OUTPUT);   // Dir
  pinMode(POSZ, INPUT);    // Position knob
  // endstops
  pinMode(ENDSTOPZ, INPUT_PULLUP); // Endstop default closed

  step_delay_z = DEFDELAY;
  current_dir = false; // set reverse
  disp_timer = 0;

  error = true; // we don't know the position of the machine
  dest_z_mm = 0;
  dest_z = 0L;
  pos_z = 0L;    // should actually be undefined
  run_z = false; // wait for enable

  sanityCheck();
}

void loop()
{
  // get destination
  readPosSet();
  checkEnabled();

  if (run_z && error) {
    homeZAxis();
  } else if (run_z && !checkEndstopZ()) {
    // check where we are
    if (pos_z == dest_z) {
      doStay();
    } else if (dest_z > pos_z) {
      doStepZ(true);
    } else if (dest_z < pos_z) {
      doStepZ(false);
    }
    updateCounter();
  }
}

void sanityCheck() {
  while (digitalRead(ENDSTOPZ) == HIGH) {
    serialPrint("Z axis endstop fault!");
    display.setBrightness(0x0f);
    display.clear();
    display.setSegments(SEG_ERRZ);
    delay(1000);
  }
}

int readPosSet() {
  destz_Read = analogRead(POSZ);
  dest_z_mm = destz_Read * MMRATIO;
  dest_z = (long)destz_Read * STEPRATIO;
}

void checkEnabled() {
  if (digitalRead(RESUME) == LOW) {
    run_z = true;
    display.setSegments(SEG_START);
    delay(1000);
  }
  if (!run_z) {
    display.setSegments(SEG_STOP);
    delay(1000);
  }
}

void doStay() {
  if (disp_timer == 0) {
    serialPrint("At set position:");
    serialPrint(dest_z);
    for (int x = 0; x < 10; x++) {
      if (x % 2 == 0) {
        display.setBrightness(0x0f);
      } else {
        display.setBrightness(0x01);
      }
      display.showNumberDec(dest_z_mm, false);
      delay(100);
    }
  }
}

void updateCounter() {
  if (disp_timer % MMSTEPSZ == 0) {
    if (disp_timer < 10 * MMSTEPSZ) {
      display.setBrightness(0x0f);
      display.showNumberDec(round(pos_z / MMSTEPSZ), false);
    } else {
      display.setBrightness(0x02);
      display.showNumberDec(dest_z_mm, false);
    }
    serialPrint("Position/destination:");
    serialPrint(destz_Read);
    serialPrint(pos_z);
    serialPrint(dest_z);
  }
  disp_timer += 1;
  if (disp_timer > 15 * MMSTEPSZ) {
    disp_timer = 0;
  }
}

void doStepZ(const bool dir) {
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
  display.clear();
  display.setSegments(SEG_HOME);
  current_dir = false;

  digitalWrite(ENABLE, LOW); // enable steppers
  while (digitalRead(ENDSTOPZ) == LOW) {
    // go home
    doStepZ(current_dir);
  }
  // back off
  reverseDir();
  for (int x = 0; x < MMSTEPSZ; x++) {
    doStepZ(current_dir);
  }

  serialPrint("Homing Z axis done.");
  for (int x = 0; x < 6; x++) {
    if (x % 2 == 0) {
      display.setBrightness(0x0f);
    } else {
      display.setBrightness(0x01);
    }
    display.clear();
    display.setSegments(SEG_DONE);
    delay(500);
  }
  display.setBrightness(0x01);

  pos_z = 0;
  error = false;
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
    display.clear();
    display.setSegments(SEG_ERRZ);

    // reverse and back off
    reverseDir();
    for (int x = 0; x < MMSTEPSZ; x++) {
      doStepZ(current_dir);
    }
    serialPrint("Z axis endstop cleared.");

    // disable steppers, wait for reset
    digitalWrite(ENABLE, HIGH);
    error = true;
    run_z = false;
    serialPrint("Stopped Z");

    return true;
  }
  return false;
}

void serialPrint(const String &text) {
  if (DEBUG) {
    Serial.print(millis());
    Serial.print(": ");
    Serial.println(text);
  }
}

void serialPrint(const int text) {
  if (DEBUG) {
    Serial.print(millis());
    Serial.print(": ");
    Serial.println(text);
  }
}
