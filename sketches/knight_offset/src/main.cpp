#include <Arduino.h>

const uint8_t X_STEP_PIN = 8;
const uint8_t X_DIR_PIN = 9;
const uint8_t X_EN_PIN = 10;

const uint8_t Y_STEP_PIN = 16;
const uint8_t Y_DIR_PIN = 17;
const uint8_t Y_EN_PIN = 18;

const uint8_t ELECTROMAGNET_PIN = 39;

String serialInput;

const int STEPS_PER_COLUMN = 2220;
const int STEPS_PER_ROW = 2220;
const uint32_t MOVE_SPEED = 1000;

const float SQUARE_PITCH = 45.1;
const float HALL_Y_OFFSET = 15.0;
const float SURROUND_OFFSET = 0.25;
const float CLEARANCE_OFFSET = 0.25;

const float START_COLUMN = 4.5;
const float START_ROW = 4.5;
float currentColumn = START_COLUMN;
float currentRow = START_ROW;

bool xDirectionInverted = false;
bool yDirectionInverted = true;
bool sensorMode = false;

void moveXSteps(int steps, bool direction) {
  const bool positive = direction;
  if (xDirectionInverted) {
    direction = !direction;
  }
  
  digitalWrite(X_DIR_PIN, direction);
  delay(10);
  
  uint32_t minDelay = 1000000 / (MOVE_SPEED * 2);
  uint32_t startDelay = minDelay * 4;
  
  int accelSteps = min(100, steps / 3);
  int decelSteps = min(100, steps / 3);
  
  for (int i = 0; i < steps; i++) {
    uint32_t currentDelay = minDelay;
    
    if (i < accelSteps) {
      currentDelay = startDelay - (startDelay - minDelay) * i / accelSteps;
    } else if (i > steps - decelSteps) {
      int decelPos = steps - i;
      currentDelay = minDelay + (startDelay - minDelay) * (decelSteps - decelPos) / decelSteps;
    }
    
    digitalWrite(X_STEP_PIN, HIGH);
    delayMicroseconds(currentDelay);
    digitalWrite(X_STEP_PIN, LOW);
    delayMicroseconds(currentDelay);

    currentColumn += (positive ? 1.0f : -1.0f) / STEPS_PER_COLUMN;
  }
}

void moveYSteps(int steps, bool direction) {
  const bool positive = direction;
  if (yDirectionInverted) {
    direction = !direction;
  }
  
  digitalWrite(Y_DIR_PIN, direction);
  delay(10);
  
  uint32_t minDelay = 1000000 / (MOVE_SPEED * 2);
  uint32_t startDelay = minDelay * 4;
  
  int accelSteps = min(100, steps / 3);
  int decelSteps = min(100, steps / 3);
  
  for (int i = 0; i < steps; i++) {
    uint32_t currentDelay = minDelay;
    
    if (i < accelSteps) {
      currentDelay = startDelay - (startDelay - minDelay) * i / accelSteps;
    } else if (i > steps - decelSteps) {
      int decelPos = steps - i;
      currentDelay = minDelay + (startDelay - minDelay) * (decelSteps - decelPos) / decelSteps;
    }
    
    digitalWrite(Y_STEP_PIN, HIGH);
    delayMicroseconds(currentDelay);
    digitalWrite(Y_STEP_PIN, LOW);
    delayMicroseconds(currentDelay);

    currentRow += (positive ? 1.0f : -1.0f) / STEPS_PER_ROW;
  }
}

void moveToPosition(float targetColumn, float targetRow) {
  float colDistance = targetColumn - currentColumn;
  int xSteps = abs(colDistance) * STEPS_PER_COLUMN;
  bool xDir = (colDistance > 0);

  float rowDistance = targetRow - currentRow;
  int ySteps = abs(rowDistance) * STEPS_PER_ROW;
  bool yDir = (rowDistance > 0);

  if (xSteps > 0) {
    moveXSteps(xSteps, xDir);
  }

  if (ySteps > 0) {
    moveYSteps(ySteps, yDir);
  }

  currentColumn = targetColumn;
  currentRow = targetRow;
}

void moveToSquare(char col, int row) {
  float targetCol = col - 'a' + 1;
  float targetRow = row;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(targetCol, targetRow);
}

void moveToColumn(char col) {
  moveToPosition(col - 'a' + 1, currentRow);
}

void moveToRow(int row) {
  float targetRow = row;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(currentColumn, targetRow);
}

void moveToLeftSurround() {
  float targetRow = currentRow;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(SURROUND_OFFSET, targetRow);
}

void moveToRightSurround() {
  float targetRow = currentRow;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(8.75, targetRow);
}

void moveToTopSurround() {
  float targetCol = currentColumn;
  float targetRow = 8.75;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(targetCol, targetRow);
}

void moveToBottomSurround() {
  float targetCol = currentColumn;
  float targetRow = SURROUND_OFFSET;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(targetCol, targetRow);
}

void pickUpPiece() {
  digitalWrite(ELECTROMAGNET_PIN, HIGH);
  delay(200);
}

void placePiece() {
  digitalWrite(ELECTROMAGNET_PIN, LOW);
  delay(200);
}

void executeKnightG1ToF3() {
  Serial.println("=== Knight g1 -> f3 with grid line navigation ===");
  
  Serial.println("Step 1: Go to g1, pick up knight");
  moveToSquare('g', 1);
  pickUpPiece();
  Serial.println("Knight picked up");
  
  // Piece displacement disabled - grid line navigation provides enough clearance
  /*
  Serial.println("Step 2: Shift knight left to create exit corridor");
  moveToPosition(7.0 - CLEARANCE_OFFSET, 1.0);
  placePiece();
  Serial.println("Knight placed temporarily at (6.75, 1)");
  
  Serial.println("Step 3: Move f2 pawn left");
  moveToSquare('f', 2);
  pickUpPiece();
  moveToPosition(6.0 - CLEARANCE_OFFSET, 2.0);
  placePiece();
  Serial.println("f2 pawn moved to (5.75, 2)");
  
  Serial.println("Step 4: Move g2 pawn right");
  moveToSquare('g', 2);
  pickUpPiece();
  moveToPosition(7.0 + CLEARANCE_OFFSET, 2.0);
  placePiece();
  Serial.println("g2 pawn moved to (7.25, 2)");
  
  Serial.println("Step 5: Pick up knight again");
  moveToPosition(7.0 - CLEARANCE_OFFSET, 1.0);
  pickUpPiece();
  Serial.println("Knight picked up");
  */
  
  Serial.println("Step 2: Move to grid intersection (6.5, 1.5)");
  moveToPosition(6.5, 1.5);
  
  Serial.println("Step 3: Move along grid line through gap (6.5, 2.5)");
  moveToPosition(6.5, 2.5);
  
  Serial.println("Step 4: Enter f3 center");
  moveToSquare('f', 3);
  placePiece();
  Serial.println("Knight placed at f3");
  
  // Piece restoration disabled - no pieces were displaced
  /*
  Serial.println("Step 9: Restore f2 pawn");
  moveToPosition(6.0 - CLEARANCE_OFFSET, 2.0);
  pickUpPiece();
  moveToSquare('f', 2);
  placePiece();
  Serial.println("f2 pawn restored to f2");
  
  Serial.println("Step 10: Restore g2 pawn");
  moveToPosition(7.0 + CLEARANCE_OFFSET, 2.0);
  pickUpPiece();
  moveToSquare('g', 2);
  placePiece();
  Serial.println("g2 pawn restored to g2");
  */
  
  Serial.println("Step 5: Return to center");
  moveToPosition(START_COLUMN, START_ROW);
  
  Serial.println("=== Knight move complete ===");
  Serial.println("DONE");
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
  
  Serial.println("=== Knight Offset Test ===");
  Serial.println("\nCommands:");
  Serial.println("  k       - Execute knight g1->f3 with clearance");
  Serial.println("  s       - Toggle sensor mode");
  Serial.println("  m a3    - Move to square a3");
  Serial.println("  1-8     - Move to row");
  Serial.println("  a-h     - Move to column");
  Serial.println("  l       - Move to left surround");
  Serial.println("  r       - Move to right surround");
  Serial.println("  t       - Move to top surround");
  Serial.println("  w       - Move to bottom surround");
  Serial.println("  p       - Print current position");
  Serial.println("  x       - Reset to center (4.5, 4.5)");
  Serial.println("  o       - Electromagnet ON");
  Serial.println("  O       - Electromagnet OFF");
  Serial.println("  d       - Diagnostic");
  Serial.println("=============================");
  Serial.print("Current position: ");
  Serial.print(currentColumn);
  Serial.print(", ");
  Serial.println(currentRow);
}

void loop() {
  while (Serial.available()) {
    const char ch = Serial.read();
    if (ch == '\r') {
      continue;
    }
    if (ch != '\n') {
      if (serialInput.length() < 8) {
        serialInput += ch;
      }
      continue;
    }

    serialInput.trim();
    
    if (serialInput == "k") {
      executeKnightG1ToF3();
    }
    else if (serialInput.length() == 1 && serialInput[0] >= 'a' && serialInput[0] <= 'h') {
      const char col = serialInput[0];
      moveToColumn(col);
      Serial.print("Column ");
      Serial.println(col);
    }
    else if (serialInput.length() == 1 && serialInput[0] >= '1' && serialInput[0] <= '8') {
      const int row = serialInput[0] - '0';
      moveToRow(row);
      Serial.print("Row ");
      Serial.println(row);
    }
    else if (serialInput == "s") {
      sensorMode = !sensorMode;
      Serial.print("Sensor mode: ");
      Serial.println(sensorMode ? "ON (hall sensor targeting)" : "OFF (electromagnet targeting)");
    }
    else if (serialInput.length() == 4 && serialInput[0] == 'm' && serialInput[1] == ' ') {
      const char col = serialInput[2];
      const int row = serialInput[3] - '0';
      if (col >= 'a' && col <= 'h' && row >= 1 && row <= 8) {
        moveToSquare(col, row);
        Serial.print("Moved to ");
        Serial.print(col);
        Serial.println(row);
      } else {
        Serial.println("Invalid square. Use: m a3");
      }
    }
    else if (serialInput == "p") {
      Serial.print("Current position: ");
      Serial.print(currentColumn);
      Serial.print(", ");
      Serial.println(currentRow);
    }
    else if (serialInput == "l") {
      moveToLeftSurround();
      Serial.println("Left surround");
    }
    else if (serialInput == "r") {
      moveToRightSurround();
      Serial.println("Right surround");
    }
    else if (serialInput == "t") {
      moveToTopSurround();
      Serial.println("Top surround");
    }
    else if (serialInput == "w") {
      moveToBottomSurround();
      Serial.println("Bottom surround");
    }
    else if (serialInput == "x") {
      Serial.println("Resetting to center...");
      moveToPosition(START_COLUMN, START_ROW);
      Serial.print("Position reset to: ");
      Serial.print(currentColumn);
      Serial.print(", ");
      Serial.println(currentRow);
    }
    else if (serialInput == "o") {
      digitalWrite(ELECTROMAGNET_PIN, HIGH);
      Serial.println("Electromagnet: ON");
    }
    else if (serialInput == "O") {
      digitalWrite(ELECTROMAGNET_PIN, LOW);
      Serial.println("Electromagnet: OFF");
    }
    else if (serialInput == "d") {
      Serial.println("=== DIAGNOSTIC ===");
      Serial.print("X_EN_PIN (");
      Serial.print(X_EN_PIN);
      Serial.print("): ");
      Serial.println(digitalRead(X_EN_PIN) ? "HIGH (disabled)" : "LOW (enabled)");
      Serial.print("Y_EN_PIN (");
      Serial.print(Y_EN_PIN);
      Serial.print("): ");
      Serial.println(digitalRead(Y_EN_PIN) ? "HIGH (disabled)" : "LOW (enabled)");
      Serial.print("X_STEP_PIN (");
      Serial.print(X_STEP_PIN);
      Serial.print("): ");
      Serial.println(digitalRead(X_STEP_PIN) ? "HIGH" : "LOW");
      Serial.print("Y_STEP_PIN (");
      Serial.print(Y_STEP_PIN);
      Serial.print("): ");
      Serial.println(digitalRead(Y_STEP_PIN) ? "HIGH" : "LOW");
      Serial.println("Sending test pulse...");
      digitalWrite(X_STEP_PIN, HIGH);
      delay(100);
      digitalWrite(X_STEP_PIN, LOW);
      Serial.println("Test pulse sent to X motor");
      Serial.println("==================");
    }
    else if (serialInput == "E") {
      Serial.println("Force enabling motors...");
      digitalWrite(X_EN_PIN, LOW);
      digitalWrite(Y_EN_PIN, LOW);
      Serial.println("Motors ENABLED");
      Serial.println("Try moving by hand - should be locked");
    }

    serialInput = "";
  }
}
