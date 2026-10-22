#include <Arduino.h>

const uint8_t STEP_PIN = 16;
const uint8_t DIR_PIN = 17;
const uint8_t EN_PIN = 18;

const uint8_t HALL_SENSOR_A_PIN = 4;
const uint8_t HALL_SENSOR_B_PIN = 5;

const int STEPS_PER_COLUMN = 2611;
const float START_POSITION = 3.5;

float currentPosition = START_POSITION;

bool lastSensorA = false;
bool lastSensorB = false;

void moveSteps(int steps, bool direction) {
  digitalWrite(DIR_PIN, direction);
  delay(10);
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100);
    
    // Read sensors during movement
    bool sensorA = digitalRead(HALL_SENSOR_A_PIN) == LOW;
    bool sensorB = digitalRead(HALL_SENSOR_B_PIN) == LOW;
    
    if (sensorA || sensorB) {
      Serial.print("DETECTED at step ");
      Serial.print(i);
      Serial.print(": ");
      if (sensorA) Serial.print("A(White) ");
      if (sensorB) Serial.print("B(Black) ");
      Serial.println();
      delay(50); // Slow down to see detection
    }
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
  Serial.println(" steps");
  
  Serial.println("Watching for sensor detections...");
  moveSteps(steps, direction);
  currentPosition = targetCol;
  
  Serial.print("Now at column ");
  Serial.println(col);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  
  pinMode(HALL_SENSOR_A_PIN, INPUT_PULLUP);
  pinMode(HALL_SENSOR_B_PIN, INPUT_PULLUP);
  
  digitalWrite(EN_PIN, LOW);
  
  Serial.println("=== Sensor + Motor Test ===");
  Serial.println("Motor starts between d and e");
  Serial.println("Commands:");
  Serial.println("  a-h = Move to column (watches sensors)");
  Serial.println("  s = Read sensor state");
  Serial.println("  p = Print current position");
  Serial.println("  e = Enable/disable motor");
  Serial.println("  r = Home to 3.5");
  Serial.println("============================");
  Serial.print("Current position: ");
  Serial.println(currentPosition);
}

void loop() {
  // Continuously monitor sensors
  bool sensorA = digitalRead(HALL_SENSOR_A_PIN) == LOW;
  bool sensorB = digitalRead(HALL_SENSOR_B_PIN) == LOW;
  
  if (sensorA != lastSensorA) {
    Serial.print("Sensor A (White): ");
    Serial.println(sensorA ? "DETECTED" : "empty");
    lastSensorA = sensorA;
  }
  
  if (sensorB != lastSensorB) {
    Serial.print("Sensor B (Black): ");
    Serial.println(sensorB ? "DETECTED" : "empty");
    lastSensorB = sensorB;
  }
  
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd >= 'a' && cmd <= 'h') {
      moveToColumn(cmd);
    } else {
      switch(cmd) {
        case 's':
          Serial.print("Sensor A (White): ");
          Serial.println(sensorA ? "DETECTED" : "empty");
          Serial.print("Sensor B (Black): ");
          Serial.println(sensorB ? "DETECTED" : "empty");
          break;
        case 'p':
          Serial.print("Current position: ");
          Serial.println(currentPosition);
          break;
        case 'e':
          digitalWrite(EN_PIN, !digitalRead(EN_PIN));
          Serial.print("Motor: ");
          Serial.println(digitalRead(EN_PIN) ? "DISABLED" : "ENABLED");
          break;
        case 'r':
          {
            float distance = START_POSITION - currentPosition;
            int steps = abs(distance) * STEPS_PER_COLUMN;
            bool direction = (distance > 0);
            moveSteps(steps, direction);
            currentPosition = START_POSITION;
            Serial.println("Homed to 3.5");
          }
          break;
      }
    }
  }
}
