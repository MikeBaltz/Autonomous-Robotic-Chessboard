// TMC2209 UART + StallGuard bring-up test.
//
// Wiring (see README.md):
//   BigTreeTech TMC2209 V1.3: the pin labelled RX is the driver's
//   single-wire UART pin (TX is unconnected). GPIO6 and GPIO5 are joined on one row and that
//   row goes to that pin on both drivers. TX runs open-drain, so no
//   resistor is needed:
//   GPIO6 (TX) --+
//   GPIO5 (RX) --+-- X UART pin, Y UART pin
//   X: MS1=GND,  MS2=GND  -> address 0      X DIAG -> GPIO7
//   Y: MS1=3.3V, MS2=GND  -> address 1      Y DIAG -> GPIO15
//
// A stall is detected either by DIAG going HIGH or by reading SG_RESULT
// over UART (SG_RESULT <= 2 * SGTHRS), so DIAG does not have to be wired.
//
// Commands (USB serial, 115200):
//   v        re-configure drivers and check UART connection
//   w        swap the UART TX/RX pins and re-check
//   e        raw UART test: shows echo (wiring) and reply (driver)
//   i        print driver settings and DIAG states
//   x+ x-    jog X positive / negative
//   y+ y-    jog Y positive / negative
//   s        stop jogging
//   f N      jog speed in steps/s (default 800)
//   t N      StallGuard threshold SGTHRS 0-255 (0 = never stall)
//   m N      max jog time in ms (safety stop, default 2000)
//   h        help

#include <Arduino.h>
#include <TMCStepper.h>
#include "driver/gpio.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"

const uint8_t X_STEP_PIN = 8;
const uint8_t X_DIR_PIN = 9;
const uint8_t X_EN_PIN = 10;
const uint8_t X_DIAG_PIN = 7;

const uint8_t Y_STEP_PIN = 16;
const uint8_t Y_DIR_PIN = 17;
const uint8_t Y_EN_PIN = 18;
const uint8_t Y_DIAG_PIN = 15;

uint8_t tmcTxPin = 6;  // to both drivers' TX pins
uint8_t tmcRxPin = 5;  // to both drivers' RX pins
const uint32_t TMC_BAUD = 115200;

// Sense resistor on most TMC2209 StepStick modules. Only matters if current
// is set over UART; this sketch leaves current to the Vref potentiometer.
const float R_SENSE = 0.11f;

// 1/8 is the TMC2209 default with MS1/MS2 low, which the existing
// 2220 steps-per-square calibration was measured with. Must be set over
// UART because Y's MS1 is now tied high for addressing.
const uint16_t MICROSTEPS = 8;

// StallGuard is unreliable while the motor is accelerating.
const uint32_t STALL_IGNORE_MS = 300;
const uint32_t RAMP_MS = 200;
const uint32_t PRINT_PERIOD_MS = 100;
const uint32_t SG_POLL_MS = 5;

TMC2209Stepper driverX(&Serial1, R_SENSE, 0);
TMC2209Stepper driverY(&Serial1, R_SENSE, 1);

struct Axis {
  const char* name;
  TMC2209Stepper* driver;
  uint8_t stepPin;
  uint8_t dirPin;
  uint8_t enPin;
  uint8_t diagPin;
  uint8_t ledcChannel;
};

Axis axisX = {"X", &driverX, X_STEP_PIN, X_DIR_PIN, X_EN_PIN, X_DIAG_PIN, 0};
Axis axisY = {"Y", &driverY, Y_STEP_PIN, Y_DIR_PIN, Y_EN_PIN, Y_DIAG_PIN, 2};

uint32_t jogSpeed = 800;
uint32_t maxJogMs = 2000;
uint8_t sgThreshold = 0;

Axis* jogAxis = nullptr;
uint32_t jogStartMs = 0;
uint32_t lastPrintMs = 0;
uint32_t currentFreq = 0;
uint32_t lastSgPollMs = 0;
uint16_t lastSg = 0;

String input;

void configureDriver(TMC2209Stepper& d) {
  d.begin();
  d.pdn_disable(true);       // PDN_UART pin is used for UART, not power-down
  d.mstep_reg_select(true);  // microsteps from register, not MS1/MS2 pins
  d.toff(4);                 // enable the driver stage
  d.microsteps(MICROSTEPS);
  d.en_spreadCycle(false);   // StealthChop: StallGuard4 only works here
  d.pwm_autoscale(true);
  d.TPWMTHRS(0);             // stay in StealthChop at all speeds
  d.TCOOLTHRS(0xFFFFF);      // StallGuard/DIAG active at all speeds
  d.SGTHRS(sgThreshold);
}

void beginUart() {
  Serial1.end();
  Serial1.begin(TMC_BAUD, SERIAL_8N1, tmcRxPin, tmcTxPin);
  // Open-drain TX: the ESP32 only pulls the line low and the pull-ups hold
  // it high, so TX and RX can share one wire with the driver's UART pin
  // without an external resistor (the driver can answer without fighting
  // the ESP32's TX output).
  // Only set the pad's open-drain bit; gpio_set_direction() would also
  // disconnect the pin from the UART peripheral.
  gpio_ll_od_enable(&GPIO, (gpio_num_t)tmcTxPin);
  gpio_pullup_en((gpio_num_t)tmcTxPin);
  gpio_pullup_en((gpio_num_t)tmcRxPin);
  delay(10);
  Serial.printf("UART: ESP32 TX=GPIO%u, RX=GPIO%u\n", tmcTxPin, tmcRxPin);
}

uint8_t tmcCrc(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t b = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if ((crc >> 7) ^ (b & 0x01)) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc <<= 1;
      }
      b >>= 1;
    }
  }
  return crc;
}

// Sends a raw IOIN read request and prints every byte that comes back.
// The first 4 bytes should be our own request echoed back through the
// single-wire UART line (proves wiring); 8 more bytes are the driver's reply.
void rawProbe(uint8_t address) {
  uint8_t req[4] = {0x05, address, 0x06, 0};
  req[3] = tmcCrc(req, 3);

  while (Serial1.available()) Serial1.read();
  Serial.printf("line idle level: %s\n",
                gpio_get_level((gpio_num_t)tmcRxPin) ? "HIGH (ok)" : "LOW (something pulls it down)");
  Serial1.write(req, 4);
  Serial1.flush();
  delay(20);

  uint8_t buf[32];
  uint8_t n = 0;
  while (Serial1.available() && n < sizeof(buf)) buf[n++] = Serial1.read();

  Serial.printf("addr %u: sent %02X %02X %02X %02X, got %u bytes:", address,
                req[0], req[1], req[2], req[3], n);
  for (uint8_t i = 0; i < n; i++) Serial.printf(" %02X", buf[i]);
  Serial.println();

  bool echo = n >= 4 && memcmp(buf, req, 4) == 0;
  bool reply = false;
  for (uint8_t i = 0; i + 2 < n; i++) {
    if (buf[i] == 0x05 && buf[i + 1] == 0xFF && buf[i + 2] == 0x06) reply = true;
  }
  if (reply) {
    Serial.println("  -> driver REPLIED: UART works");
  } else if (echo) {
    Serial.println("  -> echo only: wires OK, driver silent (12V? MS1/MS2 address? driver dead?)");
  } else if (n == 0) {
    Serial.println("  -> nothing: TX/RX wires not reaching the driver pins (or swapped: try w)");
  } else {
    Serial.println("  -> garbage: check GND between ESP32 and drivers");
  }
}

void printConnection(Axis& a) {
  uint8_t result = a.driver->test_connection();
  Serial.printf("%s driver: ", a.name);
  if (result == 0) {
    Serial.printf("OK, version 0x%02X (expect 0x21)\n", a.driver->version());
  } else if (result == 1) {
    Serial.println("NO RESPONSE (reads all 1s) - check UART wire, 1k resistor, PDN_UART pin");
  } else {
    Serial.println("NO RESPONSE (reads all 0s) - check RX wire, address (MS1/MS2), VDD/GND");
  }
}

void printInfo(Axis& a) {
  TMC2209Stepper& d = *a.driver;
  Serial.printf(
      "%s: microsteps=%u SGTHRS=%u SG_RESULT=%u TSTEP=%lu spreadCycle=%d DIAG=%s\n",
      a.name, d.microsteps(), d.SGTHRS(), d.SG_RESULT(),
      (unsigned long)d.TSTEP(), d.en_spreadCycle(),
      digitalRead(a.diagPin) ? "HIGH" : "LOW");
}

void stopJog(const char* reason) {
  if (!jogAxis) return;
  ledcWrite(jogAxis->ledcChannel, 0);
  ledcDetachPin(jogAxis->stepPin);
  pinMode(jogAxis->stepPin, OUTPUT);
  digitalWrite(jogAxis->stepPin, LOW);
  Serial.printf("%s stopped: %s after %lu ms\n", jogAxis->name, reason,
                (unsigned long)(millis() - jogStartMs));
  jogAxis = nullptr;
}

void startJog(Axis& a, bool positive) {
  stopJog("new jog");
  digitalWrite(a.dirPin, positive ? HIGH : LOW);
  delay(5);

  currentFreq = max<uint32_t>(jogSpeed / 4, 50);
  ledcSetup(a.ledcChannel, currentFreq, 8);
  ledcAttachPin(a.stepPin, a.ledcChannel);
  ledcWrite(a.ledcChannel, 128);  // 50% duty square wave = one step per period

  jogAxis = &a;
  jogStartMs = millis();
  lastPrintMs = 0;
  lastSgPollMs = 0;
  lastSg = 0xFFFF;
  Serial.printf("Jogging %s%c at %lu steps/s (SGTHRS=%u, max %lu ms)\n", a.name,
                positive ? '+' : '-', (unsigned long)jogSpeed, sgThreshold,
                (unsigned long)maxJogMs);
}

void updateJog() {
  if (!jogAxis) return;
  const uint32_t elapsed = millis() - jogStartMs;

  // Linear ramp from speed/4 to full speed.
  uint32_t target = jogSpeed;
  if (elapsed < RAMP_MS) {
    uint32_t start = max<uint32_t>(jogSpeed / 4, 50);
    target = start + (jogSpeed - start) * elapsed / RAMP_MS;
  }
  if (target != currentFreq) {
    currentFreq = target;
    ledcChangeFrequency(jogAxis->ledcChannel, currentFreq, 8);
  }

  if (millis() - lastSgPollMs >= SG_POLL_MS) {
    lastSgPollMs = millis();
    lastSg = jogAxis->driver->SG_RESULT();
  }

  if (elapsed > STALL_IGNORE_MS) {
    if (digitalRead(jogAxis->diagPin) == HIGH) {
      stopJog("STALL (DIAG HIGH)");
      return;
    }
    if (sgThreshold > 0 && lastSg <= 2u * sgThreshold) {
      stopJog("STALL (SG_RESULT over UART)");
      return;
    }
  }
  if (elapsed > maxJogMs) {
    stopJog("max jog time");
    return;
  }
  if (millis() - lastPrintMs >= PRINT_PERIOD_MS) {
    lastPrintMs = millis();
    Serial.printf("  t=%4lu ms  SG_RESULT=%3u  DIAG=%s\n", (unsigned long)elapsed,
                  lastSg,
                  digitalRead(jogAxis->diagPin) ? "HIGH" : "LOW");
  }
}

void printHelp() {
  Serial.println("=== TMC2209 UART / StallGuard test ===");
  Serial.println("  v        re-configure drivers and check UART connection");
  Serial.println("  w        swap UART TX/RX pins and re-check");
  Serial.println("  e        raw UART echo/reply test");
  Serial.println("  i        driver info");
  Serial.println("  x+ x-    jog X");
  Serial.println("  y+ y-    jog Y");
  Serial.println("  s        stop");
  Serial.println("  f N      jog speed steps/s");
  Serial.println("  t N      StallGuard threshold (0-255, 0 = off)");
  Serial.println("  m N      max jog time ms");
  Serial.println("Stall when SG_RESULT <= 2 * SGTHRS.");
}

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd == "v") {
    // Re-send the configuration too, in case 12 V was off at boot.
    configureDriver(driverX);
    configureDriver(driverY);
    printConnection(axisX);
    printConnection(axisY);
  } else if (cmd == "w") {
    uint8_t tmp = tmcTxPin;
    tmcTxPin = tmcRxPin;
    tmcRxPin = tmp;
    beginUart();
    configureDriver(driverX);
    configureDriver(driverY);
    printConnection(axisX);
    printConnection(axisY);
  } else if (cmd == "e") {
    Serial.printf("UART: ESP32 TX=GPIO%u, RX=GPIO%u\n", tmcTxPin, tmcRxPin);
    for (uint8_t addr = 0; addr < 4; addr++) rawProbe(addr);
  } else if (cmd == "i") {
    printInfo(axisX);
    printInfo(axisY);
  } else if (cmd == "x+") {
    startJog(axisX, true);
  } else if (cmd == "x-") {
    startJog(axisX, false);
  } else if (cmd == "y+") {
    startJog(axisY, true);
  } else if (cmd == "y-") {
    startJog(axisY, false);
  } else if (cmd == "s") {
    stopJog("user");
  } else if (cmd.startsWith("f ")) {
    long v = cmd.substring(2).toInt();
    if (v < 50 || v > 5000) {
      Serial.println("ERROR: speed must be 50-5000");
      return;
    }
    jogSpeed = v;
    Serial.printf("Jog speed %lu steps/s\n", (unsigned long)jogSpeed);
  } else if (cmd.startsWith("t ")) {
    long v = cmd.substring(2).toInt();
    if (v < 0 || v > 255) {
      Serial.println("ERROR: threshold must be 0-255");
      return;
    }
    sgThreshold = v;
    driverX.SGTHRS(sgThreshold);
    driverY.SGTHRS(sgThreshold);
    Serial.printf("SGTHRS = %u (stall when SG_RESULT <= %u)\n", sgThreshold,
                  2 * sgThreshold);
  } else if (cmd.startsWith("m ")) {
    long v = cmd.substring(2).toInt();
    if (v < 100 || v > 30000) {
      Serial.println("ERROR: max jog time must be 100-30000 ms");
      return;
    }
    maxJogMs = v;
    Serial.printf("Max jog time %lu ms\n", (unsigned long)maxJogMs);
  } else if (cmd == "h") {
    printHelp();
  } else {
    Serial.println("ERROR: unknown command, type h");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  for (Axis* a : {&axisX, &axisY}) {
    pinMode(a->stepPin, OUTPUT);
    pinMode(a->dirPin, OUTPUT);
    pinMode(a->enPin, OUTPUT);
    pinMode(a->diagPin, INPUT_PULLDOWN);  // reads LOW if DIAG is not wired
    digitalWrite(a->stepPin, LOW);
    digitalWrite(a->enPin, HIGH);  // disabled until configured
  }

  beginUart();
  configureDriver(driverX);
  configureDriver(driverY);

  digitalWrite(X_EN_PIN, LOW);
  digitalWrite(Y_EN_PIN, LOW);

  printHelp();
  printConnection(axisX);
  printConnection(axisY);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      handleCommand(input);
      input = "";
    } else if (input.length() < 32) {
      input += c;
    }
  }
  updateJog();
}
