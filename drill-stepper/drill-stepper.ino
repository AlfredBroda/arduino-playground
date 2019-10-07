#include <TM1637Display.h>

#define BAUD (115200)
#define DEBUG true

#define STEPPULSE 10 // stepper driver step pulse length
#define STEPDELAY 10 // delay until the next step
#define MMSTEPSZ  200*16 // 200 steps is one full rotation of the stepper
#define MAXZ      300 // 30cm is the maximum z axis displacement
#define MMRATIO   0.2932f // (float) MAXZ/1023
#define STEPRATIO 938     // (int) MMSTEPSZ*MAXZ/1023 
#define DEADZONE  5

#define RESUME A2
// Z axis stepper driver pins
#define ENABLE 8 // shared by all axis
#define STEPZ  7
#define DIRZ   4
#define ENDSTOPZ 11
// #define POSZ   A6 // for UNO
#define POSZ   A11 // for Leonardo, pin D12
#define SPEEDZ  A9 // for Leonardo, pin D9

int step_delay_z;
int step_delay_old;
int step_delay_read;
bool current_dir; // forward or reverse
bool error;

bool run_z;
int destz_Read;
int destz_Old;
long pos_z;
long dest_z;
float dest_z_mm;
float old_dest_z_mm;

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
  display.clear();
  display.setBrightness(0x01);

  if (DEBUG) {
    display.showNumberDec(BAUD/100, false);
    Serial.begin(BAUD);
    delay(100);
    serialPrintLn("Drill press starting...");
  }

  pinMode(RESUME, INPUT_PULLUP);  // Resume/Pause button
  // stepper pins
  pinMode(ENABLE, OUTPUT); // Enable
  pinMode(STEPZ, OUTPUT);  // Step
  pinMode(DIRZ, OUTPUT);   // Dir
  pinMode(POSZ, INPUT);    // Position knob
  pinMode(SPEEDZ, INPUT);  // Speed knob
  // endstops
  pinMode(ENDSTOPZ, INPUT_PULLUP); // Endstop default closed

  step_delay_z = analogRead(SPEEDZ)+STEPDELAY;
  current_dir = false; // set reverse
  disp_timer = 0;

  error = true; // we don't know the position of the machine
  old_dest_z_mm = 0.0f;
  dest_z_mm = 120.0f;
  dest_z = dest_z_mm * STEPRATIO;
  pos_z = 0L;    // should actually be undefined
  run_z = false; // wait for enable

  sanityCheck();
}

void loop()
{
  // get destination
  readPots();
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
  }
}

void sanityCheck() {
  while (digitalRead(ENDSTOPZ) == HIGH) {
    serialPrintLn("Z axis endstop fault!");
    display.setBrightness(0x0f);
    display.clear();
    display.setSegments(SEG_ERRZ);
    delay(1000);
  }
}

int readPots() {
  destz_Read = analogRead(POSZ);
  // Use a deadzone for stable target value
  while (destz_Read < (destz_Old - DEADZONE) || destz_Read > (destz_Old + DEADZONE)) {
    destz_Old = destz_Read;
    dest_z_mm = destz_Read * MMRATIO;
    dest_z = (long)destz_Read * STEPRATIO;
    
    updateCounter();
  }

  step_delay_read = analogRead(SPEEDZ);
  while (step_delay_read < (step_delay_old - DEADZONE) || step_delay_read > (step_delay_old + DEADZONE)) {
    step_delay_old = step_delay_read;
    step_delay_z = (int)(step_delay_read/10)+STEPDELAY;
    serialPrint("Step delay: ");
    serialPrint(step_delay_z);
  }
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
  serialPrint("At set position:");
  serialPrint(dest_z);
  for (int x = 0; x < 4; x++) {
    if (x % 2 == 0) {
      display.setBrightness(0x0f);
    } else {
      display.setBrightness(0x01);
    }
    updateCounter();
    delay(100);
  }
}

void updateCounter() {
  display.showNumberDec(dest_z_mm, false);
  disp_timer += 1;
}

void doStepZ(const bool dir) {
  current_dir = dir;
  digitalWrite(DIRZ, dir); // Set Dir
  digitalWrite(STEPZ, HIGH); // Output high
  delayMicroseconds(STEPPULSE); // Step
  digitalWrite(STEPZ, LOW); // Output low
  delayMicroseconds(step_delay_z); // Wait

  if (dir) {
    pos_z += 1;
  } else {
    pos_z -= 1;
  }
}

void homeZAxis() {
  serialPrintLn("Homing Z axis...");
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

  serialPrintLn("Homing Z axis done.");
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
  updateCounter();
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
    serialPrintLn("Z axis endstop reached!");
    display.setBrightness(0x0f);
    display.clear();
    display.setSegments(SEG_ERRZ);

    // reverse and back off
    reverseDir();
    for (int x = 0; x < MMSTEPSZ; x++) {
      doStepZ(current_dir);
    }
    serialPrintLn("Z axis endstop cleared.");

    // disable steppers, wait for reset
    digitalWrite(ENABLE, HIGH);
    error = true;
    run_z = false;
    serialPrintLn("Stopped Z");

    return true;
  }
  return false;
}

void serialPrint(const String &text) {
  if (DEBUG) {
    Serial.print(text);
  }
}

void serialPrintLn(const String &text) {
  if (DEBUG) {
    Serial.println(text);
  }
}

void serialPrint(const int text) {
  if (DEBUG) {
    Serial.println(text);
  }
}
