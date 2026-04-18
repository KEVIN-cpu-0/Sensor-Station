/*
  MKR WAN 1310 — Hardware Self-Test
  ====================================
  Tests the following subsystems in sequence:
    1. LED blink      — basic GPIO output (onboard LED, pin 6)
    2. Serial output  — USB CDC serial comms
    3. Analog input   — reads A0 (leave floating to see noise, or wire a pot)
    4. LoRa radio     — initialises the SX1276 and reads RSSI

  Required library:  MKRWAN  (install via Library Manager → search "MKRWAN")

  Open Serial Monitor at 115200 baud after uploading.
*/

#include <MKRWAN.h>

LoRaModem modem;

// ── Pin definitions ───────────────────────────────────────────────────────────
const int LED_PIN    = LED_BUILTIN;  // pin 6 on MKR WAN 1310
const int ANALOG_PIN = A0;

// ── Test configuration ────────────────────────────────────────────────────────
const int BLINK_COUNT   = 6;     // number of LED flashes during LED test
const int BLINK_DELAY   = 300;   // ms per half-cycle
const int ANALOG_READS  = 5;     // number of analog samples to average
const long BAUD_RATE    = 115200;

// ── Helpers ───────────────────────────────────────────────────────────────────
void printHeader(const char* title) {
  Serial.println();
  Serial.println("────────────────────────────────");
  Serial.print  ("  ");
  Serial.println(title);
  Serial.println("────────────────────────────────");
}

void pass(const char* msg) {
  Serial.print("  [PASS]  ");
  Serial.println(msg);
}

void fail(const char* msg) {
  Serial.print("  [FAIL]  ");
  Serial.println(msg);
}

// ── Test 1 : LED blink ────────────────────────────────────────────────────────
void testLED() {
  printHeader("TEST 1 — LED blink");
  Serial.println("  Blinking onboard LED (pin LED_BUILTIN).");
  Serial.println("  Verify you see the amber LED flashing...");

  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < BLINK_COUNT; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(BLINK_DELAY);
    digitalWrite(LED_PIN, LOW);
    delay(BLINK_DELAY);
  }

  pass("LED blink sequence complete.");
}

// ── Test 2 : Serial loopback ──────────────────────────────────────────────────
void testSerial() {
  printHeader("TEST 2 — Serial communication");
  // If you can read this, serial is working.
  Serial.println("  If you can read this message, USB Serial is working.");
  Serial.print  ("  Baud rate : ");
  Serial.println(BAUD_RATE);
  pass("Serial output verified.");
}

// ── Test 3 : Analog input ─────────────────────────────────────────────────────
void testAnalog() {
  printHeader("TEST 3 — Analog input (A0)");
  Serial.println("  Reading A0 five times (floating pin = noisy values, that's OK).");

  long sum = 0;
  for (int i = 0; i < ANALOG_READS; i++) {
    int val = analogRead(ANALOG_PIN);
    Serial.print("    Sample ");
    Serial.print(i + 1);
    Serial.print(" : ");
    Serial.println(val);
    sum += val;
    delay(50);
  }

  int avg = sum / ANALOG_READS;
  Serial.print("  Average raw value : ");
  Serial.println(avg);

  // A floating pin should read somewhere in the 0–1023 range — just verify
  // the ADC is alive (i.e. not stuck at exactly 0 or 1023 every read).
  if (avg > 0 && avg < 1023) {
    pass("ADC is responsive.");
  } else {
    Serial.println("  Note: ADC reads a rail value — try grounding or floating A0.");
    pass("ADC returned a value (check wiring if unexpected).");
  }
}

// ── Test 4 : LoRa radio ───────────────────────────────────────────────────────
void testLoRa() {
  printHeader("TEST 4 — LoRa radio (SX1276)");
  Serial.println("  Initialising LoRa modem...");

  // Begin in EU868 band — change to US915 / AU915 etc. as needed.
  if (!modem.begin(EU868)) {
    fail("modem.begin() returned false — check MKRWAN library install.");
    return;
  }
  pass("Modem initialised.");

  Serial.print  ("  Module version  : ");
  Serial.println(modem.version());

  Serial.print  ("  DevEUI (unique) : ");
  Serial.println(modem.deviceEUI());

 // If modem.begin() succeeded above, the radio is confirmed working
  Serial.print  ("  Module version  : ");
  Serial.println(modem.version());
  Serial.print  ("  DevEUI          : ");
  Serial.println(modem.deviceEUI());
  pass("LoRa radio is alive — version and DevEUI retrieved.");
}

// ── setup / loop ──────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(BAUD_RATE);

  // Wait up to 5 s for a serial monitor; continue anyway (handy when battery-powered)
  unsigned long t = millis();
  while (!Serial && millis() - t < 5000) { ; }

  Serial.println();
  Serial.println("========================================");
  Serial.println("  MKR WAN 1310 — Hardware Self-Test");
  Serial.println("========================================");

  testLED();
  testSerial();
  testAnalog();
  testLoRa();

  Serial.println();
  Serial.println("========================================");
  Serial.println("  All tests complete. Check results above.");
  Serial.println("  [PASS] = subsystem OK");
  Serial.println("  [FAIL] = investigate that subsystem");
  Serial.println("========================================");
}

void loop() {
  // Nothing to do — test runs once in setup().
}
