# ESP32 Heat Exchanger Controller

A project to control and monitor a heat exchanger system using ESP32, temperature sensors, pump control, and a responsive web interface.

## 📋 Features

- Read **3 temperature sensors** (Dallas DS18B20)
- Water level detection via digital sensor
- Automatic control of **cold recipe pump** and **target recipe pump**
- Heater control for fluid warming
- Responsive web interface with:
  - **Light/Dark mode**
  - Settings panel for fine adjustments
  - Live temperature graph (10-point rolling history)
  - **Test mode** for random value generation
  - Temperature history reset option

## ⚙️ Components

| Component | Description |
|-----------|-------------|
| ESP32 | Main controller |
| DS18B20 | Temperature sensor (x3) |
| Water level sensor | Digital water presence sensor |
| Relays | Controls heater and pumps |
| Cold Recipe pump | Cold fluid circulation |
| Target Recipe pump | Target Recipe fluid circulation |

## 🖥️ Web Interface

### Dashboard
- Displays temperatures (Source, Target, Cold)
- Water level status
- Pump states and configuration
- Real-time temperature graph

### Controls
- Light/Dark toggle
- Show/Hide Settings panel
- Pump timing adjustments
- Reset temperature history button

### Responsive
- Fully adaptable to desktop, tablet, and mobile
- Temperature graph spans full width for better readability

## 🚀 How It Works

1. **Initialization**: ESP32 starts as a Wi-Fi access point (`ESP32-HeatControl / 12345678`)
2. Periodically reads sensors and saves last 10 readings in circular buffer
3. Web interface updates data every 5 seconds
4. If `testMode = true`, fake random data is generated for simulation

## 📡 Available Routes

| Route | Description |
|-------|-------------|
| `/` | Main web dashboard |
| `/data` | JSON with latest readings and 10-point history |
| `/set` | Updates pump timing parameters |
| `/reset_history` | Clears the temperature history |
| `/chart.min.js` | Embedded Chart.js library |

## 💻 Example JSON Response

```json
{
  "source": [21.1, 22.0, ...],
  "target": [33.0, 32.5, ...],
  "cold": [12.5, 13.0, ...],
  "last_source": 22.0,
  "last_target": 32.5,
  "last_cold": 13.0
}
```

## 📝 Code Structure

- **main.ino** → setup, main loop, hardware control
- **routes.ino** → HTTP handlers + dashboard logic
- **chartjs.h** → Embedded Chart.js script

## ⚠️ Notes

- Temperature history is **not persistent** after ESP32 reset (RAM only)
- For test mode, set:
```cpp
bool testMode = true;
```

<!-- ## 🛠️ Future Improvements

- Save history in **SPIFFS or LittleFS**
- Add OTA (Over-the-Air Updates)
- Integrate real flow sensors
- Add authentication for the web interface -->

## 🧑‍💻 Credits

Developed by Paulo Nahes.