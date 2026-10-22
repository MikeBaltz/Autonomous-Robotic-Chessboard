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

const float START_COLUMN = 3.5;
const float START_ROW = 3.5;
float currentColumn = START_COLUMN;
float currentRow = START_ROW;

bool xDirectionInverted = false;
bool yDirectionInverted = true;
bool sensorMode = false;

float columnOffset = 0.0;
float rowOffset = 0.0;

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
  float targetCol = col - 'a';
  float targetRow = row - 1;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(targetCol, targetRow);
}

void moveToColumn(char col) {
  moveToPosition(col - 'a', currentRow);
}

void moveToRow(int row) {
  float targetRow = row - 1;
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
  moveToPosition(-0.5, targetRow);
}

void moveToRightSurround() {
  float targetRow = currentRow;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(7.5, targetRow);
}

void moveToTopSurround() {
  float targetCol = currentColumn;
  float targetRow = 7.5;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(targetCol, targetRow);
}

void moveToBottomSurround() {
  float targetCol = currentColumn;
  float targetRow = -0.5;
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

bool isKnightMove(char fromCol, int fromRow, char toCol, int toRow) {
  int dx = abs(toCol - fromCol);
  int dy = abs(toRow - fromRow);
  return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
}

bool isBishopMove(char fromCol, int fromRow, char toCol, int toRow) {
  int dx = abs(toCol - fromCol);
  int dy = abs(toRow - fromRow);
  return dx == dy && dx > 0;
}

void moveDiagonal(float targetX, float targetY) {
  float currentX = currentColumn;
  float currentY = currentRow;
  
  float dx = targetX - currentX;
  float dy = targetY - currentY;
  
  int stepsX = abs(dx) * STEPS_PER_COLUMN;
  int stepsY = abs(dy) * STEPS_PER_ROW;
  
  bool dirX = dx > 0;
  bool dirY = dy > 0;
  
  int maxSteps = max(stepsX, stepsY);
  
  if (maxSteps == 0) return;
  
  bool actualDirX = xDirectionInverted ? !dirX : dirX;
  bool actualDirY = yDirectionInverted ? !dirY : dirY;
  
  digitalWrite(X_DIR_PIN, actualDirX);
  digitalWrite(Y_DIR_PIN, actualDirY);
  delay(10);
  
  for (int i = 0; i < maxSteps; i++) {
    if (i < stepsX) {
      digitalWrite(X_STEP_PIN, HIGH);
      delayMicroseconds(500);
      digitalWrite(X_STEP_PIN, LOW);
      delayMicroseconds(500);
      currentColumn += (dirX ? 1.0f : -1.0f) / STEPS_PER_COLUMN;
    }
    
    if (i < stepsY) {
      digitalWrite(Y_STEP_PIN, HIGH);
      delayMicroseconds(500);
      digitalWrite(Y_STEP_PIN, LOW);
      delayMicroseconds(500);
      currentRow += (dirY ? 1.0f : -1.0f) / STEPS_PER_ROW;
    }
  }
}

void executeKnightMove(char fromCol, int fromRow, char toCol, int toRow) {
  Serial.println("Knight move detected - using grid line navigation");
  
  float fromX = fromCol - 'a';
  float fromY = fromRow - 1;
  float toX = toCol - 'a';
  float toY = toRow - 1;
  
  float gridX = min(fromX, toX) + 0.5;
  
  moveToSquare(fromCol, fromRow);
  pickUpPiece();
  Serial.println("Knight picked up");
  
  moveToPosition(gridX, fromY);
  Serial.print("Moved to grid line at X=");
  Serial.println(gridX);
  
  moveToPosition(gridX, toY);
  Serial.print("Moved along grid to Y=");
  Serial.println(toY);
  
  moveToPosition(toX, toY);
  Serial.print("Moved to target column X=");
  Serial.println(toX);
  
  placePiece();
  Serial.println("Knight placed");
}

void executeBishopMove(char fromCol, int fromRow, char toCol, int toRow) {
  Serial.println("Bishop move detected - using diagonal movement");
  
  float fromX = fromCol - 'a';
  float fromY = fromRow - 1;
  float toX = toCol - 'a';
  float toY = toRow - 1;
  
  moveToSquare(fromCol, fromRow);
  pickUpPiece();
  Serial.println("Bishop picked up");
  
  moveDiagonal(toX, toY);
  Serial.println("Diagonal movement complete");
  
  placePiece();
  Serial.println("Bishop placed");
}

void executeUCIMove(const String& uciMove) {
  if (uciMove.length() < 4) {
    Serial.println("ERROR: Invalid UCI move format");
    return;
  }
  
  char fromCol = uciMove[0];
  int fromRow = uciMove[1] - '0';
  char toCol = uciMove[2];
  int toRow = uciMove[3] - '0';
  
  if (fromCol < 'a' || fromCol > 'h' || fromRow < 1 || fromRow > 8 ||
      toCol < 'a' || toCol > 'h' || toRow < 1 || toRow > 8) {
    Serial.println("ERROR: Invalid square in UCI move");
    return;
  }
  
  Serial.print("Executing move: ");
  Serial.print(fromCol);
  Serial.print(fromRow);
  Serial.print(" to ");
  Serial.print(toCol);
  Serial.println(toRow);
  
  if (isKnightMove(fromCol, fromRow, toCol, toRow)) {
    executeKnightMove(fromCol, fromRow, toCol, toRow);
  } else if (isBishopMove(fromCol, fromRow, toCol, toRow)) {
    executeBishopMove(fromCol, fromRow, toCol, toRow);
  } else {
    moveToSquare(fromCol, fromRow);
    pickUpPiece();
    Serial.println("Picked up piece");
    
    moveToSquare(toCol, toRow);
    placePiece();
    Serial.println("Placed piece");
  }
  
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
  
  Serial.println("=== Movement Controller ===");
  Serial.println("\nCommands:");
  Serial.println("  e2e4    - UCI move (pick up, move, place)");
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
    
    if (serialInput.length() == 4 && 
        serialInput[0] >= 'a' && serialInput[0] <= 'h' &&
        serialInput[1] >= '1' && serialInput[1] <= '8' &&
        serialInput[2] >= 'a' && serialInput[2] <= 'h' &&
        serialInput[3] >= '1' && serialInput[3] <= '8') {
      executeUCIMove(serialInput);
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
