#include <TM1637Display.h>

#define BAUD (9600)
#define DEBUG true

#define DEFDELAY (100)
#define MMSTEPSZ (320) // 320 steps is 1mm by default on TMC chips
#define MAXZ (300)     // 30cm is the maximum z axis displacement

// Z axis stepper driver pins
#define ENABLE 8 // shared by all axis
#define STEPZ 7
#define DIRZ 4
#define ENDSTOPZ 11
#define POSZ A6

int step_delay_z;
bool current_dir; // forward or reverse
bool error;

bool run_z;
long pos_z;
long dest_z;
int dest_z_mm;
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
  pinMode(STEPZ, OUTPUT);  // Step
  pinMode(DIRZ, OUTPUT);   // Dir
  pinMode(POSZ, INPUT);    // Position knob
  pinMode(ENDSTOPZ, INPUT_PULLUP); // Endstop default closed

  step_delay_z = DEFDELAY;
  current_dir = false; // set reverse 
  disp_timer = 0;

  error = true; // we don't know the position of the machine
  pos_z = 0L;    // should actually be undefined
  dest_z = 0L;
  run_z = true; // enable z axis
}

void loop() 
{
  if (!error) {
    serialPrint("Start running main loop");
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
      if (pos_z == dest_z) {
        serialPrint("At set position:");
        serialPrint(dest_z);
        for (int x = 0; x < 6; x++) {
          if (x%2 == 0) {
            display.setBrightness(0x0f);
          } else {
            display.setBrightness(0x01);
          }
          display.showNumberDec(dest_z_mm, false);
          delay(500);
        }
      } else if (dest_z > pos_z) {
        doStepZ(true);
      } else if (dest_z < pos_z) {
        doStepZ(false);
      }
      updateCounter();
    }
    if (!run_z) {
      readPosSetZ();
      updateCounter();
      delay(100);
    }
  } else {
      homeZAxis();
      run_z = true;
  }
}

int readPosSetZ() {
  int posz_read = analogRead(POSZ);
  dest_z_mm = round(posz_read*MAXZ/1024);
  dest_z = round(dest_z_mm*MMSTEPSZ);
}

void updateCounter() {
  if (disp_timer%MMSTEPSZ == 0) {
    if (disp_timer < 10*MMSTEPSZ) {
      display.setBrightness(0x0f);
      display.showNumberDec(dest_z_mm, false);
    } else {  
      display.setBrightness(0x01);
      display.showNumberDec(round(pos_z/MMSTEPSZ), false);
    }
    serialPrint("Position/destination");
    serialPrint(pos_z);
    serialPrint(dest_z);
  }
  disp_timer += 1;
  if (disp_timer > 20*MMSTEPSZ) {
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
  display.setSegments(SEG_HOMZ);
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
    if (x%2 == 0) {
      display.setBrightness(0x0f);
    } else {
      display.setBrightness(0x01);
    }
    display.setSegments(SEG_DONE);
    delay(500);
  }
  display.setBrightness(0x01);
  
  display.clear();

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
      return true;
  }
  return false;
}

void serialPrint(const String &text) {
    if (DEBUG){
      Serial.print(millis());
      Serial.print(": ");
      Serial.println(text);
    }
}

void serialPrint(const int text) {
    if (DEBUG){
      Serial.print(millis());
      Serial.print(": ");
      Serial.println(text);
    }
}
