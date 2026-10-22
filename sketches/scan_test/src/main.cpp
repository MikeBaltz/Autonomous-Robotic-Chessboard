#include <Arduino.h>

const uint8_t X_STEP_PIN = 8;
const uint8_t X_DIR_PIN = 9;
const uint8_t X_EN_PIN = 10;

const uint8_t Y_STEP_PIN = 16;
const uint8_t Y_DIR_PIN = 17;
const uint8_t Y_EN_PIN = 18;

const uint8_t HALL_SENSOR_PIN = 4;
const uint8_t ELECTROMAGNET_PIN = 39;

const unsigned long SENSOR_POLL_INTERVAL = 10;
const uint8_t SENSOR_DEBOUNCE_SAMPLES = 5;
unsigned long lastSensorPoll = 0;
bool lastMagnetState = false;
bool candidateMagnetState = false;
uint8_t candidateSampleCount = 0;
String serialInput;

const int STEPS_PER_COLUMN = 2220;
const int STEPS_PER_ROW = 2220;
const uint32_t MOVE_SPEED = 1000;

const float SQUARE_PITCH = 45.1;
const float HALL_Y_OFFSET = 15.0;
const float SURROUND_OFFSET = 0.25;

const float START_COLUMN = 4.5;
const float START_ROW = 4.5;
float currentColumn = START_COLUMN;
float currentRow = START_ROW;

bool xDirectionInverted = false;
bool yDirectionInverted = true;
bool sensorMode = false;

void pollHallSensor() {
  const unsigned long now = millis();
  if (now - lastSensorPoll < SENSOR_POLL_INTERVAL) {
    return;
  }

  lastSensorPoll = now;
  const bool rawMagnetState = digitalRead(HALL_SENSOR_PIN) == LOW;
  if (rawMagnetState != candidateMagnetState) {
    candidateMagnetState = rawMagnetState;
    candidateSampleCount = 1;
    return;
  }

  if (candidateSampleCount < SENSOR_DEBOUNCE_SAMPLES) {
    candidateSampleCount++;
  }

  if (candidateSampleCount >= SENSOR_DEBOUNCE_SAMPLES) {
    if (candidateMagnetState != lastMagnetState) {
      lastMagnetState = candidateMagnetState;
      Serial.print("Hall: ");
      Serial.println(lastMagnetState ? "MAGNET" : "EMPTY");
    }
  }
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
  
  Serial.println("=== Scan Test (Skeleton) ===");
  Serial.println("Movement commands in: sketches/movement");
  Serial.println("============================");
}

void loop() {
  pollHallSensor();

  while (Serial.available()) {
    const char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch != '\n') {
      if (serialInput.length() < 8) {
        serialInput += ch;
      }
      continue;
    }

    serialInput.trim();
    
    if (serialInput == "o") {
      digitalWrite(ELECTROMAGNET_PIN, HIGH);
      Serial.println("Electromagnet: ON");
    }
    else if (serialInput == "O") {
      digitalWrite(ELECTROMAGNET_PIN, LOW);
      Serial.println("Electromagnet: OFF");
    }
    else if (serialInput == "p") {
      Serial.print("Current position: ");
      Serial.print(currentColumn);
      Serial.print(", ");
      Serial.println(currentRow);
    }

    serialInput = "";
  }
}
