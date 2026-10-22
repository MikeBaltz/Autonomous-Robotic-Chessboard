#include <Arduino.h>

const uint8_t STEP_PIN = 16;
const uint8_t DIR_PIN = 17;
const uint8_t EN_PIN = 18;

void stepMotor(int steps, bool dir) {
  digitalWrite(DIR_PIN, dir);
  delay(10);
  
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  
  digitalWrite(EN_PIN, LOW);
  digitalWrite(DIR_PIN, HIGH);
  
  Serial.println("=== Motor Calibration Tool ===");
  Serial.println("Commands:");
  Serial.println("  f = Forward 1 step");
  Serial.println("  b = Backward 1 step");
  Serial.println("  + = Forward 10 steps");
  Serial.println("  - = Backward 10 steps");
  Serial.println("  m = Move 6.57cm (2611 steps)");
  Serial.println("  r = Reverse direction");
  Serial.println("  e = Enable/disable motor");
  Serial.println("  s = Status");
  Serial.println("=============================");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch(cmd) {
      case 'f':
        stepMotor(1, HIGH);
        Serial.println("Forward 1 step");
        break;
      case 'b':
        stepMotor(1, LOW);
        Serial.println("Backward 1 step");
        break;
      case '+':
        stepMotor(10, HIGH);
        Serial.println("Forward 10 steps");
        break;
      case '-':
        stepMotor(10, LOW);
        Serial.println("Backward 10 steps");
        break;
      case 'm':
        stepMotor(2611, HIGH);
        Serial.println("Moved 6.57cm forward");
        break;
      case 'r':
        digitalWrite(DIR_PIN, !digitalRead(DIR_PIN));
        Serial.print("Direction: ");
        Serial.println(digitalRead(DIR_PIN) ? "HIGH" : "LOW");
        break;
      case 'e':
        digitalWrite(EN_PIN, !digitalRead(EN_PIN));
        Serial.print("Motor: ");
        Serial.println(digitalRead(EN_PIN) ? "DISABLED" : "ENABLED");
        break;
      case 's':
        Serial.print("EN=");
        Serial.print(digitalRead(EN_PIN) ? "OFF" : "ON");
        Serial.print(" DIR=");
        Serial.println(digitalRead(DIR_PIN) ? "HIGH" : "LOW");
        break;
    }
  }
}
