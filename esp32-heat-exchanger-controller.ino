#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "chartjs.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Pins
#define ONE_WIRE_BUS 4
#define LEVEL_SENSOR_PIN 14
#define VALVE_PIN 25
#define HEATER_PIN 26
#define COLD_PUMP_PIN 27
#define TARGET_PUMP_PIN 33
#define LED_AP_PIN 2  // LED da placa

// WiFi Credentials
const char* ssidAP = "ESP32-HeatControl";
const char* passwordAP = "12345678";
const char* ssidSTA = "IOT-SRV";
const char* passwordSTA = "12345678";

// Setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// DeviceAddress sensor_cold, sensor_source, sensor_target;

// Sensores
const DeviceAddress sensor_cold   = { 0x28, 0x84, 0xF4, 0x49, 0xF6, 0xEF, 0x3C, 0xCE };
const DeviceAddress sensor_source = { 0x28, 0x65, 0xBF, 0x49, 0xF6, 0xD3, 0x3C, 0x29 };
const DeviceAddress sensor_target = { 0x28, 0x13, 0x3F, 0x49, 0xF6, 0x09, 0x3C, 0x77 };

// Pump control
unsigned long lastTargetPulse = 0;
unsigned long targetPumpInterval = 0;
unsigned long targetPumpDuration = 0;
bool targetPumpActive = false;

float tempCold = 0.0, tempSource = 0.0, tempTarget = 0.0;
bool levelDetected = false;
bool heaterStatus = false;
bool heaterLockout = false;

// Opcional: Cold Pump pulse parameters
unsigned long lastColdPulse = 0;
unsigned long coldPumpInterval = 10000;
unsigned long coldPumpDuration = 1000;
bool coldPumpActive = false;
float theoreticalMaxColdFlow = 1000.0;
float estimatedColdFlow = 0.0;

// Target pump
float theoreticalMaxTargetFlow = 65.0;  // bomba real = 1.0-1.1 L/min ≈ 65 L/h
float estimatedTargetFlow = 0.0;

// Manual Mode
bool manualMode = false;
bool manualColdPumpActive = false;
bool manualTargetPumpActive = false;
bool manualHeaterActive = false;
bool manualValveActive = false;

// Chart
bool testMode = false;
bool animationEnabled = false;
float lineTension = 0.4;

unsigned long lastHistoryUpdate = 0;
const unsigned long historyUpdateInterval = 10000;
const int HISTORY_SIZE = 10;
float sourceTempHistory[HISTORY_SIZE];
float targetTempHistory[HISTORY_SIZE];
float coldTempHistory[HISTORY_SIZE];
float efficiencyHistory[HISTORY_SIZE];

// WebServer
WebServer server(80);

// --- Funções auxiliares ---

void clearHistory() {
  for (int i = 0; i < HISTORY_SIZE; i++) {
    sourceTempHistory[i] = 0;
    targetTempHistory[i] = 0;
    coldTempHistory[i] = 0;
    efficiencyHistory[i] = 0;
  }
}

void shiftAndAdd(float* arr, int size, float newValue) {
  for (int i = 0; i < size - 1; i++)
    arr[i] = arr[i + 1];
  arr[size - 1] = newValue;
}

void connectToWiFiOrAP() {
  String complete_hostname = "esp32-heatcontrol";
  complete_hostname.toLowerCase();
  WiFi.setHostname(complete_hostname.c_str());
  WiFi.begin(ssidSTA, passwordSTA);
  Serial.print("Conectando à rede WiFi");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao WiFi!");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_AP_PIN, LOW);
  } else {
    Serial.println("\nFalha ao conectar, iniciando modo AP.");
    WiFi.softAP(ssidAP, passwordAP);
    Serial.println(WiFi.softAPIP());
    digitalWrite(LED_AP_PIN, HIGH);
  }
}

// --- Modo manual ---

void setManualMode(bool manual) {
  manualMode = manual;
  if (!manual) {
    digitalWrite(COLD_PUMP_PIN, LOW);
    digitalWrite(TARGET_PUMP_PIN, LOW);
    digitalWrite(HEATER_PIN, LOW);
    manualColdPumpActive = false;
    manualTargetPumpActive = false;
    manualHeaterActive = false;
  }
}

void manualColdPump(bool on) {
  if (manualMode) {
    digitalWrite(COLD_PUMP_PIN, on ? HIGH : LOW);
    manualColdPumpActive = on;
  }
}

void manualTargetPump(bool on) {
  if (manualMode) {
    digitalWrite(TARGET_PUMP_PIN, on ? HIGH : LOW);
    manualTargetPumpActive = on;
  }
}

void manualHeater(bool on) {
  if (manualMode) {
    digitalWrite(HEATER_PIN, on ? HIGH : LOW);
    manualHeaterActive = on;
  }
}

// --- Handlers web (ainda placeholders) ---
void handleRoot();
void handleData();
void handleSet();

// --- Setup ---
void setup() {
  Serial.begin(115200);

  pinMode(VALVE_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(COLD_PUMP_PIN, OUTPUT);
  pinMode(TARGET_PUMP_PIN, OUTPUT);
  pinMode(LEVEL_SENSOR_PIN, INPUT);
  pinMode(LED_AP_PIN, OUTPUT);
  digitalWrite(VALVE_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(COLD_PUMP_PIN, LOW);
  digitalWrite(TARGET_PUMP_PIN, LOW);
  digitalWrite(LED_AP_PIN, LOW);

  sensors.begin();
  sensors.setWaitForConversion(false);

  clearHistory();
  connectToWiFiOrAP();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);
  server.on("/chart.min.js", HTTP_GET, []() {
    server.send_P(200, "application/javascript", chartJs);
  });
  server.on("/reset_history", []() {
    clearHistory();
    server.send(200, "text/plain", "History reset");
  });

  // Rotas manual mode
  server.on("/manual/on", []() {
    manualMode = true;
    server.send(200, "text/plain", "Manual mode ON");
  });

  server.on("/manual/off", []() {
    manualMode = false;
    manualColdPumpActive = false;
    manualTargetPumpActive = false;
    manualHeaterActive = false;
    manualValveActive = false;
    server.send(200, "text/plain", "Manual mode OFF");
  });

  server.on("/manual/cold/on", []() {
    manualMode = true;
    manualColdPumpActive = true;
    server.send(200, "text/plain", "Cold pump ON");
  });

  server.on("/manual/cold/off", []() {
    manualMode = true;
    manualColdPumpActive = false;
    server.send(200, "text/plain", "Cold pump OFF");
  });

  server.on("/manual/target/on", []() {
    manualMode = true;
    manualTargetPumpActive = true;
    server.send(200, "text/plain", "Target pump ON");
  });

  server.on("/manual/target/off", []() {
    manualMode = true;
    manualTargetPumpActive = false;
    server.send(200, "text/plain", "Target pump OFF");
  });

  server.on("/manual/heater/on", []() {
    manualMode = true;
    manualHeaterActive = true;
    server.send(200, "text/plain", "Heater ON");
  });

  server.on("/manual/heater/off", []() {
    manualMode = true;
    manualHeaterActive = false;
    server.send(200, "text/plain", "Heater OFF");
  });

  server.on("/manual/valve/on", []() {
    manualMode = true;
    manualValveActive = true;
    server.send(200, "text/plain", "Valve OPEN");
  });

  server.on("/manual/valve/off", []() {
    manualMode = true;
    manualValveActive = false;
    server.send(200, "text/plain", "Valve CLOSED");
  });


  server.begin();
}

// --- Loop principal ---
void loop() {
  server.handleClient();

  if (manualMode) {
    digitalWrite(COLD_PUMP_PIN, manualColdPumpActive ? HIGH : LOW);
    digitalWrite(TARGET_PUMP_PIN, manualTargetPumpActive ? HIGH : LOW);
    digitalWrite(HEATER_PIN, manualHeaterActive ? HIGH : LOW);
    digitalWrite(VALVE_PIN, manualValveActive ? HIGH : LOW);
    delay(100);
    return;
  }

  unsigned long now = millis();

  // Atualiza sensores com leitura assíncrona
  if (now - lastHistoryUpdate >= historyUpdateInterval) {
    if (testMode) {
      tempSource = random(250, 1200) / 10.0;
      tempTarget = random(250, 1200) / 10.0;
      tempCold = random(250, 1200) / 10.0;
    } else {
      sensors.setWaitForConversion(false);
      sensors.requestTemperatures();

      tempSource = sensors.getTempC(sensor_source);
      tempTarget = sensors.getTempC(sensor_target);
      tempCold = sensors.getTempC(sensor_cold);
    }

    shiftAndAdd(sourceTempHistory, HISTORY_SIZE, tempSource);
    shiftAndAdd(targetTempHistory, HISTORY_SIZE, tempTarget);
    shiftAndAdd(coldTempHistory, HISTORY_SIZE, tempCold);

    float deltaT = tempSource - tempTarget;
    float efficiency = (tempSource > 0) ? (deltaT / tempSource * 100.0) : 0.0;
    shiftAndAdd(efficiencyHistory, HISTORY_SIZE, efficiency);

    lastHistoryUpdate = now;
  }

  // Atualiza nível
  levelDetected = digitalRead(LEVEL_SENSOR_PIN);

  // 👉 Cold Pump: pulso ou contínuo
  if (coldPumpInterval > 0 && coldPumpDuration > 0) {
    if (now - lastColdPulse >= coldPumpInterval && !coldPumpActive) {
      digitalWrite(COLD_PUMP_PIN, HIGH);
      lastColdPulse = now;
      coldPumpActive = true;
    }
    if (coldPumpActive && now - lastColdPulse >= coldPumpDuration) {
      digitalWrite(COLD_PUMP_PIN, LOW);
      coldPumpActive = false;
    }
  } else {
    digitalWrite(COLD_PUMP_PIN, HIGH);  // 👉 modo contínuo
  }

  // 👉 Target Pump control: depende da válvula aberta
  if (digitalRead(VALVE_PIN) == HIGH) {
    if (targetPumpInterval > 0 && targetPumpDuration > 0) {
      if (now - lastTargetPulse >= targetPumpInterval && !targetPumpActive) {
        digitalWrite(TARGET_PUMP_PIN, HIGH);
        lastTargetPulse = now;
        targetPumpActive = true;
      }
      if (targetPumpActive && now - lastTargetPulse >= targetPumpDuration) {
        digitalWrite(TARGET_PUMP_PIN, LOW);
        targetPumpActive = false;
      }
    } else {
      digitalWrite(TARGET_PUMP_PIN, HIGH);  // 👉 modo contínuo
    }
  } else {
    digitalWrite(TARGET_PUMP_PIN, LOW);
    targetPumpActive = false;
  }

  // 👉 Heater + valve + lockout
  if (!heaterLockout) {
    if (tempSource < 60.0) {
      digitalWrite(HEATER_PIN, HIGH);
    } else {
      digitalWrite(HEATER_PIN, LOW);
      if (!levelDetected) {
        digitalWrite(VALVE_PIN, HIGH);
        heaterLockout = true;
      }
    }
  } else {
    digitalWrite(HEATER_PIN, LOW);
  }

  if (levelDetected) digitalWrite(VALVE_PIN, LOW);

  delay(100);
}