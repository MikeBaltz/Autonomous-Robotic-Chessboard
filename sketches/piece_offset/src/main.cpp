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
const float COLUMN_WIDTH_MM = 65.0;
const float STEPS_PER_MM = STEPS_PER_COLUMN / COLUMN_WIDTH_MM;
const int OFFSET_MM = 15;
const int OFFSET_STEPS = OFFSET_MM * STEPS_PER_MM;

const float START_COLUMN = 3.5;
const float START_ROW = 4.5;
float currentColumn = START_COLUMN;
float currentRow = START_ROW;
bool xDirectionInverted = false;
bool yDirectionInverted = true;

void moveXSteps(int steps, bool direction) {
  if (xDirectionInverted) {
    direction = !direction;
  }
  
  digitalWrite(X_DIR_PIN, direction);
  delay(10);
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(X_STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(X_STEP_PIN, LOW);
    delayMicroseconds(100);
  }
}

void moveYSteps(int steps, bool direction) {
  if (yDirectionInverted) {
    direction = !direction;
  }
  
  digitalWrite(Y_DIR_PIN, direction);
  delay(10);
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(Y_STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(Y_STEP_PIN, LOW);
    delayMicroseconds(100);
  }
}

void moveDiagonal(int xSteps, int ySteps) {
  bool xDir = (xSteps > 0);
  bool yDir = (ySteps > 0);
  int absX = abs(xSteps);
  int absY = abs(ySteps);
  
  if (xDirectionInverted) xDir = !xDir;
  if (yDirectionInverted) yDir = !yDir;
  
  digitalWrite(X_DIR_PIN, xDir);
  digitalWrite(Y_DIR_PIN, yDir);
  delay(10);
  
  int maxSteps = max(absX, absY);
  if (maxSteps == 0) return;
  
  long xAccum = 0;
  long yAccum = 0;
  
  for (int i = 0; i < maxSteps; i++) {
    xAccum += absX;
    yAccum += absY;
    
    if (xAccum >= maxSteps) {
      digitalWrite(X_STEP_PIN, HIGH);
      xAccum -= maxSteps;
    }
    if (yAccum >= maxSteps) {
      digitalWrite(Y_STEP_PIN, HIGH);
      yAccum -= maxSteps;
    }
    
    delayMicroseconds(100);
    
    digitalWrite(X_STEP_PIN, LOW);
    digitalWrite(Y_STEP_PIN, LOW);
    
    delayMicroseconds(100);
  }
}

void moveToSquareDiagonal(char col, int row) {
  if (col < 'a' || col > 'h' || row < 1 || row > 8) {
    Serial.println("Invalid square");
    return;
  }
  
  int targetCol = col - 'a';
  float colDistance = targetCol - currentColumn;
  int xSteps = colDistance * STEPS_PER_COLUMN;
  
  float rowDistance = row - currentRow;
  int ySteps = rowDistance * STEPS_PER_ROW;
  
  Serial.print("Moving diagonally to ");
  Serial.print(col);
  Serial.print(row);
  Serial.print(" (X:");
  Serial.print(xSteps);
  Serial.print(" Y:");
  Serial.print(ySteps);
  Serial.println(" steps)");
  
  moveDiagonal(xSteps, ySteps);
  currentColumn = targetCol;
  currentRow = row;
}

void moveToColumn(char col) {
  if (col < 'a' || col > 'h') {
    Serial.println("Invalid column. Use a-h");
    return;
  }
  
  int targetCol = col - 'a';
  float distance = targetCol - currentColumn;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  
  Serial.print("Moving to column ");
  Serial.print(col);
  Serial.print(" (");
  Serial.print(steps);
  Serial.println(" steps)");
  
  moveXSteps(steps, direction);
  currentColumn = targetCol;
}

void moveToRow(int row) {
  if (row < 1 || row > 8) {
    Serial.println("Invalid row. Use 1-8");
    return;
  }
  
  float distance = row - currentRow;
  int steps = abs(distance) * STEPS_PER_ROW;
  bool direction = (distance > 0);
  
  Serial.print("Moving to row ");
  Serial.print(row);
  Serial.print(" (");
  Serial.print(steps);
  Serial.println(" steps)");
  
  moveYSteps(steps, direction);
  currentRow = row;
}

void moveToSquare(char col, int row) {
  moveToColumn(col);
  moveToRow(row);
}

void moveOffset(int steps, bool direction) {
  Serial.print("Moving offset ");
  Serial.print(steps);
  Serial.print(" steps ");
  Serial.println(direction ? "right" : "left");
  
  moveXSteps(steps, direction);
  
  if (direction) {
    currentColumn += (float)steps / STEPS_PER_COLUMN;
  } else {
    currentColumn -= (float)steps / STEPS_PER_COLUMN;
  }
}

void electromagnetOn() {
  digitalWrite(ELECTROMAGNET_PIN, HIGH);
  Serial.println("Electromagnet ON");
  delay(200);
}

void electromagnetOff() {
  digitalWrite(ELECTROMAGNET_PIN, LOW);
  Serial.println("Electromagnet OFF");
  delay(200);
}

void waitForNext() {
  Serial.println("Press 'n' for next step...");
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'n' || c == 'N') {
        break;
      }
    }
    delay(10);
  }
}

void runSequence() {
  Serial.println("\n=== KNIGHT MOVE SEQUENCE ===");
  Serial.println("Knight g8 to f6 (diagonal)");
  Serial.println("Clearing f7 and g7 pawns\n");
  
  float f7OffsetPos = 5.0 - (float)OFFSET_STEPS / STEPS_PER_COLUMN;
  float g7OffsetPos = 6.0 + (float)OFFSET_STEPS / STEPS_PER_COLUMN;
  
  Serial.println("Step 1: Move to f7 pawn");
  waitForNext();
  moveToSquareDiagonal('f', 7);
  delay(500);
  
  Serial.println("\nStep 2: Grab f7 pawn");
  waitForNext();
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 3: Move f7 pawn 15mm left");
  waitForNext();
  moveOffset(OFFSET_STEPS, false);
  delay(500);
  
  Serial.println("\nStep 4: Release f7 pawn");
  waitForNext();
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 5: Move to g7 pawn");
  waitForNext();
  moveToColumn('g');
  delay(500);
  
  Serial.println("\nStep 6: Grab g7 pawn");
  waitForNext();
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 7: Move g7 pawn 15mm right");
  waitForNext();
  moveOffset(OFFSET_STEPS, true);
  delay(500);
  
  Serial.println("\nStep 8: Release g7 pawn");
  waitForNext();
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 9: Move to g8 (knight position)");
  waitForNext();
  moveToSquareDiagonal('g', 8);
  delay(500);
  
  Serial.println("\nStep 10: Grab knight");
  waitForNext();
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 11: Move knight diagonally to f6");
  waitForNext();
  moveToSquareDiagonal('f', 6);
  delay(500);
  
  Serial.println("\nStep 12: Place knight");
  waitForNext();
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 13: Move to f7 offset position");
  waitForNext();
  float distance = f7OffsetPos - currentColumn;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  moveXSteps(steps, direction);
  currentColumn = f7OffsetPos;
  delay(500);
  
  Serial.println("\nStep 14: Grab f7 pawn");
  waitForNext();
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 15: Move f7 pawn back");
  waitForNext();
  moveOffset(OFFSET_STEPS, true);
  delay(500);
  
  Serial.println("\nStep 16: Release f7 pawn");
  waitForNext();
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 17: Move to g7 offset position");
  waitForNext();
  distance = g7OffsetPos - currentColumn;
  steps = abs(distance) * STEPS_PER_COLUMN;
  direction = (distance > 0);
  moveXSteps(steps, direction);
  currentColumn = g7OffsetPos;
  delay(500);
  
  Serial.println("\nStep 18: Grab g7 pawn");
  waitForNext();
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 19: Move g7 pawn back");
  waitForNext();
  moveOffset(OFFSET_STEPS, false);
  delay(500);
  
  Serial.println("\nStep 20: Release g7 pawn");
  waitForNext();
  electromagnetOff();
  delay(500);
  
  Serial.println("\n=== SEQUENCE COMPLETE ===");
  Serial.print("Final position: col=");
  Serial.print(currentColumn);
  Serial.print(" row=");
  Serial.println(currentRow);
}

void runSequenceAuto() {
  Serial.println("\n=== KNIGHT MOVE SEQUENCE (AUTO) ===");
  Serial.println("Knight g8 to f6 (diagonal)");
  Serial.println("Clearing f7 and g7 pawns\n");
  
  float f7OffsetPos = 5.0 - (float)OFFSET_STEPS / STEPS_PER_COLUMN;
  float g7OffsetPos = 6.0 + (float)OFFSET_STEPS / STEPS_PER_COLUMN;
  
  Serial.println("Step 1: Move to f7 pawn");
  moveToSquareDiagonal('f', 7);
  delay(500);
  
  Serial.println("\nStep 2: Grab f7 pawn");
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 3: Move f7 pawn 15mm left");
  moveOffset(OFFSET_STEPS, false);
  delay(500);
  
  Serial.println("\nStep 4: Release f7 pawn");
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 5: Move to g7 pawn");
  moveToColumn('g');
  delay(500);
  
  Serial.println("\nStep 6: Grab g7 pawn");
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 7: Move g7 pawn 15mm right");
  moveOffset(OFFSET_STEPS, true);
  delay(500);
  
  Serial.println("\nStep 8: Release g7 pawn");
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 9: Move to g8 (knight position)");
  moveToSquareDiagonal('g', 8);
  delay(500);
  
  Serial.println("\nStep 10: Grab knight");
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 11: Move knight diagonally to f6");
  moveToSquareDiagonal('f', 6);
  delay(500);
  
  Serial.println("\nStep 12: Place knight");
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 13: Move to f7 offset position");
  float distance = f7OffsetPos - currentColumn;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  moveXSteps(steps, direction);
  currentColumn = f7OffsetPos;
  delay(500);
  
  Serial.println("\nStep 14: Grab f7 pawn");
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 15: Move f7 pawn back");
  moveOffset(OFFSET_STEPS, true);
  delay(500);
  
  Serial.println("\nStep 16: Release f7 pawn");
  electromagnetOff();
  delay(500);
  
  Serial.println("\nStep 17: Move to g7 offset position");
  distance = g7OffsetPos - currentColumn;
  steps = abs(distance) * STEPS_PER_COLUMN;
  direction = (distance > 0);
  moveXSteps(steps, direction);
  currentColumn = g7OffsetPos;
  delay(500);
  
  Serial.println("\nStep 18: Grab g7 pawn");
  electromagnetOn();
  delay(500);
  
  Serial.println("\nStep 19: Move g7 pawn back");
  moveOffset(OFFSET_STEPS, false);
  delay(500);
  
  Serial.println("\nStep 20: Release g7 pawn");
  electromagnetOff();
  delay(500);
  
  Serial.println("\n=== SEQUENCE COMPLETE ===");
  Serial.print("Final position: col=");
  Serial.print(currentColumn);
  Serial.print(" row=");
  Serial.println(currentRow);
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
  
  Serial.println("=== Piece Offset Test (2-Axis) ===");
  Serial.println("Simulates moving pawns aside for knight move");
  Serial.println("\nCommands:");
  Serial.println("  s = Run sequence (step-by-step with 'n')");
  Serial.println("  a = Run sequence (auto, no pauses)");
  Serial.println("  m a = Move to column a");
  Serial.println("  m 7 = Move to row 7");
  Serial.println("  m a7 = Move to square a7");
  Serial.println("  1 = Electromagnet ON");
  Serial.println("  0 = Electromagnet OFF");
  Serial.println("  i = Invert X direction");
  Serial.println("  r = Home to center");
  Serial.println("  p = Print position");
  Serial.println("=================================");
  Serial.print("Current position: col=");
  Serial.print(currentColumn);
  Serial.print(" row=");
  Serial.println(currentRow);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input == "s") {
      runSequence();
    }
    else if (input == "a") {
      runSequenceAuto();
    }
    else if (input.length() >= 3 && input[0] == 'm' && input[1] == ' ') {
      if (input.length() == 3) {
        char c = input[2];
        if (c >= 'a' && c <= 'h') {
          moveToColumn(c);
        } else if (c >= '1' && c <= '8') {
          moveToRow(c - '0');
        } else {
          Serial.println("Invalid. Use: m a, m 7, or m a7");
        }
      } else if (input.length() == 4) {
        char col = input[2];
        int row = input[3] - '0';
        if (col >= 'a' && col <= 'h' && row >= 1 && row <= 8) {
          moveToSquare(col, row);
        } else {
          Serial.println("Invalid square. Use: m a7");
        }
      }
    }
    else if (input == "1") {
      electromagnetOn();
    }
    else if (input == "0") {
      electromagnetOff();
    }
    else if (input == "i") {
      xDirectionInverted = !xDirectionInverted;
      Serial.print("X Direction: ");
      Serial.println(xDirectionInverted ? "INVERTED" : "NORMAL");
    }
    else if (input == "r") {
      float colDistance = START_COLUMN - currentColumn;
      int colSteps = abs(colDistance) * STEPS_PER_COLUMN;
      bool colDir = (colDistance > 0);
      moveXSteps(colSteps, colDir);
      currentColumn = START_COLUMN;
      
      float rowDistance = START_ROW - currentRow;
      int rowSteps = abs(rowDistance) * STEPS_PER_ROW;
      bool rowDir = (rowDistance > 0);
      moveYSteps(rowSteps, rowDir);
      currentRow = START_ROW;
      
      Serial.println("Homed to center");
    }
    else if (input == "p") {
      Serial.print("Current position: col=");
      Serial.print(currentColumn);
      Serial.print(" row=");
      Serial.println(currentRow);
    }
  }
}
