#include <Arduino.h>

const uint8_t X_STEP_PIN = 8;
const uint8_t X_DIR_PIN = 9;
const uint8_t X_EN_PIN = 10;

const uint8_t Y_STEP_PIN = 16;
const uint8_t Y_DIR_PIN = 17;
const uint8_t Y_EN_PIN = 18;

const uint8_t ELECTROMAGNET_PIN = 39;

const int STEPS_PER_COLUMN = 2611;
const int STEPS_PER_ROW = 2611;

float currentColumn = 3.5;
float currentRow = 4.5;

bool xDirectionInverted = false;
bool yDirectionInverted = true;

void stepMotor(uint8_t stepPin, uint8_t dirPin, int steps, bool dir) {
  digitalWrite(dirPin, dir);
  delay(10);
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(100);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(100);
  }
}

void moveXSteps(int steps, bool dir) {
  if (xDirectionInverted) dir = !dir;
  stepMotor(X_STEP_PIN, X_DIR_PIN, steps, dir);
}

void moveYSteps(int steps, bool dir) {
  if (yDirectionInverted) dir = !dir;
  stepMotor(Y_STEP_PIN, Y_DIR_PIN, steps, dir);
}

void moveToColumn(char col) {
  int targetCol = col - 'a';
  float distance = targetCol - currentColumn;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool dir = (distance > 0);
  moveXSteps(steps, dir);
  currentColumn = targetCol;
  Serial.print("Column ");
  Serial.println(col);
}

void moveToRow(int row) {
  float distance = row - currentRow;
  int steps = abs(distance) * STEPS_PER_ROW;
  bool dir = (distance > 0);
  moveYSteps(steps, dir);
  currentRow = row;
  Serial.print("Row ");
  Serial.println(row);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(X_STEP_PIN, OUTPUT);
  pinMode(X_DIR_PIN, OUTPUT);
  pinMode(X_EN_PIN, OUTPUT);
  
  pinMode(Y_STEP_PIN, OUTPUT);
  pinMode(Y_DIR_PIN, OUTPUT);
  pinMode(Y_EN_PIN, OUTPUT);
  
  pinMode(ELECTROMAGNET_PIN, OUTPUT);
  
  digitalWrite(X_EN_PIN, LOW);
  digitalWrite(Y_EN_PIN, LOW);
  digitalWrite(ELECTROMAGNET_PIN, LOW);
  
  Serial.println("=== Dual Motor Test ===");
  Serial.println("  a-h     - Move to column");
  Serial.println("  1-8     - Move to row");
  Serial.println("  f/b     - X +/- 1 step");
  Serial.println("  F/B     - X +/- 10 steps");
  Serial.println("  y/z     - Y +/- 1 step");
  Serial.println("  Y/Z     - Y +/- 10 steps");
  Serial.println("  +       - Electromagnet ON");
  Serial.println("  -       - Electromagnet OFF");
  Serial.println("  e       - Toggle motors");
  Serial.println("  s       - Status");
  Serial.println("=======================");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd >= 'a' && cmd <= 'h') {
      moveToColumn(cmd);
    }
    else if (cmd >= '1' && cmd <= '8') {
      moveToRow(cmd - '0');
    }
    else {
      switch(cmd) {
        case 'f':
          moveXSteps(1, true);
          Serial.println("X +1");
          break;
        case 'b':
          moveXSteps(1, false);
          Serial.println("X -1");
          break;
        case 'F':
          moveXSteps(10, true);
          Serial.println("X +10");
          break;
        case 'B':
          moveXSteps(10, false);
          Serial.println("X -10");
          break;
        case 'y':
          moveYSteps(1, true);
          Serial.println("Y +1");
          break;
        case 'z':
          moveYSteps(1, false);
          Serial.println("Y -1");
          break;
        case 'Y':
          moveYSteps(10, true);
          Serial.println("Y +10");
          break;
        case 'Z':
          moveYSteps(10, false);
          Serial.println("Y -10");
          break;
        case '+':
          digitalWrite(ELECTROMAGNET_PIN, HIGH);
          Serial.println("Electromagnet: ON");
          break;
        case '-':
          digitalWrite(ELECTROMAGNET_PIN, LOW);
          Serial.println("Electromagnet: OFF");
          break;
        case 'e': {
          bool xEn = digitalRead(X_EN_PIN);
          digitalWrite(X_EN_PIN, !xEn);
          digitalWrite(Y_EN_PIN, !xEn);
          Serial.print("Motors: ");
          Serial.println(xEn ? "ENABLED" : "DISABLED");
          break;
        }
        case 's':
          Serial.print("Position: col=");
          Serial.print(currentColumn);
          Serial.print(" row=");
          Serial.println(currentRow);
          Serial.print("X: EN=");
          Serial.print(digitalRead(X_EN_PIN) ? "OFF" : "ON");
          Serial.println(xDirectionInverted ? " (inv)" : "");
          Serial.print("Y: EN=");
          Serial.print(digitalRead(Y_EN_PIN) ? "OFF" : "ON");
          Serial.println(yDirectionInverted ? " (inv)" : "");
          Serial.print("Electromagnet: ");
          Serial.println(digitalRead(ELECTROMAGNET_PIN) ? "ON" : "OFF");
          break;
      }
    }
  }
}
