#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "chartjs.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Defina o endereço do seu adaptador I2C do LCD.
// A maioria usa 0x27 ou 0x3F. Teste se necessário.

LiquidCrystal_I2C lcd(0x27, 16, 2);  // (endereço, colunas, linhas)

// Pins
#define ONE_WIRE_BUS 23
#define LEVEL_SENSOR_PIN 14

#define HEATER_PIN 33
#define VALVE_PIN 32
#define TARGET_PUMP_PIN 25
#define COLD_PUMP_PIN 26

#define LED_AP_PIN 2  // LED da placa

// WiFi Credentials
const char* ssidAP = "ESP32-HeatControl";
const char* passwordAP = "12345678";
const char* ssidSTA = "IOT-SRV";
const char* passwordSTA = "p4ul0n4h3s@123";

// Setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// DeviceAddress sensor_cold, sensor_source, sensor_target;

// Sensores
const DeviceAddress sensor_cold = { 0x28, 0x84, 0xF4, 0x49, 0xF6, 0xEF, 0x3C, 0xCE };
const DeviceAddress sensor_source = { 0x28, 0x65, 0xBF, 0x49, 0xF6, 0xD3, 0x3C, 0x29 };
const DeviceAddress sensor_target = { 0x28, 0x13, 0x3F, 0x49, 0xF6, 0x09, 0x3C, 0x77 };

unsigned long lastWebCheck = 0;
unsigned long lastSensorRequest = 0;
const unsigned long webCheckInterval = 20;         // 20ms para webserver
const unsigned long sensorRequestInterval = 2000;  // 30s

unsigned long lastSensorPrint = 0;
const unsigned long sensorPrintInterval = 2000;

// Pump control
unsigned long lastTargetPulse = 0;
unsigned long targetPumpInterval = 0;
unsigned long targetPumpDuration = 0;
bool targetPumpActive = false;

float tempCold = 20.0, tempSource = 20.0, tempTarget = 20.0;
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
const unsigned long historyUpdateInterval = 5000;
const int HISTORY_SIZE = 10;
float sourceTempHistory[HISTORY_SIZE];
float targetTempHistory[HISTORY_SIZE];
float coldTempHistory[HISTORY_SIZE];
float efficiencyHistory[HISTORY_SIZE];

// WebServer
WebServer server(80);

// --- Funções auxiliares ---

// Adicione esta função em seu código
void setRelay(int pin, bool on) {
  // Para módulos de relé low-level trigger:
  // LOW = ligado; HIGH = desligado
  digitalWrite(pin, on ? LOW : HIGH);
}

bool isRelayOn(uint8_t pin) {
  return digitalRead(pin) == LOW;  // LOW = relé ativo (ON)
}

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
  setRelay(VALVE_PIN, false);
  setRelay(HEATER_PIN, false);
  setRelay(COLD_PUMP_PIN, false);
  setRelay(TARGET_PUMP_PIN, false);
  digitalWrite(LED_AP_PIN, LOW);

  lcd.init();  // initialize the lcd
  lcd.init();
  lcd.backlight();  // Liga o backlight
  lcd.setCursor(0, 0);
  lcd.print("Heat Control");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();

  sensors.begin();
  sensors.setWaitForConversion(false);

  WiFi.setSleep(false);

  clearHistory();
  connectToWiFiOrAP();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);
  server.on("/chart.min.js", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "public, max-age=31536000");  // 1 ano de cache
    server.send_P(200, "application/javascript", chartJs);
  });
  server.on("/reset_history", []() {
    clearHistory();
    server.send(200, "text/plain", "History reset");
  });

  // Rotas manual mode
  server.on("/manual/on", []() {
    manualMode = true;
    server.send(200, "text/plain", "Manual ON");
  });
  server.on("/manual/off", []() {
    manualMode = false;
    manualColdPumpActive = manualTargetPumpActive = manualHeaterActive = manualValveActive = false;
    server.send(200, "text/plain", "Manual OFF");
  });
  server.on("/manual/cold/on", []() {
    manualMode = true;
    manualColdPumpActive = true;
    server.send(200, "text/plain", "Cold Pump ON");
  });
  server.on("/manual/cold/off", []() {
    manualMode = true;
    manualColdPumpActive = false;
    server.send(200, "text/plain", "Cold Pump OFF");
  });
  server.on("/manual/target/on", []() {
    manualMode = true;
    manualTargetPumpActive = true;
    server.send(200, "text/plain", "Target Pump ON");
  });
  server.on("/manual/target/off", []() {
    manualMode = true;
    manualTargetPumpActive = false;
    server.send(200, "text/plain", "Target Pump OFF");
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
  unsigned long now = millis();

  // Webserver
  if (now - lastWebCheck >= webCheckInterval) {
    server.handleClient();
    lastWebCheck = now;
  }

  if (manualMode) {
    setRelay(COLD_PUMP_PIN, manualColdPumpActive);
    setRelay(TARGET_PUMP_PIN, manualTargetPumpActive);
    setRelay(HEATER_PIN, manualHeaterActive);
    setRelay(VALVE_PIN, manualValveActive);
    vTaskDelay(1);
    return;
  }

  // ✅ Leitura contínua dos sensores
  if (!testMode && now - lastSensorRequest >= sensorRequestInterval) {
    sensors.requestTemperatures();  // ✅ esta linha resolve o problema

    tempSource = sensors.getTempC(sensor_source);
    tempTarget = sensors.getTempC(sensor_target);
    tempCold = sensors.getTempC(sensor_cold);
    lastSensorRequest = now;
  }

  // ✅ Log Serial
  if (now - lastSensorPrint >= sensorPrintInterval) {
    Serial.print("Source: ");
    Serial.print(tempSource, 2);
    Serial.print(" C | Target: ");
    Serial.print(tempTarget, 2);
    Serial.print(" C | Cold: ");
    Serial.print(tempCold, 2);
    Serial.println(" C");

    // ✅ LCD display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Src:");
    lcd.print(tempSource, 1);
    lcd.print(" Tgt:");
    lcd.print(tempTarget, 1);

    lcd.setCursor(0, 1);
    lcd.print("Cold:");
    lcd.print(tempCold, 1);
    lcd.print("C");


    lastSensorPrint = now;
  }

  // ✅ Atualiza histórico (mantém a lógica)
  if (now - lastHistoryUpdate >= historyUpdateInterval) {
    if (testMode) {
      tempSource = random(250, 1200) / 10.0;
      tempTarget = random(250, 1200) / 10.0;
      tempCold = random(250, 1200) / 10.0;
    }

    shiftAndAdd(sourceTempHistory, HISTORY_SIZE, tempSource);
    shiftAndAdd(targetTempHistory, HISTORY_SIZE, tempTarget);
    shiftAndAdd(coldTempHistory, HISTORY_SIZE, tempCold);

    float deltaT = tempSource - tempTarget;
    float efficiency = (tempSource > 0) ? (deltaT / tempSource * 100.0) : 0.0;
    shiftAndAdd(efficiencyHistory, HISTORY_SIZE, efficiency);

    lastHistoryUpdate = now;
  }

  // Leitura do nível
  levelDetected = digitalRead(LEVEL_SENSOR_PIN);

  // Controle da Cold Pump
  if (coldPumpInterval > 0 && coldPumpDuration > 0) {
    if (now - lastColdPulse >= coldPumpInterval && !coldPumpActive) {
      setRelay(COLD_PUMP_PIN, true);
      lastColdPulse = now;
      coldPumpActive = true;
    }
    if (coldPumpActive && now - lastColdPulse >= coldPumpDuration) {
      setRelay(COLD_PUMP_PIN, false);
      coldPumpActive = false;
    }
  } else {
    setRelay(COLD_PUMP_PIN, true);
  }

  // Controle da Target Pump
  if (digitalRead(VALVE_PIN) == LOW) {
    if (targetPumpInterval > 0 && targetPumpDuration > 0) {
      if (now - lastTargetPulse >= targetPumpInterval && !targetPumpActive) {
        setRelay(TARGET_PUMP_PIN, true);
        lastTargetPulse = now;
        targetPumpActive = true;
      }
      if (targetPumpActive && now - lastTargetPulse >= targetPumpDuration) {
        setRelay(TARGET_PUMP_PIN, false);
        targetPumpActive = false;
      }
    } else {
      setRelay(TARGET_PUMP_PIN, true);
    }
  } else {
    setRelay(TARGET_PUMP_PIN, false);
    targetPumpActive = false;
  }

  // Controle do Heater e Valve
  if (!heaterLockout) {
    if (tempSource < 60.0) {
      setRelay(HEATER_PIN, true);
    } else {
      setRelay(HEATER_PIN, false);
      setRelay(VALVE_PIN, true);
      heaterLockout = true;
    }
  } else {
    setRelay(HEATER_PIN, false);
  }

  vTaskDelay(1);
}
