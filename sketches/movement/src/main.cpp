#include <Arduino.h>
#include <stdlib.h>

const uint8_t X_STEP_PIN = 8;
const uint8_t X_DIR_PIN = 9;
const uint8_t X_EN_PIN = 10;

const uint8_t Y_STEP_PIN = 16;
const uint8_t Y_DIR_PIN = 17;
const uint8_t Y_EN_PIN = 18;

const uint8_t HALL_SENSOR_PIN = 4;
const uint8_t ELECTROMAGNET_PIN = 39;

String serialInput;
const int SERIAL_INPUT_MAX = 64;

const int STEPS_PER_COLUMN = 2220;
const int STEPS_PER_ROW = 2220;
const uint32_t MOVE_SPEED = 1000;

const float SQUARE_PITCH = 45.1;
const float HALL_Y_OFFSET = 15.0;
const float SURROUND_OFFSET = 0.25;

const float COORD_MIN = -1.5;
const float COORD_MAX = 8.5;

const uint8_t HALL_SAMPLES = 5;
const uint8_t HALL_MAX_POLLS = 20;
const uint16_t HALL_POLL_INTERVAL_MS = 15;

const float START_COLUMN = 3.5;
const float START_ROW = 3.5;
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
  float targetCol = col - 'a';
  float targetRow = row - 1;
  if (sensorMode) {
    targetRow -= HALL_Y_OFFSET / SQUARE_PITCH;
  }
  moveToPosition(targetCol, targetRow);
}

// Electromagnet target for a square. Used by piece moves, which must never
// apply the hall-sensor offset regardless of the manual sensor-mode toggle.
void moveToSquareCenter(char col, int row) {
  moveToPosition(col - 'a', row - 1);
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

void moveToSensorPosition(float x, float y) {
  moveToPosition(x, y - HALL_Y_OFFSET / SQUARE_PITCH);
}

char readHallSensor() {
  bool candidate = (digitalRead(HALL_SENSOR_PIN) == LOW);
  uint8_t count = 1;

  for (uint8_t i = 0; i < HALL_MAX_POLLS; i++) {
    delay(HALL_POLL_INTERVAL_MS);
    bool raw = (digitalRead(HALL_SENSOR_PIN) == LOW);

    if (raw != candidate) {
      candidate = raw;
      count = 1;
    } else if (count < HALL_SAMPLES) {
      count++;
    }

    if (count >= HALL_SAMPLES) {
      return candidate ? 'M' : 'E';
    }
  }

  return 'X';
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

  moveToSquareCenter(fromCol, fromRow);
  pickUpPiece();
  Serial.println("Knight picked up");

  // Travel the long leg (2 squares) along the grid line between files or
  // ranks, so the knight never passes over the center of an occupied square.
  if (fabsf(toY - fromY) > 1.5f) {
    float gridX = min(fromX, toX) + 0.5;
    moveToPosition(gridX, fromY);
    moveToPosition(gridX, toY);
    Serial.print("Moved along grid line X=");
    Serial.println(gridX);
  } else {
    float gridY = min(fromY, toY) + 0.5;
    moveToPosition(fromX, gridY);
    moveToPosition(toX, gridY);
    Serial.print("Moved along grid line Y=");
    Serial.println(gridY);
  }

  moveToPosition(toX, toY);

  placePiece();
  Serial.println("Knight placed");
}

void executeBishopMove(char fromCol, int fromRow, char toCol, int toRow) {
  Serial.println("Bishop move detected - using diagonal movement");

  float fromX = fromCol - 'a';
  float fromY = fromRow - 1;
  float toX = toCol - 'a';
  float toY = toRow - 1;

  moveToSquareCenter(fromCol, fromRow);
  pickUpPiece();
  Serial.println("Bishop picked up");

  moveDiagonal(toX, toY);
  Serial.println("Diagonal movement complete");

  placePiece();
  Serial.println("Bishop placed");
}

bool parseSquare(const String& s, float& x, float& y) {
  if (s.length() != 2) return false;
  char col = s[0];
  char rowc = s[1];
  if (col < 'a' || col > 'h' || rowc < '1' || rowc > '8') return false;
  x = col - 'a';
  y = rowc - '1';
  return true;
}

int parseFloatList(const char* str, float* out, int maxCount) {
  int count = 0;
  const char* p = str;

  while (*p == ' ') p++;
  while (*p && count < maxCount) {
    char* end;
    float v = strtof(p, &end);
    if (end == p) return -1;
    out[count++] = v;
    p = end;
    while (*p == ' ') p++;
  }

  if (*p != '\0') return -1;
  return count;
}

bool inBounds(float x, float y) {
  return x >= COORD_MIN && x <= COORD_MAX && y >= COORD_MIN && y <= COORD_MAX;
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

  if (uciMove.length() == 5) {
    char promo = uciMove[4];
    Serial.print("Promotion piece: ");
    Serial.println(promo);
  }

  if (isKnightMove(fromCol, fromRow, toCol, toRow)) {
    executeKnightMove(fromCol, fromRow, toCol, toRow);
  } else if (isBishopMove(fromCol, fromRow, toCol, toRow)) {
    executeBishopMove(fromCol, fromRow, toCol, toRow);
  } else {
    moveToSquareCenter(fromCol, fromRow);
    pickUpPiece();
    Serial.println("Picked up piece");

    moveToSquareCenter(toCol, toRow);
    placePiece();
    Serial.println("Placed piece");
  }
}

void printPosition() {
  Serial.print("Current position: ");
  Serial.print(currentColumn);
  Serial.print(", ");
  Serial.println(currentRow);
}

bool handleCommand(const String& cmd) {
  int len = cmd.length();

  if (len == 4 || len == 5) {
    bool squaresOk = cmd[0] >= 'a' && cmd[0] <= 'h' &&
                    cmd[1] >= '1' && cmd[1] <= '8' &&
                    cmd[2] >= 'a' && cmd[2] <= 'h' &&
                    cmd[3] >= '1' && cmd[3] <= '8';
    bool promoOk = len == 4 ||
                   (cmd[4] == 'q' || cmd[4] == 'r' || cmd[4] == 'b' || cmd[4] == 'n');

    if (squaresOk && promoOk) {
      executeUCIMove(cmd);
      return true;
    }
  }

  if (cmd.startsWith("scan ")) {
    String sq = cmd.substring(5);
    sq.trim();
    float x, y;
    if (!parseSquare(sq, x, y)) {
      Serial.println("ERROR: Invalid square. Use: scan e4");
      return false;
    }
    moveToSensorPosition(x, y);
    char state = readHallSensor();
    Serial.print("SCAN ");
    Serial.print(sq);
    Serial.print(" ");
    Serial.println(state);
    return true;
  }

  if (cmd.startsWith("scanpos ")) {
    float v[2];
    int n = parseFloatList(cmd.c_str() + 8, v, 2);
    if (n != 2 || !inBounds(v[0], v[1])) {
      Serial.println("ERROR: Invalid coords. Use: scanpos -0.5 3");
      return false;
    }
    moveToSensorPosition(v[0], v[1]);
    char state = readHallSensor();
    Serial.print("SCANPOS ");
    Serial.print(v[0]);
    Serial.print(" ");
    Serial.print(v[1]);
    Serial.print(" ");
    Serial.println(state);
    return true;
  }

  if (cmd.startsWith("g ")) {
    float v[2];
    int n = parseFloatList(cmd.c_str() + 2, v, 2);
    if (n != 2 || !inBounds(v[0], v[1])) {
      Serial.println("ERROR: Invalid coords. Use: g 3.5 2.0");
      return false;
    }
    moveToPosition(v[0], v[1]);
    printPosition();
    return true;
  }

  if (cmd.startsWith("path ")) {
    float v[16];
    int n = parseFloatList(cmd.c_str() + 5, v, 16);
    if (n < 2 || n % 2 != 0) {
      Serial.println("ERROR: Need waypoint pairs. Use: path 7 0.5 5 0.5");
      return false;
    }
    for (int i = 0; i < n; i += 2) {
      if (!inBounds(v[i], v[i + 1])) {
        Serial.println("ERROR: Waypoint out of bounds");
        return false;
      }
    }
    Serial.print("Path: ");
    Serial.print(n / 2);
    Serial.println(" waypoints");
    for (int i = 0; i < n; i += 2) {
      moveToPosition(v[i], v[i + 1]);
      Serial.print("Waypoint ");
      Serial.print(i / 2 + 1);
      Serial.print(": ");
      Serial.print(v[i]);
      Serial.print(", ");
      Serial.println(v[i + 1]);
    }
    return true;
  }

  if (cmd.startsWith("lane ")) {
    String slot = cmd.substring(5);
    slot.trim();
    if (slot.length() != 2) {
      Serial.println("ERROR: Invalid lane. Use: lane L3 or lane R3");
      return false;
    }
    char side = slot[0];
    int n = slot[1] - '0';
    if ((side != 'L' && side != 'R') || n < 1 || n > 8) {
      Serial.println("ERROR: Invalid lane. Use: lane L3 or lane R3");
      return false;
    }
    float x = (side == 'L') ? -0.5 : 7.5;
    float y = n - 1;
    moveToPosition(x, y);
    Serial.print("Lane ");
    Serial.print(side);
    Serial.print(n);
    Serial.print(" (");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.println(")");
    return true;
  }

  if (cmd.startsWith("m ")) {
    if (cmd.length() < 3) {
      Serial.println("ERROR: Usage: m <col> or m <col><row>");
      return false;
    }
    char col = cmd[2];
    if (col < 'a' || col > 'h') {
      Serial.println("ERROR: Invalid column");
      return false;
    }
    if (cmd.length() == 3) {
      moveToColumn(col);
      Serial.print("Column ");
      Serial.println(col);
      return true;
    }
    if (cmd.length() == 4) {
      int row = cmd[3] - '0';
      if (row < 1 || row > 8) {
        Serial.println("ERROR: Invalid row");
        return false;
      }
      moveToSquare(col, row);
      Serial.print("Moved to ");
      Serial.print(col);
      Serial.println(row);
      return true;
    }
    Serial.println("ERROR: Usage: m <col> or m <col><row>");
    return false;
  }

  if (len == 1 && cmd[0] >= 'a' && cmd[0] <= 'h') {
    moveToColumn(cmd[0]);
    Serial.print("Column ");
    Serial.println(cmd[0]);
    return true;
  }

  if (len == 1 && cmd[0] >= '1' && cmd[0] <= '8') {
    int row = cmd[0] - '0';
    moveToRow(row);
    Serial.print("Row ");
    Serial.println(row);
    return true;
  }

  if (cmd == "s") {
    sensorMode = !sensorMode;
    Serial.print("Sensor mode: ");
    Serial.println(sensorMode ? "ON (hall sensor targeting)" : "OFF (electromagnet targeting)");
    return true;
  }

  if (cmd == "p") {
    printPosition();
    return true;
  }

  if (cmd == "l") {
    moveToLeftSurround();
    Serial.println("Left surround");
    return true;
  }

  if (cmd == "r") {
    moveToRightSurround();
    Serial.println("Right surround");
    return true;
  }

  if (cmd == "t") {
    moveToTopSurround();
    Serial.println("Top surround");
    return true;
  }

  if (cmd == "w") {
    moveToBottomSurround();
    Serial.println("Bottom surround");
    return true;
  }

  if (cmd == "x") {
    Serial.println("Resetting to center...");
    moveToPosition(START_COLUMN, START_ROW);
    printPosition();
    return true;
  }

  if (cmd == "o") {
    digitalWrite(ELECTROMAGNET_PIN, HIGH);
    Serial.println("Electromagnet: ON");
    return true;
  }

  if (cmd == "O") {
    digitalWrite(ELECTROMAGNET_PIN, LOW);
    Serial.println("Electromagnet: OFF");
    return true;
  }

  if (cmd == "d") {
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
    Serial.print("HALL_SENSOR_PIN (");
    Serial.print(HALL_SENSOR_PIN);
    Serial.print("): ");
    Serial.println(digitalRead(HALL_SENSOR_PIN) == LOW ? "MAGNET" : "EMPTY");
    Serial.println("==================");
    return true;
  }

  if (cmd == "E") {
    Serial.println("Force enabling motors...");
    digitalWrite(X_EN_PIN, LOW);
    digitalWrite(Y_EN_PIN, LOW);
    Serial.println("Motors ENABLED");
    return true;
  }

  if (cmd == "h") {
    Serial.println("=== Movement Controller ===");
    Serial.println("\nCommands:");
    Serial.println("  e2e4    - UCI move (pick up, move, place)");
    Serial.println("  e7e8q   - UCI move with promotion");
    Serial.println("  s       - Toggle sensor mode");
    Serial.println("  m a3    - Move to square a3");
    Serial.println("  1-8     - Move to row");
    Serial.println("  a-h     - Move to column");
    Serial.println("  l       - Move to left surround");
    Serial.println("  r       - Move to right surround");
    Serial.println("  t       - Move to top surround");
    Serial.println("  w       - Move to bottom surround");
    Serial.println("  g 3.5 2 - Move to grid coordinates");
    Serial.println("  path x1 y1 x2 y2 ... - Move through waypoints");
    Serial.println("  lane L3 - Move to capture lane slot (L1-L8, R1-R8)");
    Serial.println("  scan e4 - Scan square with hall sensor");
    Serial.println("  scanpos -0.5 3 - Scan grid coordinates");
    Serial.println("  p       - Print current position");
    Serial.println("  x       - Reset to center");
    Serial.println("  o / O   - Electromagnet ON / OFF");
    Serial.println("  d       - Diagnostic");
    Serial.println("=============================");
    return true;
  }

  Serial.println("ERROR: Unknown command. Type 'h' for help.");
  return false;
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

  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  pinMode(ELECTROMAGNET_PIN, OUTPUT);

  digitalWrite(X_EN_PIN, LOW);
  digitalWrite(Y_EN_PIN, LOW);
  digitalWrite(ELECTROMAGNET_PIN, LOW);

  handleCommand("h");
  printPosition();
}

void loop() {
  while (Serial.available()) {
    const char ch = Serial.read();
    if (ch == '\r') {
      continue;
    }
    if (ch != '\n') {
      if (serialInput.length() < SERIAL_INPUT_MAX) {
        serialInput += ch;
      }
      continue;
    }

    serialInput.trim();

    if (serialInput.length() > 0) {
      bool ok = handleCommand(serialInput);
      if (ok) {
        Serial.println("DONE");
      }
    }

    serialInput = "";
  }
}
