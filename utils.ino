String calcularTempoResfriamento(float targetTemp, float coldTemp, float volumeLitros, float pumpPower, float eficiencia) {
  const float coolingEnergy = 4186.0;  // J para 1L reduzir 1°C
  float deltaT = targetTemp - coldTemp;
  if (deltaT < 0.1) deltaT = 0.1;

  float effectiveCoolingPower = pumpPower * eficiencia;
  float tempoPara1L1C = coolingEnergy / effectiveCoolingPower;
  float tempoTotal = (tempoPara1L1C / deltaT) * volumeLitros;

  return (tempoTotal < 60.0)
           ? String(tempoTotal, 0) + " sec"
           : String(tempoTotal / 60.0, 1) + " min";
}


bool blinkActive = false;
unsigned long blinkStartTime = 0;
unsigned long lastBlinkToggle = 0;
int blinkCount = 0;
bool backlightState = true;
bool overTempTriggered = false;


void handleOverTemperatureBlink() {
  // Borda de subida: ativa piscar só uma vez ao cruzar 60 °C
  if (!blinkActive && tempSource >= 60.0 && !overTempTriggered) {
    blinkActive = true;
    overTempTriggered = true;
    blinkStartTime = millis();
    lastBlinkToggle = millis();
    blinkCount = 0;
    backlightState = true;
    lcd.backlight();
  }

  // Reseta a borda quando temperatura volta ao normal (histerese)
  if (tempSource < 59.5 && overTempTriggered) {
    overTempTriggered = false;
  }

  // Controle de piscar não bloqueante
  if (blinkActive) {
    if (millis() - lastBlinkToggle >= 150) {
      backlightState = !backlightState;
      if (backlightState)
        lcd.backlight();
      else
        lcd.noBacklight();

      lastBlinkToggle = millis();
      blinkCount++;

      if (blinkCount >= 10) {
        blinkActive = false;
        lcd.backlight();  // garante que termina ligado
      }
    }
  }
}

// LCD Temp Screen
void displayTemperatures() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SRC   TRGT  CLD");

  // Prepara as strings formatadas com 4 caracteres (XX.X)
  char line2[17];  // 16 + 1 para null terminator
  snprintf(line2, sizeof(line2), "%4.1f %5.1f %5.1f", tempSource, tempTarget, tempCold);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}

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