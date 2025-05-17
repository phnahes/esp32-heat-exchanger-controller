#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "chartjs.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // (endereço, colunas, linhas)

// Pins
#define ONE_WIRE_BUS 14

#define HEATER_PIN 27       // 4
#define TARGET_PUMP_PIN 33  // 1
#define COLD_PUMP_PIN 25    // 2
#define VALVE_PIN 33        // 1

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
float coolingEfficiency = 0.0;  // inicializa como 0%


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

// Global
// Variável global para armazenar temperatura máxima do Source
float maxTempSource = 0.0;
float previousTargetTemp = 20.0;

// Display
int animationDelay = 120;

// WebServer
WebServer server(80);

// --- Funções auxiliares ---

// --- Forward declarations ---
String calcularTempoResfriamento(float targetTemp, float coldTemp, float volumeLitros, float pumpPower, float eficiencia);
void handleOverTemperatureBlink();
void displayTemperatures();
void setRelay(int pin, bool on);
bool isRelayOn(uint8_t pin);
void clearHistory();
void shiftAndAdd(float* arr, int size, float newValue);
void configRoutes();
void connectToWiFiOrAP();

void desplazar_dino();

void setManualMode(bool manual);
void manualColdPump(bool on);
void manualTargetPump(bool on);
void manualHeater(bool on);

void handleRoot();
void handleData();
void handleSet();


// --- Setup ---
void setup() {
  Serial.begin(115200);

  pinMode(HEATER_PIN, OUTPUT);
  pinMode(COLD_PUMP_PIN, OUTPUT);
  pinMode(TARGET_PUMP_PIN, OUTPUT);
  pinMode(LED_AP_PIN, OUTPUT);

  setRelay(HEATER_PIN, false);
  setRelay(COLD_PUMP_PIN, true);  // Bomba fria sempre ligada
  setRelay(TARGET_PUMP_PIN, false);
  digitalWrite(LED_AP_PIN, LOW);

  lcd.init();
  lcd.backlight();

  // Introdução
  // lcd.setCursor(0, 0);
  // lcd.print("Heat Exchanger");
  // lcd.setCursor(0, 1);
  // lcd.print("Controller");
  // delay(2000);
  // lcd.clear();

  desplazar_dino();

  // Desativa modo sleep do Wi-Fi e limpa histórico
  WiFi.setSleep(false);
  clearHistory();

  // Conecta
  connectToWiFiOrAP();
  delay(500);  // pequena pausa para garantir estado

  // Identificação do modo de operação
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("Connected to:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.SSID());
    delay(1000);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(1500);
    lcd.clear();
  } else if (WiFi.getMode() == WIFI_AP) {
    lcd.setCursor(0, 0);
    lcd.print("Access Point");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.softAPIP());
    delay(1000);
    lcd.clear();
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Wi-Fi Failed");
    lcd.setCursor(0, 1);
    lcd.print("Using fallback...");
    delay(1000);
    lcd.clear();
  }

  // Mensagem de transição para sensores
  lcd.setCursor(0, 0);
  lcd.print("Getting recipes");
  lcd.setCursor(0, 1);
  lcd.print("Temperatures...");
  delay(1000);
  lcd.clear();


  sensors.begin();
  sensors.setWaitForConversion(false);

  configRoutes();
}


// --- Loop principal ---
void loop() {
  unsigned long now = millis();

  if (now - lastWebCheck >= webCheckInterval) {
    server.handleClient();
    lastWebCheck = now;
  }

  if (manualMode) {
    setRelay(COLD_PUMP_PIN, manualColdPumpActive);
    setRelay(TARGET_PUMP_PIN, manualTargetPumpActive);
    setRelay(HEATER_PIN, manualHeaterActive);
    vTaskDelay(1);
    return;
  }

  if (!testMode && now - lastSensorRequest >= sensorRequestInterval) {
    sensors.requestTemperatures();
    tempSource = sensors.getTempC(sensor_source);
    tempTarget = sensors.getTempC(sensor_target);
    tempCold = sensors.getTempC(sensor_cold);

    // Armazena temperatura máxima do source
    if (tempSource > maxTempSource) {
      maxTempSource = tempSource;
    }

    lastSensorRequest = now;
  }

  if (now - lastSensorPrint >= sensorPrintInterval) {
    Serial.print("Source: ");
    Serial.print(tempSource, 2);
    Serial.print(" C | Target: ");
    Serial.print(tempTarget, 2);
    Serial.print(" C | Cold: ");
    Serial.print(tempCold, 2);
    Serial.print(" C | MaxSrc: ");
    Serial.println(maxTempSource, 2);

    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print("SRC:");
    // lcd.print(tempSource, 1);  // Ex: 23.4
    // lcd.print(" CLD:");
    // lcd.print(tempCold, 1);  // Ex: 17.2

    // lcd.setCursor(0, 1);
    // lcd.print("TGT:");
    // lcd.print(tempTarget, 1);  // Ex: 38.7
    // lcd.print(" EFF:");

    // int eff = (int)(coolingEfficiency + 0.5);  // Arredonda para inteiro
    // lcd.print(eff);
    // lcd.print("%");
    displayTemperatures();

    lastSensorPrint = now;
  }

  if (now - lastHistoryUpdate >= historyUpdateInterval) {
    if (testMode) {
      tempSource = random(250, 1200) / 10.0;
      tempTarget = random(250, 1200) / 10.0;
      tempCold = random(250, 1200) / 10.0;
    }
    shiftAndAdd(sourceTempHistory, HISTORY_SIZE, tempSource);
    shiftAndAdd(targetTempHistory, HISTORY_SIZE, tempTarget);
    shiftAndAdd(coldTempHistory, HISTORY_SIZE, tempCold);

    // Cálculo de eficiência térmica
    float deltaFull = tempSource - tempCold;
    float deltaTarget = tempTarget - tempCold;
    float efficiency = 0.0;

    Serial.print("[EFF] Source: ");
    Serial.print(tempSource, 2);
    Serial.print(" | Target: ");
    Serial.print(tempTarget, 2);
    Serial.print(" | Cold: ");
    Serial.print(tempCold, 2);
    Serial.print(" | ΔFull: ");
    Serial.print(deltaFull, 2);
    Serial.print(" | ΔTarget: ");
    Serial.print(deltaTarget, 2);

    if (deltaFull > 0.5) {  // Evita divisões instáveis
      if (deltaTarget < 0) {
        efficiency = 100.0;  // Target abaixo do cold → resfriamento máximo
        Serial.print(" | Target abaixo do Cold → eficiência máxima");
      } else {
        efficiency = (deltaTarget / deltaFull) * 100.0;  // Como porcentagem
        efficiency = constrain(efficiency, 0.0, 100.0);
      }
    } else {
      efficiency = 0.0;
      Serial.print(" | ΔFull muito pequeno");
    }

    Serial.print(" | Eficiência calculada: ");
    Serial.print(efficiency, 1);
    Serial.println(" %");

    // Armazena a eficiência já como porcentagem
    coolingEfficiency = efficiency;
    shiftAndAdd(efficiencyHistory, HISTORY_SIZE, efficiency);


    lastHistoryUpdate = now;
  }

  // Controle do Heater
  if (!heaterLockout) {
    if (tempSource < 60.0) {
      setRelay(HEATER_PIN, true);
      setRelay(TARGET_PUMP_PIN, false);
    } else {
      setRelay(HEATER_PIN, false);
      setRelay(TARGET_PUMP_PIN, true);
      heaterLockout = true;
    }
  } else {
    setRelay(HEATER_PIN, false);
    setRelay(TARGET_PUMP_PIN, true);
  }

  // Cold Pump sempre ligada no modo automático
  setRelay(COLD_PUMP_PIN, true);

  // Source Recipe Temperature Check
  handleOverTemperatureBlink();

  vTaskDelay(1);
}
