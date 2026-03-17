# 🌱 SmartFarm POC

> **สถานะโปรเจค: Phase 1 — Core Automation สมบูรณ์ · Phase 2 — Water Pump AUTO + Water Level Integration กำลังพัฒนา**

SmartFarm เป็นระบบควบคุมฟาร์มอัตโนมัติบน **ESP32-S3 + FreeRTOS** ออกแบบตาม **Clean Architecture** แยก business logic ออกจาก hardware เพื่อให้ทดสอบและ port ได้ง่าย

ควบคุม Water Pump / Mist System / Air Pump ผ่าน 4 ช่องทาง:
- 🌐 **Web Dashboard** — หน้าเว็บ responsive บน ESP32
- 🖥️ **Serial CLI** — ผ่าน USB / Serial Monitor (`sm <command>`)
- ⏰ **Schedule Automation** — ตาราง JSON สำหรับ Air Pump
- 🔘 **Physical Switch** — สวิตช์หน้าตู้ควบคุมโดยตรง

---

## 📋 สถานะ Feature ปัจจุบัน

| Feature | สถานะ | หมายเหตุ |
|---|---|---|
| **Mist System AUTO** — Hysteresis (Temp + Humidity) | ✅ สมบูรณ์ | ON >32°C / OFF <29°C, dead-band hold, MistGuard |
| **Mist Time Guard** | ✅ สมบูรณ์ | ON สูงสุด 3 นาที, พัก 5 นาที ป้องกัน overrun |
| **Air Pump Schedule** — ตาราง JSON | ✅ สมบูรณ์ | โหลดจาก `/schedule.json` ใน LittleFS |
| **Manual Mode** — Switch หน้าตู้ | ✅ สมบูรณ์ | Physical switch + Serial CLI + Web |
| **IDLE Mode** — ปิดระบบปลอดภัย | ✅ สมบูรณ์ | ปิด actuator ทุกตัว, reset state |
| **Serial CLI** — `sm <command>` | ✅ สมบูรณ์ | mode, relay, net, clock, status, help |
| **Web Dashboard** — `/` | ✅ สมบูรณ์ | sensor values, actuator states, mode switch |
| **WiFi Config** — AP + STA | ✅ สมบูรณ์ | offline-first AP, STA connect, timeout detect |
| **WiFi Setup Page** — `/wifi` | ✅ สมบูรณ์ | บันทึก SSID/pass ลง LittleFS, reboot |
| **RTC DS3231** — นาฬิกา | ✅ สมบูรณ์ | NTP sync, set via CLI |
| **Sensor BH1750** — แสง (Lux) | ✅ สมบูรณ์ | อ่านค่า + valid flag |
| **Sensor SHT40** — Temp + Humidity | ✅ สมบูรณ์ | ใช้ control mist AUTO |
| **Sensor DS18B20** — อุณหภูมิน้ำ | ✅ สมบูรณ์ | รองรับสูงสุด 4 ตัว (1-Wire) |
| **Water Level XKC-Y25** — CH1/CH2 | ✅ Driver พร้อม | LED alarm กระพริบ, flag ปิดอยู่ (Feature Flag) |
| **Boot Glitch Protection** | ✅ สมบูรณ์ | initRelayPins() ก่อน FreeRTOS tasks |
| **Boot Guard** — หน่วง 10 วินาที | ✅ สมบูรณ์ | ป้องกัน AUTO สั่งงานก่อน sensor stable |
| **Unit Tests** — FarmManager + ScheduledRelay | ✅ 6 test cases | ใช้ Unity framework |
| **Water Pump AUTO Logic** | 🔲 TODO | placeholder ใน FarmManager |
| **EC Sensor** | 🔲 TODO | reserved field ใน Types.h |
| **Schedule Web API (GET/POST)** | 🔲 TODO | endpoint ยังไม่มี |
| **Water Temp แสดงบน Dashboard** | 🔲 TODO | state มีแล้ว WebUI ยังไม่ render |
| **Water Level แสดงบน Dashboard** | 🔲 TODO | state มีแล้ว WebUI ยังไม่ render |

---

## 🧠 Architecture

Clean Architecture สำหรับ Embedded — dependency ไหลทางเดียวจากล่างขึ้นบน

```
┌──────────────────────────────────────────┐
│  Tasks / Main  (FreeRTOS wiring)         │  ← inputTask, controlTask, commandTask
│                                          │     networkTask, webUiTask
├──────────────────────────────────────────┤
│  Infrastructure                          │  ← SharedState, AppBoot, RtcClock
│                                          │     Esp32WiFiNetwork, Esp32WebUi
│                                          │     ScheduleStore, WifiConfigStore
├──────────────────────────────────────────┤
│  Drivers  (Hardware Adapters)            │  ← Esp32Relay, Esp32ManualSwitch
│                                          │     Esp32Bh1750Light, Esp32Sht40
│                                          │     Esp32Ds18b20, Esp32WaterLevelInput
│                                          │     RtcDs3231Time, Esp32NetModeSwitch
├──────────────────────────────────────────┤
│  Application                             │
│    FarmManager    — pump + mist decision │  ← sensor-based hysteresis + time guard
│    ScheduledRelay — air pump schedule    │  ← time-window based (minutes of day)
│    CommandService — CLI handler          │
├──────────────────────────────────────────┤
│  Domain  (ไม่มี Arduino API)             │  ← TimeSchedule, TimeWindow
├──────────────────────────────────────────┤
│  Interfaces  (Contracts / Abstractions)  │  ← ISensor, IActuator, ISchedule
│                                          │     INetwork, IUi, IClock, IModeSource
└──────────────────────────────────────────┘
```

**กฎสำคัญ:**
- **Domain layer ห้ามแตะ Arduino API** — ทดสอบได้บน host โดยไม่ต้องมี hardware
- **Relay สั่งงานจาก `controlTask` เท่านั้น** — ห้าม task อื่นสัมผัส actuator โดยตรง
- **คำสั่งจาก Serial / Web → `SharedState` เสมอ** — thread-safe ผ่าน FreeRTOS mutex
- **Air Pump ควบคุมโดย `ScheduledRelay`** — ไม่ผ่าน `FarmDecision`

---

## ⚙️ Task Map & FreeRTOS Layout

| Task | Core | Priority | หน้าที่ |
|---|---|---|---|
| `inputTask` | 1 | 1 | อ่าน sensor + switch ทุก 1 วินาที |
| `controlTask` | 1 | 2 | ตัดสินใจ + สั่ง actuator ทุก 1 วินาที |
| `commandTask` | 0 | 1 | รับ Serial CLI ทุก 50ms |
| `networkTask` | 0 | 1 | WiFi state machine (AP/STA/NTP) |
| `webUiTask` | 1 | 1 | `WebUi.tick()` ทุก 10ms |

**SharedState** คือ single source of truth ระหว่าง task ทั้งหมด (mutex-protected)

---

## ⚡ Boot Sequence

```
main::setup()
  └─ AppBoot::setup(ctx)
       ├── initRelayPins()          ← ⚠️ ต้องก่อน task — ป้องกัน boot glitch
       ├── initI2C()                ← Wire.begin(SDA=8, SCL=9)
       ├── initFileSystemAndSchedule() ← mount LittleFS + โหลด schedule.json
       ├── initModeSource()         ← Physical mode switch
       ├── initNetModeSource()      ← Net AP/STA switch
       ├── initClock()              ← RtcClock (DS3231)
       ├── initDrivers()            ← Sensor, Relay, Switch, LED begin()
       ├── initNetwork()            ← WiFi begin()
       ├── setInitialSafeState()    ← IDLE mode เสมอ
       └── startTasks()             ← สร้าง FreeRTOS tasks ทั้งหมด
```

---

## 🧩 Hardware

### Board: ESP32-S3-WROOM-1 N16R8 (44-Pin DevKitC-1)

| รายการ | ค่า |
|---|---|
| CPU | Xtensa LX7 Dual-Core 240MHz |
| Flash | 16 MB QIO |
| PSRAM | 8 MB Octal (OPI) |
| Framework | Arduino + FreeRTOS |
| PlatformIO board | `esp32-s3-devkitc-1` |

### GPIO Pinout สรุป

| GPIO | ฟังก์ชัน | หมายเหตุ |
|---|---|---|
| 8 | I2C SDA | BH1750, SHT40, DS3231 |
| 9 | I2C SCL | ต้อง pull-up 4.7kΩ ทั้ง SDA/SCL |
| 4 | 1-Wire DATA | DS18B20 (ต้อง pull-up 4.7kΩ) |
| 38 | Relay — Water Pump | Active LOW |
| 40 | Relay — Mist System | Active LOW |
| 41 | Relay — Air Pump | Active LOW |
| 18 | SW Manual Pump | INPUT_PULLUP |
| 5 | SW Manual Mist | INPUT_PULLUP |
| 15 | SW Manual Air | INPUT_PULLUP |
| 6 | SW Mode A | Mode encode bit A |
| 7 | SW Mode B | Mode encode bit B |
| 39 | SW Net (AP/STA) | Input-only GPIO |
| 1 | Water Level CH1 | XKC-Y25 |
| 2 | Water Level CH2 | XKC-Y25 |
| 47 | LED Alarm CH1 | Active HIGH |
| 10 | LED Alarm CH2 | Active HIGH |

> ⚠️ **GPIO26–37 ห้ามใช้** บนโมดูล N16R8 (Octal PSRAM) — ถูก reserve โดย Flash/PSRAM

### Mode Switch Encoding (GPIO6 + GPIO7)

| SW_MODE_A | SW_MODE_B | SystemMode |
|---|---|---|
| LOW | LOW | IDLE |
| HIGH | LOW | AUTO |
| LOW | HIGH | MANUAL |

---

## 🌡️ Control Logic

### Mist System (AUTO mode)

ใช้ **Hysteresis** ป้องกัน on/off กระชั้น:

```
Temp ≥ 32°C AND Humidity ≤ 65%RH  →  เปิด Mist
Temp ≤ 29°C OR  Humidity ≥ 75%RH  →  ปิด Mist
ระหว่างกัน                         →  คงสถานะเดิม (dead-band)
```

ถ้า valid เฉพาะ Temp → ใช้ Temp อย่างเดียว  
ถ้า valid เฉพาะ Humidity → ใช้ Humidity อย่างเดียว

### Mist Time Guard

ป้องกันปั๊มหมอกทำงานนานเกินไป:
- เปิดได้สูงสุด **3 นาที** ต่อรอบ
- บังคับพัก **5 นาที** ก่อนเปิดรอบถัดไป

### Air Pump (Time Schedule)

โหลดตารางจาก `/schedule.json` (LittleFS) เป็น Time Window หลายช่วง  
เปลี่ยน schedule โดยแก้ไขไฟล์ JSON แล้ว reboot

```json
{
  "air_pump": {
    "enabled": true,
    "windows": [
      { "start": "07:00", "end": "08:00" },
      { "start": "13:00", "end": "14:00" }
    ]
  }
}
```

### Water Pump (AUTO mode)

> 🔲 **ยังไม่ implement** — placeholder ใน `FarmManager::applyAuto()` คืน `pumpOn = false`  
> แนะนำ: ต่อยอดจาก Water Level sensor + schedule แบบ demand-driven

---

## 🌐 Web UI & API

### Pages

| URL | คำอธิบาย |
|---|---|
| `/` | Dashboard — sensor, actuator state, mode control |
| `/wifi` | WiFi Setup — ตั้ง SSID/password, hostname, AP name |
| `/wifi-saved` | หน้าแจ้ง save สำเร็จ (redirect กลับหลัง reboot) |

### REST API

| Method | Endpoint | คำอธิบาย |
|---|---|---|
| GET | `/api/status` | JSON snapshot ของระบบทั้งหมด |
| GET | `/api/wifi/config` | WiFi config ปัจจุบัน (ไม่รวม password) |
| POST | `/api/mode/auto` | เปลี่ยน mode → AUTO |
| POST | `/api/mode/manual` | เปลี่ยน mode → MANUAL |
| POST | `/api/mode/idle` | เปลี่ยน mode → IDLE |
| POST | `/api/net/on` | สั่งต่อ WiFi STA |
| POST | `/api/net/off` | สั่งตัด WiFi STA |
| POST | `/api/ntp` | สั่ง sync NTP |
| POST | `/save` | บันทึก WiFi config + reboot |

### `/api/status` Response

```json
{
  "mode": 1,
  "pump": false,
  "mist": true,
  "air": false,
  "lux": "1234.5",
  "luxValid": true,
  "temp": "30.25",
  "tempValid": true,
  "hum": "62.0",
  "humValid": true,
  "wifiMode": 3,
  "staConnected": true,
  "apIp": "192.168.4.1",
  "staIp": "192.168.1.42",
  "netState": 2,
  "netMsg": "WiFi connected."
}
```

---

## 🖥️ Serial CLI

Prefix: `sm` (ตั้งค่าใน `Config.h`)  
Baud rate: **115200**

```bash
# Mode
sm mode auto
sm mode manual
sm mode idle

# Relay (ใช้ได้เฉพาะ MANUAL mode)
sm relay pump on
sm relay pump off
sm relay mist on
sm relay mist off
sm relay air on
sm relay air off

# Network
sm net on         # ต่อ WiFi STA (ต้องมี config ก่อน)
sm net off        # ตัด STA, กลับ AP
sm net ntp        # sync NTP (ต้อง STA connected)

# Clock
sm clock set 13:30
sm clock set 13:30:00

# Status & Help
sm status
sm help
sm clear
```

---

## 🔧 การตั้งค่า (`include/Config.h`)

### Feature Flags

```cpp
#define ENABLE_WATER_LEVEL_CH1 0  // 1=เปิด, 0=ปิด
#define ENABLE_WATER_LEVEL_CH2 0  // ปิดถ้าไม่มีเซนเซอร์ต่อ
```

### Debug Log Switches

```cpp
#define DEBUG_BH1750_LOG     1
#define DEBUG_TEMP_LOG       0
#define DEBUG_TIME_LOG       1
#define DEBUG_WATER_LEVEL_LOG 1
#define DEBUG_CONTROL_LOG    1
```

### Mist Thresholds

```cpp
constexpr float HYSTERESIS_TEMP_ON  = 32.0f;  // °C — เปิด mist
constexpr float HYSTERESIS_TEMP_OFF = 29.0f;  // °C — ปิด mist
constexpr float HYSTERESIS_HUMIDITY_ON  = 65.0f; // %RH — เปิดเมื่อแห้งกว่า
constexpr float HYSTERESIS_HUMIDITY_OFF = 75.0f; // %RH — ปิดเมื่อชื้นกว่า

constexpr uint32_t MIST_MAX_ON_MS  = 3 * 60 * 1000; // 3 นาที
constexpr uint32_t MIST_MIN_OFF_MS = 5 * 60 * 1000; // 5 นาที
```

---

## 🧪 Unit Tests

ใช้ **Unity** framework ผ่าน PlatformIO

```bash
pio test
```

| Test | ครอบคลุม |
|---|---|
| `test_idle_resets_actuators` | IDLE mode ปิดทุก actuator + reset state |
| `test_auto_mist_temp_hysteresis` | Hysteresis ON/dead-band/OFF ตาม temp |
| `test_manual_overrides` | MANUAL mode ส่ง override ถูก |
| `test_scheduled_relay_in_window` | Air pump เปิด/ปิดตาม time window |
| `test_scheduled_relay_disabled` | schedule.enabled=false → ปิดทั้งวัน |
| `test_scheduled_relay_multiple_windows` | หลาย window ในวันเดียว |

---

## 🚀 Flash & Run

```bash
# 1. Clone & เปิดด้วย PlatformIO (VSCode + PlatformIO extension)

# 2. Flash firmware
pio run --target upload

# 3. Flash filesystem (LittleFS: www/, schedule.json, wifi.json)
pio run --target uploadfs

# 4. Open Serial Monitor
pio device monitor --baud 115200
```

> ⚠️ ต้อง `uploadfs` ด้วยทุกครั้งที่แก้ไขไฟล์ใน `data/`

### Boot AP

เมื่อ boot ครั้งแรก (หรือยังไม่มี WiFi config):
1. เปิด WiFi → เชื่อมต่อ `SmartFarm-Setup`
2. เปิด browser ไปที่ `http://192.168.4.1/wifi`
3. ใส่ SSID / Password → Save → รอ reboot

---

## 📦 Libraries (PlatformIO)

| Library | เวอร์ชัน | ใช้สำหรับ |
|---|---|---|
| `claws/BH1750` | ^1.3.0 | เซนเซอร์แสง |
| `adafruit/RTClib` | ^2.1.1 | RTC DS3231 |
| `bblanchon/ArduinoJson` | ^7.0.0 | JSON schedule, WiFi config, API |
| `adafruit/Adafruit SHT4x Library` | ^1.0.4 | Temp/Humidity sensor |
| `adafruit/Adafruit Unified Sensor` | ^1.1.14 | Sensor abstraction |
| `paulstoffregen/OneWire` | ^2.3.8 | 1-Wire bus |
| `milesburton/DallasTemperature` | ^3.11.0 | DS18B20 water temp |

---

## 🗺️ Roadmap — แนะนำสิ่งที่ควรทำต่อ

### Phase 2 — ใกล้ๆ นี้ (High Value)

1. **Water Pump AUTO Logic**
   - ใช้ Water Level sensor ตรวจน้ำในถัง
   - เปิด pump เมื่อน้ำต่ำกว่า threshold, ปิดเมื่อเต็ม
   - เพิ่ม pump runtime guard (เหมือน MistGuard)

2. **Water Level + Water Temp บน Dashboard**
   - State มีพร้อมแล้วใน `SharedState`
   - เพิ่ม card ใน `dashboard.html` และ field ใน `/api/status`

3. **Schedule Edit ผ่าน Web**
   - `GET /api/schedule` — อ่าน schedule ปัจจุบัน
   - `POST /api/schedule` — บันทึก schedule ลง LittleFS โดยไม่ต้อง reboot
   - เพิ่ม UI บน Dashboard

### Phase 3 — Medium Term

4. **EC Sensor**
   - Field ใน `Types.h` รองรับแล้ว (`ec` field)
   - ต้องเพิ่ม driver + ต่อ hardware

5. **Multi-Zone (Relay ขยาย)**
   - เพิ่ม relay channel สำหรับโซนรดน้ำหลายโซน
   - ขยาย `FarmDecision` หรือเพิ่ม `ScheduledRelay` instance

6. **Data Logging**
   - บันทึก sensor readings ลง LittleFS (CSV/JSON)
   - เพิ่ม `GET /api/history` สำหรับ chart บน dashboard

7. **MQTT / Home Automation**
   - Publish sensor data ไป MQTT broker (Home Assistant, Node-RED)
   - Subscribe command topic สำหรับ remote control

### Phase 4 — Long Term

8. **OTA Update**
   - Firmware update ผ่าน Web หรือ MQTT โดยไม่ต้องต่อ USB

9. **Alert System**
   - Line Notify / Telegram เมื่อน้ำต่ำ, sensor ล้มเหลว, หรือ pump ทำงานนานเกิน

---

## 📁 โครงสร้างไฟล์

```
SmartFarm_POC/
├── include/
│   ├── Config.h                    ← GPIO, thresholds, feature flags ทั้งหมด
│   ├── interfaces/                 ← ISensor, IActuator, INetwork, IClock, IUi ...
│   ├── domain/                     ← TimeSchedule, TimeWindow (ไม่มี Arduino)
│   ├── application/                ← FarmManager, ScheduledRelay, CommandService, FarmModels
│   ├── drivers/                    ← Esp32Relay, Esp32Sht40, Esp32Bh1750Light ...
│   ├── infrastructure/             ← SharedState, AppBoot, Esp32WebUi, RtcClock ...
│   └── tasks/                      ← TaskEntrypoints.h, WebUiTask.h
├── src/
│   ├── main.cpp                    ← Composition Root (wiring เท่านั้น)
│   ├── application/
│   ├── cli/
│   ├── domain/
│   ├── drivers/
│   ├── infrastructure/
│   └── tasks/
├── data/
│   ├── schedule.json               ← Air pump schedule (LittleFS)
│   ├── wifi.json                   ← WiFi credentials (LittleFS, สร้างจาก /wifi)
│   └── www/
│       ├── dashboard.html          ← Web UI หลัก
│       ├── wifi.html               ← WiFi setup page
│       └── wifi-saved.html         ← หน้าแจ้ง save สำเร็จ
├── test/
│   └── test_farmmanager.cpp        ← Unit tests (Unity)
├── platformio.ini
└── default_16MB.csv                ← Partition table (16MB flash)
```

---

## ⚠️ Known Issues & Notes

- **Water Level Sensor ปิดอยู่** (`ENABLE_WATER_LEVEL_CH1/CH2 = 0`) — เปิดเมื่อต่อเซนเซอร์จริง
- **WebUI เริ่มหลังได้รับ IP** — lazy start ในครั้งแรก ไม่ block task อื่น
- **WiFi STA timeout 15 วินาที** — กลับ AP mode อัตโนมัติ ไม่ค้างระบบ
- **`millis()` rollover** — ทุก guard ใช้ `uint32_t` subtraction ปลอดภัย ณ 49.7 วัน
- **BOOT_GUARD_MS = 10 วินาที** — AUTO mode รอ sensor stable ก่อนสั่งงาน

---

*Last updated: March 2026 | Board: ESP32-S3-WROOM-1 N16R8 | Framework: Arduino + FreeRTOS*