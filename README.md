# Dual-Firmware Thermostat & Zigbee Sensor Network

This repository contains **two independent but cooperating** ESP32 firmware images:

1. **Thermostat-HMI** – a touch-screen OpenTherm boiler controller (ESP32 + ILI9341 + XPT2046).  
2. **Zigbee-Sensor-Node** – a battery-friendly Zigbee end-device that reports temperature, humidity and CO₂ (ESP32-C6 / H2).

They are designed to work in the same heating ecosystem: the Zigbee node sends room data, the thermostat receives it (via MQTT/serial or any hub you add) and drives the boiler through the OpenTherm bus.

---

## 1. Thermostat-HMI (folder: `Thermostat/`)

| File | What it does |
|------|--------------|
| `main.ino` | Sets up FreeRTOS tasks, hardware, UI. |
| `hardware_pins.h` | SPI & OpenTherm pin definitions. |
| `display_config.h` | Colors, button geometry, bitmaps, graph layout. |
| `pid_config.h` | PID constants, dead-band, boiler min/max, set-point. |
| `opentherm_params.h` | List of OpenTherm message-IDs we poll + thresholds. |
| `functions.h` | UI drawing, PID calculation, OpenTherm helpers, JSON parser. |
| `tasks.h` | Four FreeRTOS tasks (see below). |

### Core FreeRTOS Tasks

| Task | Priority | Purpose |
|------|----------|---------|
| `task_serial_json_handler` | 1 | Parses incoming JSON (set-point, currentTemp, config) on Serial. |
| `task_pid_control` | 2 | Runs PID every second and calls `setBoilerTemperature()`. |
| `task_opentherm_poller` | 2 | Round-robin poll of the 13 OpenTherm IDs; emits value if delta or interval exceeded. |
| `task_touchscreen_handler` | 3 | Debounced touch; +/- buttons change set-point (17-35 °C) and call `updateTarget()`. |

### Key Functions (API surface)

| Function | Location | Notes |
|----------|----------|-------|
| `drawInterface()` | functions.h | Full paint of the 320×240 UI – call once at start. |
| `updateTarget(float)` | functions.h | Redraws big red set-point on right side. |
| `updateCurrent(float)` | functions.h | Redraws big black room temperature. |
| `drawTempGraph()` | functions.h | Sliding-window line graph (20 samples) on left side. |
| `runPID()` | functions.h | Classic PID + integral clamp; outputs boiler water temp. |
| `setStatus(bool CH, bool DHW, bool Cool)` | functions.h | Wrapper around `ot.setBoilerStatus()` with error decoding. |
| `handleJson(JSONVar)` | functions.h | Accepts `{"ID":"currentTemp","value":21.5}` or `{"ID":"setPoint","value":22.5}` etc. |

### OpenTherm Quick-Start
- Bus pins: GPIO 25 (in) / 32 (out).  
- Interrupt-driven, 1 kHz bit-bang timer.  
- Automatically prints human-readable status after every request.

---

## 2. Zigbee-Sensor-Node (folder: `ZigbeeNode/`)

| File | What it does |
|------|--------------|
| `ZigbeeSensorNode.ino` | End-device setup, endpoints, factory-reset button. |
| `Globals.h` | Endpoint numbers, RTC memory variables, reporting constants. |
| `Sensors.h` | I²C init for SHT3x (temp/RH) + SCD4x (CO₂). |
| `SleepAndBatching.h` | Deep-sleep loop with sub-sampling & batch reporting. |
| `ZigbeeCallbacks.h` | ZCL callbacks: identify LED, config cluster, “dimmer” abused as parameter setter. |

### Reporting Logic
1. Wake up → take 1 sample → store in RTC memory.  
2. After 3 sub-samples (`SUB_SAMPLES`) average & send all three clusters:  
   - Temperature Measurement (0x0402)  
   - Relative Humidity (0x0405)  
   - Carbon Dioxide (0x040D)  
3. Stay awake 30 s (configurable) to receive new `minRep` interval from coordinator.  
4. Deep-sleep until next period; RTC memory survives.

### Configuration Cluster (“Light” endpoint)
A single ZCL Level-Control cluster is reused as a **console**:  

| Level | Meaning |
|--------|---------|
| 0-63 | `minRep` = level×60 s |
| 64-127 | `maxRep` = (level-64)×60 s |
| 128-191 | `covTemp` = (level-128)/10 °C |
| 192-255 | `covCO2` = level-192 ppm |

Sending a level command immediately updates that parameter and persists it in RTC.

### Power Numbers
- ~ 25 µA in deep-sleep (ESP32-C6).  
- 3×30 s active bursts per 15 min → ~ 80 µAh on 3.7 V coin-cell → **> 1 year** life.

---

## Wiring Cheat-Sheet

### Thermostat

ILI9341    ESP32  
TFT_CS  → 5  
TFT_DC  → 16  
TFT_RST → 17  
TFT_MOSI→ 23  
TFT_CLK → 18  
TFT_MISO→ 19 (unused)  

XPT2046  
TOUCH_CS → 15  
TOUCH_IRQ→ 22  
T_DO     → 19  
T_DIN    → 23  
T_CLK    → 18  

OpenTherm  
OT_IN  → 25  
OT_OUT → 32  

### Zigbee Node

SHT3x / SCD4x  
SDA → 6  
SCL → 7  
Vcc → 3 V3  
Gnd → G  

RGB LED  
Built-in → GPIO 8 (RGB_BUILTIN)  
Boot button → GPIO 9 (already on dev-kit)  

---

## Integrating the Two

The thermostat already accepts JSON on Serial at 115 200 baud:

```json
{"ID":"currentTemp","value":21.5}
{"ID":"setPoint","value":22.0}
{"ID":"config","values":{"17":{"interval":5000,"threshold":0.2}}}
