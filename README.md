# 🌱 SmartFarm POC

> **⚠️ อยู่ระหว่างพัฒนา — Proof of Concept**

SmartFarm เป็นระบบควบคุมฟาร์มอัตโนมัติที่ออกแบบให้ **แยก business logic ออกจาก hardware**  
เพื่อให้สามารถ port ไป platform อื่น เช่น PLC หรือ Linux ได้ในอนาคต

ใช้ **ESP32-S3 + FreeRTOS + Clean Architecture** ควบคุมระบบ Water Pump / Mist / Air Pump  
ผ่าน Web Dashboard, Serial CLI, Schedule Automation และ Manual Switch

---

## 🧠 Architecture

โปรเจคใช้ Clean Architecture สำหรับ embedded — dependency ไหลทางเดียวจากล่างขึ้นบน

```
┌─────────────────────────────────────┐
│  Tasks / Main (FreeRTOS wiring)     │  ← SensorTask, CommandTask, NetworkTask, WebUiTask
├─────────────────────────────────────┤
│  Infrastructure                     │  ← SharedState, AppBoot, WiFi, RtcClock
├─────────────────────────────────────┤
│  Drivers (Hardware Adapters)        │  ← Relay, Switch, BH1750, SHT40, DS18B20
├─────────────────────────────────────┤
│  Application                        │
│    FarmManager    — pump + mist     │  ← sensor-based hysteresis
│    ScheduledRelay — air pump        │  ← time-based schedule
│    CommandService — CLI actions     │
├─────────────────────────────────────┤
│  Domain                             │  ← TimeSchedule, TimeWindow (ไม่มี Arduino)
├─────────────────────────────────────┤
│  Interfaces (Contracts)             │  ← ISensor, IActuator, ISchedule, INetwork, IUi
└─────────────────────────────────────┘
```

**กฎสำคัญ:**

- Domain layer **ห้ามแตะ Arduino API** — ทดสอบได้โดยไม่ต้องมี hardware
- Relay สั่งได้เฉพาะใน `controlTask` เท่านั้น
- คำสั่งจาก Serial / Web ต้องผ่าน `SharedState` เสมอ (thread-safe)
- Air pump ควบคุมโดย `ScheduledRelay` — ไม่ผ่าน `FarmDecision`

---

## ⚙️ Boot Flow

```
boot → AppBoot → mount LittleFS → init SystemContext → start FreeRTOS tasks
```

Tasks ที่เริ่ม: `SensorTask` · `CommandTask` · `NetworkTask` · `WebUiTask`

---

## 🧩 Hardware

### Board: ESP32-S3-WROOM-1 N16R8 (44-Pin)

| รายการ           | ค่า                         |
| ---------------- | --------------------------- |
| CPU              | Xtensa LX7 Dual-Core 240MHz |
| Flash            | 16 MB QIO                   |
| PSRAM            | 8 MB Octal (OPI)            |
| USB              | USB CDC Native (Type-C)     |
| Framework        | Arduino + FreeRTOS          |
| PlatformIO board | `esp32-s3-devkitc-1`        |

### GPIO Policy

บนโมดูล N16R8 (Octal PSRAM) **ไม่ใช้ GPIO26–37** เพื่อกันชน Flash/PSRAM

- **GPIO0**: strapping pin — ควรมี pull-up 10kΩ ไป 3.3V
- **GPIO19–20**: USB D−/D+ — ห้ามต่ออุปกรณ์ภายนอก
- **GPIO39**: input-only — ใช้เป็น net switch เท่านั้น

### Hardware Flags (`include/Config.h`)

| Flag                           | ความหมาย             |
| ------------------------------ | -------------------- |
| `RELAY_ACTIVE_LOW = true`      | IN=LOW → Relay ON    |
| `WATER_LEVEL_ALARM_LOW = true` | LOW = น้ำต่ำ (alarm) |
| `ALARM_LED_ACTIVE_HIGH = true` | HIGH = LED on        |

---

## 🧷 PIN Map — source of truth: `include/Config.h`

| หมวด               | Signal             |                  GPIO |
| ------------------ | ------------------ | --------------------: |
| **I2C**            | SDA / SCL          |             **8 / 9** |
| **1-Wire**         | DS18B20 DATA       |                 **4** |
| **Relay**          | Water / Mist / Air |      **38 / 40 / 41** |
| **Manual Switch**  | Pump / Mist / Air  |       **18 / 5 / 15** |
| **Mode Switch**    | A / B              |             **6 / 7** |
| **Network Switch** | AP/STA             | **39** _(input-only)_ |
| **Water Level**    | CH1 / CH2          |             **1 / 2** |
| **Alarm LED**      | CH1 / CH2          |           **47 / 10** |

---

## 🪛 Wiring Notes

### Switches — Active LOW + Pull-up

สวิตช์ทุกตัวใช้ external R10kΩ pull-up ร่วมกับ INPUT_PULLUP ภายใน (~45kΩ) ต่อขนานกัน

```
(1)[3V3] ──(1)[R10kΩ](2)──(2)[GPIO]
                                │
                           (2)[ขา1 / ขา2](3)──(3)[GND]
```

**BOM pull-up resistors:** R10kΩ 1/4W × 6 ตัว (GPIO6, GPIO7, GPIO18, GPIO5, GPIO15, GPIO39)

### Mode Switch — Rotary 3 ตำแหน่ง

| ตำแหน่ง | GPIO6 (A) | GPIO7 (B) | Mode   |
| ------- | --------- | --------- | ------ |
| บิดซ้าย | LOW       | HIGH      | AUTO   |
| กลาง    | HIGH      | HIGH      | IDLE   |
| บิดขวา  | HIGH      | LOW       | MANUAL |

### Net Switch — Toggle 2 ตำแหน่ง

| ตำแหน่ง | GPIO39 | Mode          |
| ------- | ------ | ------------- |
| บิดซ้าย | HIGH   | AP_PRIMARY    |
| บิดขวา  | LOW    | STA_PREFERRED |

### Manual Switches — Toggle 2 ตำแหน่ง

| ตำแหน่ง | GPIO | isOn()      |
| ------- | ---- | ----------- |
| บิดซ้าย | HIGH | false (OFF) |
| บิดขวา  | LOW  | true (ON)   |

### I2C (BH1750, SHT40, DS3231) — ต้องมี pull-up

```
[DEVICE SDA](1) ──(1)[GPIO8 (SDA)](1.1)──(1.1)[R4.7kΩ](1.2)──(1.2)[3V3]
[DEVICE SCL](2) ──(2)[GPIO9 (SCL)](2.1)──(2.1)[R4.7kΩ](2.2)──(2.2)[3V3]
```

R4.7kΩ **ค่อมครั้งเดียวที่ bus** — ไม่ต้องค่อมซ้ำถ้ามีหลาย device · **BOM:** R4.7kΩ × 2 ตัว

### DS18B20 (1-Wire) — ต้องมี pull-up

```
[DS18B20 DATA](3)──(3)[GPIO4](3.1)──(3.1)[R4.7kΩ](3.2)──(3.2)[3V3]
```

### Relay / SSR

- ขา Water=38, Mist=40, Air=41
- ขั้ว ON/OFF ปรับด้วย `RELAY_ACTIVE_LOW`
- **Boot glitch:** `AppBoot::initRelayPins()` set INACTIVE ก่อนสร้าง FreeRTOS task เสมอ

---

## ⚡ Quick Start

```sh
# Build
platformio run

# Upload firmware
platformio run -t upload

# Upload filesystem (schedule.json + Web UI)
pio run -t uploadfs

# Monitor Serial
pio device monitor
```

> **ESP32-S3 USB CDC:** ครั้งแรกอาจต้องกด **BOOT + RESET** เพื่อเข้า Download Mode

---

## 🌐 Network & Web Dashboard

ระบบ boot เป็น **AP mode เสมอ** (offline-first) แล้วค่อย connect STA ถ้ามี config

| รายการ       | ค่า                                            |
| ------------ | ---------------------------------------------- |
| AP SSID      | `SmartFarm-Setup`                              |
| AP IP        | `192.168.4.1`                                  |
| ตั้งค่า WiFi | `http://192.168.4.1/wifi`                      |
| Dashboard    | `http://192.168.4.1/` หรือ `http://device-ip/` |
| Status API   | `GET /api/status`                              |

เมื่อต่อ STA สำเร็จ → AP ยังเปิดอยู่ (AP+STA พร้อมกัน)  
เปลี่ยนเป็น STA: บิด Net Switch ไปทางขวา (GPIO39=LOW)

### API Response ตัวอย่าง

```json
{
   "mode": 1,
   "pump": 1,
   "mist": 0,
   "air": 0,
   "lux": 120.0,
   "luxValid": 1,
   "temp": 29.5,
   "tempValid": 1,
   "hum": 65.0,
   "humValid": 1,
   "staConnected": 1,
   "apIp": "192.168.4.1",
   "staIp": "192.168.1.50"
}
```

---

## 📟 Serial CLI

ทุกคำสั่งขึ้นต้นด้วย prefix **`sm`** (กำหนดใน `Config.h` → `CLI_PREFIX`)

```
CommandTask → CommandParser → CommandService → SharedState
```

| คำสั่ง                    | ผล                                    |
| ------------------------- | ------------------------------------- |
| `sm help`                 | แสดงคำสั่งทั้งหมด                     |
| `sm status`               | แสดงสถานะระบบ                         |
| `sm mode auto`            | mode → AUTO                           |
| `sm mode manual`          | mode → MANUAL                         |
| `sm mode idle`            | mode → IDLE                           |
| `sm relay pump on/off`    | สั่ง pump (MANUAL เท่านั้น)           |
| `sm relay mist on/off`    | สั่ง mist (MANUAL เท่านั้น)           |
| `sm relay air on/off`     | สั่ง air pump (MANUAL เท่านั้น)       |
| `sm clear`                | reset manual overrides                |
| `sm net on/off`           | เปิด/ปิด STA WiFi                     |
| `sm net ntp`              | sync เวลาจาก NTP (ต้อง STA connected) |
| `sm clock set HH:MM[:SS]` | ตั้งเวลา RTC                          |

---

## ⏰ Schedule System

ไฟล์ `/data/schedule.json` (LittleFS) — โหลดตอน boot ผ่าน `ScheduleStore`

```json
{
   "air": [
      { "start": "06:00", "end": "06:15" },
      { "start": "12:00", "end": "12:15" }
   ]
}
```

`ScheduledRelay` รับ `ISchedule` + `IActuator` แล้วสั่ง relay อัตโนมัติตามช่วงเวลา  
Air pump ควบคุมด้วย schedule — **ไม่ผ่าน FarmManager**

---

## 📂 Data Storage (LittleFS)

| ไฟล์             | เนื้อหา                         |
| ---------------- | ------------------------------- |
| `/wifi.json`     | WiFi SSID / password / hostname |
| `/schedule.json` | ตารางเวลา relay                 |
| `/www/*`         | Web Dashboard (HTML/CSS/JS)     |

---

## 📁 โครงสร้างโปรเจค

```
SmartFarm_POC/
├── include/
│   ├── Config.h                        ← PIN map + ค่าคงที่ทั้งหมด (source of truth)
│   ├── interfaces/
│   │   ├── Types.h                     ← SystemMode, SensorReading, ...
│   │   ├── IActuator.h / ISensor.h / IClock.h
│   │   ├── IModeSource.h / INetModeSource.h
│   │   └── INetwork.h / ISchedule.h / IUi.h
│   ├── domain/
│   │   ├── TimeWindow.h                ← struct ช่วงเวลา [start, end)
│   │   └── TimeSchedule.h              ← pure logic ไม่มี Arduino
│   ├── application/
│   │   ├── FarmModels.h                ← FarmInput / FarmDecision
│   │   ├── FarmManager.h               ← ตัดสินใจ pump + mist
│   │   ├── ScheduledRelay.h            ← เชื่อม ISchedule + IActuator
│   │   └── CommandService.h            ← business logic ของ CLI
│   ├── cli/
│   │   ├── CommandParser.h             ← tokenize + dispatch
│   │   └── CommandTable.h              ← CliCommand struct
│   ├── infrastructure/
│   │   ├── SystemContext.h             ← Object graph holder
│   │   ├── SharedState.h               ← Thread-safe state (FreeRTOS mutex)
│   │   ├── AppBoot.h / RtcClock.h
│   │   ├── ScheduleStore.h / WifiConfigStore.h
│   │   ├── Esp32WebUi.h / Esp32WiFiNetwork.h
│   │   └── NetTimeSync.h
│   ├── drivers/
│   │   ├── Esp32Relay.h / Esp32ManualSwitch.h / Esp32NetModeSwitch.h
│   │   ├── Esp32Bh1750Light.h / Esp32Sht40.h / Esp32Ds18b20.h
│   │   └── Esp32WaterLevelInput.h / RtcDs3231Time.h
│   └── tasks/
│       └── TaskEntrypoints.h / WebUiTask.h
├── src/
│   ├── main.cpp                        ← Composition Root
│   ├── domain/        TimeSchedule.cpp
│   ├── application/   FarmManager.cpp / ScheduledRelay.cpp / CommandService.cpp
│   ├── cli/           CommandParser.cpp
│   ├── drivers/
│   ├── infrastructure/ AppBoot.cpp / SharedState.cpp / ScheduleStore.cpp / ...
│   └── tasks/         SensorTasks.cpp / NetworkTask.cpp / CommandTask.cpp
├── data/
│   ├── schedule.json
│   └── www/
└── test/
    └── test_farmmanager.cpp
```

---

## 🔮 Roadmap

- [ ] Logic ปั๊มน้ำอัตโนมัติ (AUTO mode — `FarmManager.applyAuto()`)
- [ ] Web Dashboard: water temperature จาก DS18B20
- [ ] MQTT support
- [ ] OTA Update
- [ ] Data logging / export
- [ ] EC / pH sensor
- [ ] PLC port

---

## 📜 License

MIT
