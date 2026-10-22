#include <Arduino.h>

const uint8_t STEP_PIN = 16;
const uint8_t DIR_PIN = 17;
const uint8_t EN_PIN = 18;

const uint8_t ELECTROMAGNET_PIN = 39;

const int STEPS_PER_COLUMN = 2611;
const float START_POSITION = 3.5;

float currentPosition = START_POSITION;
bool directionInverted = false;

void moveSteps(int steps, bool direction) {
  if (directionInverted) {
    direction = !direction;
  }
  
  digitalWrite(DIR_PIN, direction);
  delay(10);
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100);
  }
}

void moveToColumn(char col) {
  if (col < 'a' || col > 'h') {
    Serial.println("Invalid column. Use a-h");
    return;
  }
  
  int targetCol = col - 'a';
  float distance = targetCol - currentPosition;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  
  Serial.print("Moving from ");
  Serial.print(currentPosition);
  Serial.print(" to column ");
  Serial.print(col);
  Serial.print(" (");
  Serial.print(targetCol);
  Serial.print("): ");
  Serial.print(steps);
  Serial.print(" steps, direction=");
  Serial.print(direction ? "FORWARD" : "BACKWARD");
  if (directionInverted) {
    Serial.print(" (inverted)");
  }
  Serial.println();
  
  moveSteps(steps, direction);
  currentPosition = targetCol;
  
  Serial.print("Now at column ");
  Serial.println(col);
}

void home() {
  float distance = START_POSITION - currentPosition;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  
  Serial.print("Homing to position ");
  Serial.print(START_POSITION);
  Serial.print(": ");
  Serial.print(steps);
  Serial.println(" steps");
  
  moveSteps(steps, direction);
  currentPosition = START_POSITION;
  
  Serial.println("Homed to 3.5 (between d and e)");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(ELECTROMAGNET_PIN, OUTPUT);
  
  digitalWrite(EN_PIN, LOW);
  digitalWrite(ELECTROMAGNET_PIN, LOW);
  
  Serial.println("=== Chess Column Positioner ===");
  Serial.println("Motor starts between d and e");
  Serial.println("Commands:");
  Serial.println("  a-h = Move to column");
  Serial.println("  p = Print current position");
  Serial.println("  e = Enable/disable motor");
  Serial.println("  i = Invert direction");
  Serial.println("  1 = Electromagnet ON");
  Serial.println("  0 = Electromagnet OFF");
  Serial.println("  t = Toggle electromagnet");
  Serial.println("  r = Reset to home position (moves motor)");
  Serial.println("  h = Home (same as r)");
  Serial.println("==============================");
  Serial.print("Current position: ");
  Serial.println(currentPosition);
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd >= 'a' && cmd <= 'h') {
      moveToColumn(cmd);
    } else {
      switch(cmd) {
        case 'p':
          Serial.print("Current position: ");
          Serial.println(currentPosition);
          break;
        case 'e':
          digitalWrite(EN_PIN, !digitalRead(EN_PIN));
          Serial.print("Motor: ");
          Serial.println(digitalRead(EN_PIN) ? "DISABLED" : "ENABLED");
          break;
        case 'i':
          directionInverted = !directionInverted;
          Serial.print("Direction: ");
          Serial.println(directionInverted ? "INVERTED" : "NORMAL");
          break;
        case '1':
          digitalWrite(ELECTROMAGNET_PIN, HIGH);
          Serial.println("Electromagnet ON");
          break;
        case '0':
          digitalWrite(ELECTROMAGNET_PIN, LOW);
          Serial.println("Electromagnet OFF");
          break;
        case 't':
          digitalWrite(ELECTROMAGNET_PIN, !digitalRead(ELECTROMAGNET_PIN));
          Serial.print("Electromagnet: ");
          Serial.println(digitalRead(ELECTROMAGNET_PIN) ? "ON" : "OFF");
          break;
        case 'r':
        case 'H':
          home();
          break;
      }
    }
  }
}
