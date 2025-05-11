#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>
#include "chartjs.h"

// Pins
#define ONE_WIRE_BUS 4
#define LEVEL_SENSOR_PIN 14
#define VALVE_PIN 26
#define HEATER_PIN 27
#define COLD_PUMP_PIN 25
#define TARGET_PUMP_PIN 33

// Setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensor_cold, sensor_source, sensor_target;

// Pump control
unsigned long lastColdPulse = 0;
unsigned long coldPumpInterval = 10000;
unsigned long coldPumpDuration = 1000;
bool coldPumpActive = false;
float theoreticalMaxColdFlow = 400.0;
float estimatedColdFlow = 0.0;

unsigned long lastTargetPulse = 0;
unsigned long targetPumpInterval = 10000;
unsigned long targetPumpDuration = 1000;
bool targetPumpActive = false;
float theoreticalMaxTargetFlow = 400.0;
float estimatedTargetFlow = 0.0;

// Sensors
bool testMode = true;  // ou false para modo normal

float tempCold = 0.0, tempSource = 0.0, tempTarget = 0.0;
bool levelDetected = false;

// History (10 points)
float sourceTempHistory[10] = { 0 };
float targetTempHistory[10] = { 0 };
float coldTempHistory[10] = { 0 };
int currentIndex = 0;

// WiFi + Server
WebServer server(80);
const char* ssid = "ESP32-HeatControl";
const char* password = "12345678";

// Routes
void handleRoot();
void handleSet();
void handleData();


// Setup
void setup() {
  Serial.begin(115200);

  pinMode(VALVE_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(COLD_PUMP_PIN, OUTPUT);
  pinMode(TARGET_PUMP_PIN, OUTPUT);
  pinMode(LEVEL_SENSOR_PIN, INPUT);
  digitalWrite(VALVE_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(COLD_PUMP_PIN, LOW);
  digitalWrite(TARGET_PUMP_PIN, LOW);

  sensors.begin();
  sensors.getAddress(sensor_cold, 0);
  sensors.getAddress(sensor_source, 1);
  sensors.getAddress(sensor_target, 2);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);

  server.on("/chart.min.js", HTTP_GET, []() {
    server.send_P(200, "application/javascript", chartJs);
  });

  server.on("/reset_history", []() {
    for (int i = 0; i < 10; i++) {
      sourceTempHistory[i] = 0;
      targetTempHistory[i] = 0;
      coldTempHistory[i] = 0;
    }
    currentIndex = 0;
    server.send(200, "text/plain", "History reset");
  });

  server.begin();
  Serial.println("AP: ESP32-HeatControl");
}

// Main loop
void loop() {
  server.handleClient();

  if (testMode) {
    // Modo teste: gera valores aleatórios
    tempSource = random(1000, 10000) / 100.0;
    tempTarget = random(1000, 10000) / 100.0;
    tempCold = random(1000, 10000) / 100.0;
  } else {
    // Modo normal: lê sensores
    sensors.requestTemperatures();
    tempCold = sensors.getTempC(sensor_cold);
    tempSource = sensors.getTempC(sensor_source);
    tempTarget = sensors.getTempC(sensor_target);
  }

  levelDetected = digitalRead(LEVEL_SENSOR_PIN);

  unsigned long now = millis();

  // Cold pump pulsing
  if (now - lastColdPulse >= coldPumpInterval && !coldPumpActive) {
    digitalWrite(COLD_PUMP_PIN, HIGH);
    lastColdPulse = now;
    coldPumpActive = true;
  }
  if (coldPumpActive && now - lastColdPulse >= coldPumpDuration) {
    digitalWrite(COLD_PUMP_PIN, LOW);
    coldPumpActive = false;
  }

  // Target pump pulsing
  if (levelDetected) {
    if (now - lastTargetPulse >= targetPumpInterval && !targetPumpActive) {
      digitalWrite(TARGET_PUMP_PIN, HIGH);
      lastTargetPulse = now;
      targetPumpActive = true;
    }
    if (targetPumpActive && now - lastTargetPulse >= targetPumpDuration) {
      digitalWrite(TARGET_PUMP_PIN, LOW);
      targetPumpActive = false;
    }
  }

  if (tempTarget <= 35.0) {
    digitalWrite(TARGET_PUMP_PIN, LOW);
    targetPumpActive = false;
  }

  // Heater + valve
  if (tempSource < 60.0) {
    digitalWrite(HEATER_PIN, HIGH);
  } else {
    digitalWrite(HEATER_PIN, LOW);
    if (!levelDetected) digitalWrite(VALVE_PIN, HIGH);
  }

  if (levelDetected) digitalWrite(VALVE_PIN, LOW);

  // Salvar no histórico circular
  sourceTempHistory[currentIndex] = tempSource;
  targetTempHistory[currentIndex] = tempTarget;
  coldTempHistory[currentIndex] = tempCold;
  currentIndex = (currentIndex + 1) % 10;
  delay(100);
}
