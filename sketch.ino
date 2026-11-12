/*
  Galvanic Skin Response (GSR) / EDA Monitor
  Board: ESP32 DevKit V1
  ADC: GPIO34 (ADC1 input-only pin)

  Physical circuit:
    3V3 -> Rskin/electrodes -> ADC node (GPIO34) -> 100k resistor -> GND

  Wokwi:
    The potentiometer signal is used as an adjustable analog emulator for
    the divider-node voltage. It is NOT a model of human skin.

  Serial commands:
    RATE 1 | RATE 2 | RATE 4 | RATE 8 | RATE 16
    BASELINE
    MARK REST | MARK TASK | MARK RECOVERY
    HELP

  Educational prototype only; not a medical device.
*/

#include <Arduino.h>
#include <math.h>

const int EDA_PIN = 34;
const float VCC_MV = 3300.0f;       // Replace with measured 3V3 supply in mV
const float RFIXED_OHM = 100000.0f; // Measure actual resistor and update
const float RAIL_MARGIN_MV = 40.0f;

const uint8_t ALLOWED_RATES[] = {1, 2, 4, 8, 16};
const size_t RATE_COUNT = sizeof(ALLOWED_RATES) / sizeof(ALLOWED_RATES[0]);

uint8_t sampleRateHz = 4;
uint32_t sampleIntervalMs = 1000UL / 4;
uint32_t lastSampleMs = 0;

float baseline_uS = NAN;
String eventLabel = "IDLE";
String commandBuffer;

float filteredVoltage_mV = NAN;
const float FILTER_ALPHA = 0.25f; // Exponential smoothing; 0..1

bool isAllowedRate(uint8_t rate) {
  for (size_t i = 0; i < RATE_COUNT; i++) {
    if (ALLOWED_RATES[i] == rate) return true;
  }
  return false;
}

void printHelp() {
  Serial.println("# Commands: RATE 1|2|4|8|16, BASELINE, MARK REST|TASK|RECOVERY, HELP");
}

void printCsvHeader() {
  Serial.println("millis,rate_hz,adc_raw,voltage_mV,resistance_ohm,conductance_uS,baseline_uS,delta_uS,event");
}

void setRate(uint8_t rate) {
  if (!isAllowedRate(rate)) {
    Serial.println("# Unsupported rate. Use 1, 2, 4, 8, or 16 Hz.");
    return;
  }
  sampleRateHz = rate;
  sampleIntervalMs = 1000UL / sampleRateHz;
  Serial.print("# Sampling rate set to ");
  Serial.print(sampleRateHz);
  Serial.println(" Hz");
}

void processCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command == "HELP") {
    printHelp();
  } else if (command.startsWith("RATE ")) {
    int requested = command.substring(5).toInt();
    if (requested > 0 && requested <= 255) setRate((uint8_t)requested);
    else Serial.println("# Invalid RATE command");
  } else if (command == "BASELINE") {
    // Baseline is captured from the most recent valid measurement.
    Serial.println("# Baseline will be updated on the next valid sample.");
    eventLabel = "BASELINE";
  } else if (command == "MARK REST") {
    eventLabel = "REST";
    Serial.println("# Event marker: REST");
  } else if (command == "MARK TASK") {
    eventLabel = "TASK";
    Serial.println("# Event marker: TASK");
  } else if (command == "MARK RECOVERY") {
    eventLabel = "RECOVERY";
    Serial.println("# Event marker: RECOVERY");
  } else {
    Serial.println("# Unknown command. Type HELP.");
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
      }
    } else if (commandBuffer.length() < 80) {
      commandBuffer += c;
    }
  }
}

void takeSample() {
  const uint32_t now = millis();

  // analogReadMilliVolts() uses ESP32 ADC calibration where supported.
  const int raw = analogRead(EDA_PIN);
  const float voltage_mV = (float)analogReadMilliVolts(EDA_PIN);

  if (isnan(filteredVoltage_mV)) filteredVoltage_mV = voltage_mV;
  else filteredVoltage_mV += FILTER_ALPHA * (voltage_mV - filteredVoltage_mV);

  const float v = filteredVoltage_mV;
  const bool nearRail = (v <= RAIL_MARGIN_MV || v >= (VCC_MV - RAIL_MARGIN_MV));

  float resistance_ohm = NAN;
  float conductance_uS = NAN;
  float delta_uS = NAN;

  if (!nearRail && v > 0.0f && v < VCC_MV) {
    resistance_ohm = RFIXED_OHM * ((VCC_MV / v) - 1.0f);

    if (resistance_ohm > 0.0f && isfinite(resistance_ohm)) {
      conductance_uS = 1000000.0f / resistance_ohm;
      if (eventLabel == "BASELINE" || isnan(baseline_uS)) {
        baseline_uS = conductance_uS;
        eventLabel = "REST";
      }
      if (!isnan(baseline_uS)) delta_uS = conductance_uS - baseline_uS;
    }
  }

  Serial.print(now);
  Serial.print(",");
  Serial.print(sampleRateHz);
  Serial.print(",");
  Serial.print(raw);
  Serial.print(",");
  Serial.print(v, 1);
  Serial.print(",");

  if (isfinite(resistance_ohm)) Serial.print(resistance_ohm, 1);
  Serial.print(",");

  if (isfinite(conductance_uS)) Serial.print(conductance_uS, 4);
  Serial.print(",");

  if (isfinite(baseline_uS)) Serial.print(baseline_uS, 4);
  Serial.print(",");

  if (isfinite(delta_uS)) Serial.print(delta_uS, 4);
  Serial.print(",");
  Serial.println(eventLabel);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  // GPIO34 is on ADC1, avoiding the ADC2/Wi-Fi contention on classic ESP32.
  analogSetPinAttenuation(EDA_PIN, ADC_11db);

  sampleIntervalMs = 1000UL / sampleRateHz;

  Serial.println("# GSR / EDA Monitor started");
  Serial.println("# Educational prototype; not a medical device.");
  Serial.print("# Default sampling rate: ");
  Serial.print(sampleRateHz);
  Serial.println(" Hz");
  printHelp();
  printCsvHeader();
}

void loop() {
  readSerialCommands();

  const uint32_t now = millis();
  if ((uint32_t)(now - lastSampleMs) >= sampleIntervalMs) {
    lastSampleMs = now;
    takeSample();
  }
}
