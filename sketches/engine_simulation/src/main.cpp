#include <Arduino.h>

const uint8_t STEP_PIN = 16;
const uint8_t DIR_PIN = 17;
const uint8_t EN_PIN = 18;
const uint8_t ELECTROMAGNET_PIN = 39;
const uint8_t HALL_SENSOR_PIN = 4;

const int STEPS_PER_COLUMN = 2611;
const float COLUMN_WIDTH_MM = 65.0;
const float STEPS_PER_MM = STEPS_PER_COLUMN / COLUMN_WIDTH_MM;
const int SENSOR_OFFSET_MM = 5;
const int OFFSET_STEPS = SENSOR_OFFSET_MM * STEPS_PER_MM;

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
  
  Serial.print("Moving to column ");
  Serial.print(col);
  Serial.print(" (");
  Serial.print(steps);
  Serial.println(" steps)");
  
  moveSteps(steps, direction);
  currentPosition = targetCol;
}

void moveToColumnWithOffset(char col) {
  if (col < 'a' || col > 'h') {
    Serial.println("Invalid column. Use a-h");
    return;
  }
  
  int targetCol = col - 'a';
  float distance = targetCol - currentPosition;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  
  Serial.print("Moving to column ");
  Serial.print(col);
  Serial.print(" + offset (");
  Serial.print(steps + OFFSET_STEPS);
  Serial.println(" steps)");
  
  moveSteps(steps + OFFSET_STEPS, direction);
  currentPosition = targetCol + (SENSOR_OFFSET_MM / COLUMN_WIDTH_MM);
}

void alignToColumn(char col) {
  int targetCol = col - 'a';
  float distance = targetCol - currentPosition;
  int steps = abs(distance) * STEPS_PER_COLUMN;
  bool direction = (distance > 0);
  
  Serial.print("Aligning to column ");
  Serial.print(col);
  Serial.print(" (");
  Serial.print(steps);
  Serial.println(" steps)");
  
  moveSteps(steps, direction);
  currentPosition = targetCol;
}

bool detectPiece() {
  delay(100);
  return digitalRead(HALL_SENSOR_PIN) == LOW;
}

void pickUpPiece() {
  Serial.println("Picking up piece...");
  digitalWrite(ELECTROMAGNET_PIN, HIGH);
  delay(200);
}

void placePiece() {
  Serial.println("Placing piece...");
  digitalWrite(ELECTROMAGNET_PIN, LOW);
  delay(200);
}

void engineMove(char fromCol, char toCol) {
  Serial.println("=== ENGINE MOVE ===");
  Serial.print("Moving piece from ");
  Serial.print(fromCol);
  Serial.print(" to ");
  Serial.println(toCol);
  
  Serial.println("\n1. Move to source + offset (to detect piece)");
  moveToColumnWithOffset(fromCol);
  
  Serial.println("\n2. Detect piece");
  if (detectPiece()) {
    Serial.println("   Piece detected!");
  } else {
    Serial.println("   No piece detected - aborting");
    return;
  }
  
  Serial.println("\n3. Align electromagnet with piece");
  alignToColumn(fromCol);
  
  Serial.println("\n4. Pick up piece");
  pickUpPiece();
  
  Serial.println("\n5. Move to destination + offset (to check if occupied)");
  moveToColumnWithOffset(toCol);
  
  Serial.println("\n6. Check if destination is occupied");
  if (detectPiece()) {
    Serial.println("   Destination occupied - this would be a capture");
  } else {
    Serial.println("   Destination empty");
  }
  
  Serial.println("\n7. Align electromagnet with destination");
  alignToColumn(toCol);
  
  Serial.println("\n8. Place piece");
  placePiece();
  
  Serial.println("\n=== MOVE COMPLETE ===");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(ELECTROMAGNET_PIN, OUTPUT);
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  
  digitalWrite(EN_PIN, LOW);
  digitalWrite(ELECTROMAGNET_PIN, LOW);
  
  Serial.println("=== Engine Simulation ===");
  Serial.println("Simulates dark pieces (chess engine)");
  Serial.println("\nCommands:");
  Serial.println("  e2e4  - Move from e2 to e4");
  Serial.println("  d7d5  - Move from d7 to d5");
  Serial.println("  c7c5  - Move from c7 to c5");
  Serial.println("  m a   - Move to column a");
  Serial.println("  M a   - Move to column a + offset");
  Serial.println("  A a   - Align to column a");
  Serial.println("  d     - Detect piece at current position");
  Serial.println("  1     - Electromagnet ON");
  Serial.println("  0     - Electromagnet OFF");
  Serial.println("  i     - Invert direction");
  Serial.println("  r     - Home to 3.5");
  Serial.println("  p     - Print position");
  Serial.println("========================");
  Serial.print("Current position: ");
  Serial.println(currentPosition);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() == 4 && 
        input[0] >= 'a' && input[0] <= 'h' &&
        input[1] >= '1' && input[1] <= '8' &&
        input[2] >= 'a' && input[2] <= 'h' &&
        input[3] >= '1' && input[3] <= '8') {
      engineMove(input[0], input[2]);
    }
    else if (input.length() == 3 && input[0] == 'm' && input[1] == ' ') {
      moveToColumn(input[2]);
    }
    else if (input.length() == 3 && input[0] == 'M' && input[1] == ' ') {
      moveToColumnWithOffset(input[2]);
    }
    else if (input.length() == 3 && input[0] == 'A' && input[1] == ' ') {
      alignToColumn(input[2]);
    }
    else if (input == "d") {
      if (detectPiece()) {
        Serial.println("Piece detected!");
      } else {
        Serial.println("No piece detected");
      }
    }
    else if (input == "1") {
      digitalWrite(ELECTROMAGNET_PIN, HIGH);
      Serial.println("Electromagnet ON");
    }
    else if (input == "0") {
      digitalWrite(ELECTROMAGNET_PIN, LOW);
      Serial.println("Electromagnet OFF");
    }
    else if (input == "i") {
      directionInverted = !directionInverted;
      Serial.print("Direction: ");
      Serial.println(directionInverted ? "INVERTED" : "NORMAL");
    }
    else if (input == "r") {
      float distance = START_POSITION - currentPosition;
      int steps = abs(distance) * STEPS_PER_COLUMN;
      bool direction = (distance > 0);
      moveSteps(steps, direction);
      currentPosition = START_POSITION;
      Serial.println("Homed to 3.5");
    }
    else if (input == "p") {
      Serial.print("Current position: ");
      Serial.println(currentPosition);
    }
  }
}
