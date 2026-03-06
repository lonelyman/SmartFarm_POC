# 🌱 SmartFarm POC

> **⚠️ อยู่ระหว่างพัฒนา — ยังไม่เสร็จสมบูรณ์**
> โปรเจคนี้อยู่ในขั้น Proof of Concept ยังมีฟีเจอร์บางส่วนที่ยังไม่ได้ implement และโครงสร้างอาจเปลี่ยนแปลงได้โดยไม่แจ้งล่วงหน้า

โครงงานต้นแบบ SmartFarm ควบคุมระบบน้ำ/หมอก/อากาศในฟาร์มอัตโนมัติ  
ใช้ **ESP32-S3 + FreeRTOS** สถาปัตยกรรมแบบ Clean Architecture แยก business logic ออกจากฮาร์ดแวร์อย่างชัดเจน

---

## 🔄 Migration: ESP32 → ESP32-S3

### บอร์ดเดิม vs บอร์ดใหม่

| รายการ               | เดิม                        | ใหม่                                   |
| -------------------- | --------------------------- | -------------------------------------- |
| **บอร์ด**            | ESP32 DOIT DEVKIT V1        | ESP32-S3-WROOM-1 N16R8 (44-Pin Type-C) |
| **CPU**              | Xtensa LX6 Dual-Core 240MHz | Xtensa LX7 Dual-Core 240MHz            |
| **Flash**            | 4 MB                        | 16 MB QIO                              |
| **PSRAM**            | ไม่มี / 4 MB SPI            | 8 MB Octal (OPI) ในตัว                 |
| **USB**              | USB-UART Bridge (CH340)     | USB CDC Native (Type-C)                |
| **Framework**        | Arduino + FreeRTOS          | Arduino + FreeRTOS (เหมือนเดิม)        |
| **platformio board** | `esp32doit-devkit-v1`       | `esp32-s3-devkitc-1`                   |

### สาเหตุที่ต้องเปลี่ยน PIN

ESP32-S3 N16R8 มี **Octal PSRAM 8MB** ฝังใน module → GPIO26–37 ถูก reserve ไว้ทั้งหมด  
Pin เดิมหลายตัวที่ใช้บน ESP32 ตกทับในกลุ่มนี้พอดี จึงต้องย้ายทุกตัว

### ตาราง PIN เปลี่ยน

| Signal                           | GPIO เดิม (ESP32) | GPIO ใหม่ (ESP32-S3) | เหตุผล                                                                         |
| -------------------------------- | ----------------- | -------------------- | ------------------------------------------------------------------------------ |
| `PIN_I2C_SDA`                    | 21                | **8**                | เปลี่ยนให้เป็นคู่ติดกับ SCL=9                                                  |
| `PIN_I2C_SCL`                    | 22                | **9**                | GPIO22 ไม่มีบน S3                                                              |
| `PIN_RELAY_WATER_PUMP`           | 25                | **38**               | GPIO25 ไม่มีบน S3                                                              |
| `PIN_RELAY_MIST`                 | 26                | **40**               | ขาเดิม 26 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้)                                |
| `PIN_RELAY_AIR_PUMP`             | 27                | **41**               | ขาเดิม 27 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้)                                |
| `PIN_WATER_LEVEL_CH1_LOW_SENSOR` | 32                | **1**                | ขาเดิม 32 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้)                                |
| `PIN_WATER_LEVEL_CH2_LOW_SENSOR` | 33                | **2**                | ขาเดิม 33 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้)                                |
| `PIN_WATER_LEVEL_CH1_ALARM_LED`  | 23                | **47**               | GPIO23 ไม่มีบน S3                                                              |
| `PIN_WATER_LEVEL_CH2_ALARM_LED`  | 19                | **10**               | GPIO19 ชน USB D−                                                               |
| `PIN_SW_MODE_A`                  | 34                | **6**                | ย้ายจากขา input-only (บอร์ดเดิม) มาเป็น I/O ทั่วไปบน S3 (อ่านง่าย/เดินสายง่าย) |
| `PIN_SW_MODE_B`                  | 35                | **7**                | เหตุผลเดียวกับ Mode A                                                          |
| `PIN_SW_NET`                     | 36                | **39**               | ขาเดิม 36 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้)                                |
| `PIN_SW_FREE`                    | 39                | **42**               | ย้ายสำรองไปขาที่เป็น I/O ทั่วไป                                                |

**PIN ที่ไม่ต้องเปลี่ยน:** GPIO4 (1-Wire), GPIO5, GPIO15, GPIO18

---

## 🚧 สิ่งที่ยังไม่เสร็จ

- [ ] Logic ปั๊มน้ำอัตโนมัติ (AUTO mode ยังไม่มี decision สำหรับ water pump)
- [ ] EC / pH sensor integration (วางแผนไว้ในอนาคต)
- [ ] Web Dashboard: แสดง water temperature จาก DS18B20 ยังไม่ครบ
- [ ] Unit test ครอบคลุมยังน้อย (มีแค่ `test_farmmanager.cpp`)
- [ ] OTA Update
- [ ] Data logging / export ข้อมูลเซนเซอร์ย้อนหลัง

---

## ⚡ Quick Start

```sh
# 1. Build
platformio run

# 2. Upload firmware
platformio run -t upload

# 3. Upload filesystem (schedule.json + Web UI)
pio run -t uploadfs

# 4. Monitor Serial
pio device monitor
```

> **ESP32-S3 USB CDC:** ครั้งแรกอาจต้องกด **BOOT + RESET** เพื่อเข้า Download Mode  
> หรือเปิด "Erase Flash" ก่อน upload ครั้งแรก

---

## 📌 GPIO Pin Map (ESP32-S3)

### Wiring Notes

#### I2C (BH1750, SHT40, DS3231)

- SDA=GPIO8, SCL=GPIO9
- ต้องมี **external pull-up 4.7kΩ–10kΩ ไป 3.3V** ที่ทั้ง SDA และ SCL
- ถ้าลืม pull-up: I2C scan มัก "ไม่เจอ device" แม้ต่อสายถูก

#### DS18B20 (1-Wire)

- DATA=GPIO4
- ต้องมี **external pull-up 4.7kΩ ไป 3.3V** ที่ DATA
- สายยาว/หลายตัว: ปรับช่วง 2.2k–4.7k ตามผลทดสอบจริง (ไม่มีค่าตายตัว)

#### Switches (Manual/Mode/Net)

- default: ทุกตัวเป็น **Active LOW** และใช้ `INPUT_PULLUP` (pull-up ภายใน ESP32-S3)
- optional (สายยาว/มี noise): สามารถเพิ่ม **external pull-up 10kΩ** ได้

**วงจร (optional):**

```
3.3V ─── R10kΩ ─── GPIO
                     │
                  Switch / Toggle
                     │
                    GND
```

- Net switch (GPIO39) ใช้เป็น **input เท่านั้น** ห้ามเอาไปทำ output

#### Relay / SSR

- ขา: Water=38, Mist=40, Air=41
- ขั้วสัญญาณ ON/OFF ปรับได้ด้วย `RELAY_ACTIVE_LOW`
- **Boot glitch warning:** ถ้า active-low และขา floating ตอนบูต รีเลย์อาจ ON ชั่วคราว  
  → ต้อง init ขารีเลย์ให้เป็น "INACTIVE" ตั้งแต่ `AppBoot::initRelayPins()` ก่อนสร้าง task

### ตาราง GPIO ทั้งหมด

| GPIO | ประเภท        | ใช้เป็น                          | หมายเหตุ                                  |
| ---- | ------------- | -------------------------------- | ----------------------------------------- |
| 1    | I/O ✅        | WATER_LEVEL_CH1_SENSOR (XKC-Y25) | Input — ขึ้นกับ `WATER_LEVEL_ALARM_LOW`   |
| 2    | I/O ✅        | WATER_LEVEL_CH2_SENSOR (XKC-Y25) | Input — ขึ้นกับ `WATER_LEVEL_ALARM_LOW`   |
| 4    | I/O ✅        | DS18B20 (1-Wire DATA)            | ผ่าน Adapter Module — pull-up 4.7kΩ       |
| 5    | I/O ✅        | SW_MANUAL_MIST                   | Active LOW + INPUT_PULLUP                 |
| 6    | I/O ✅        | SW_MODE_A                        | Active LOW + INPUT_PULLUP                 |
| 7    | I/O ✅        | SW_MODE_B                        | Active LOW + INPUT_PULLUP                 |
| 8    | I/O ✅        | I2C SDA                          | BH1750, SHT40, DS3231 — pull-up 4.7kΩ     |
| 9    | I/O ✅        | I2C SCL                          | BH1750, SHT40, DS3231 — pull-up 4.7kΩ     |
| 10   | I/O ✅        | WATER_LEVEL_CH2_ALARM_LED        | Output — ขึ้นกับ `ALARM_LED_ACTIVE_HIGH`  |
| 15   | I/O ✅        | SW_MANUAL_AIR                    | Active LOW + INPUT_PULLUP                 |
| 18   | I/O ✅        | SW_MANUAL_PUMP                   | Active LOW + INPUT_PULLUP                 |
| 38   | I/O ✅        | RELAY_WATER_PUMP                 | Output — ขึ้นกับ `RELAY_ACTIVE_LOW`       |
| 39   | Input only ✅ | SW_NET (AP/STA)                  | HIGH=STA, LOW=AP — ใช้เป็น input เท่านั้น |
| 40   | I/O ✅        | RELAY_MIST                       | Output — ขึ้นกับ `RELAY_ACTIVE_LOW`       |
| 41   | I/O ✅        | RELAY_AIR_PUMP                   | Output — ขึ้นกับ `RELAY_ACTIVE_LOW`       |
| 42   | I/O ✅        | PIN_SW_FREE (สำรอง)              | ใช้ได้ทั้ง I/O                            |
| 47   | I/O ✅        | WATER_LEVEL_CH1_ALARM_LED        | Output — ขึ้นกับ `ALARM_LED_ACTIVE_HIGH`  |

---

## 🏗️ สถาปัตยกรรม (Clean Architecture 6 Layer)

เป้าหมาย: **แยก business logic ออกจากฮาร์ดแวร์** — พอร์ตง่าย ทดสอบได้

```
┌─────────────────────────────────────┐
│  Tasks / Main (FreeRTOS wiring)     │  ← src/tasks, main.cpp
├─────────────────────────────────────┤
│  Infrastructure (State, Network)    │  ← SharedState, AppBoot, WiFi
├─────────────────────────────────────┤
│  Drivers (Hardware Adapters)        │  ← BH1750, SHT40, Relay, Switch
├─────────────────────────────────────┤
│  Application (FarmManager)          │  ← ตัดสินใจ business logic
├─────────────────────────────────────┤
│  Domain (Models, Schedule)          │  ← AirPumpSchedule, FarmInput
├─────────────────────────────────────┤
│  Interfaces (Contracts)             │  ← ISensor, IActuator, IClock
└─────────────────────────────────────┘
```

**กฎสำคัญ:**

- เลเยอร์ล่าง (Interfaces / Domain / Application) **ห้ามแตะ Arduino API** ทุกกรณี
- Relay สั่งได้เฉพาะใน `controlTask` เท่านั้น
- คำสั่งจาก Serial / Web ต้องผ่าน `SharedState` เสมอ (thread-safe)

---

## ⚙️ Runtime Modes

### IDLE

- รีเลย์ทุกตัวปิด ล้าง override ทั้งหมด reset boot guard และ mist guard

### AUTO

- **Boot Guard** 10 วินาที — หน่วงก่อนเริ่มทำงานหลัง boot
- **หมอก** — ควบคุมด้วย temp + humidity hysteresis (ดู Mist Logic)
- **ปั๊มลม** — ตาม schedule.json บน LittleFS
- **ปั๊มน้ำ** — 🚧 ยังไม่มี logic อัตโนมัติ
- Manual switch ไม่มีผล

### MANUAL

- ปิดรีเลย์ทั้งหมดเมื่อเข้าโหมด
- สั่งผ่าน Serial, Web UI, หรือสวิตช์ทางกายภาพ (GPIO 5, 15, 18)
- สวิตช์กายภาพ: **กดค้าง = เปิด, ปล่อย = ปิด**

---

## 💧 Mist Logic (AUTO mode)

### Priority ในการตัดสินใจ

1. ทั้ง temp และ humidity valid → `decideMistByTempAndHumidity()`
2. มีแค่ humidity → `decideMistByHumidity()` (fallback)
3. มีแค่ temp → `decideMistByTemp()` (fallback)

### Hysteresis (ปรับใน `Config.h`)

```cpp
HYSTERESIS_TEMP_ON  = 32.0f   // เปิดเมื่อ temp ≥ 32°C
HYSTERESIS_TEMP_OFF = 29.0f   // ปิดเมื่อ temp ≤ 29°C

HYSTERESIS_HUMIDITY_ON  = 65.0f  // เปิดพ่นเมื่อ humidity < 65%
HYSTERESIS_HUMIDITY_OFF = 75.0f  // ปิดพ่นเมื่อ humidity > 75%
```

| Temp    | Humidity | ผล                           |
| ------- | -------- | ---------------------------- |
| ≥ 32°C  | ≤ 65%    | ✅ เปิดพ่น                   |
| ≥ 32°C  | ≥ 75%    | ❌ ปิด (ชื้นพอแล้ว)          |
| ≤ 29°C  | ใดก็ได้  | ❌ ปิด (ไม่ร้อน)             |
| ระหว่าง | ระหว่าง  | ⏸️ คง state เดิม (dead-band) |

### Time Guard

```cpp
MIST_MAX_ON_MS  = 3 นาที   // พ่นได้สูงสุดต่อครั้ง
MIST_MIN_OFF_MS = 5 นาที   // พักขั้นต่ำก่อนพ่นครั้งถัดไป
```

---

## 🕒 Time Source (DS3231 RTC)

- RTC DS3231 เป็นแหล่งเวลาเดียว ผ่าน `IClock` adapter
- หาก RTC ไม่ตอบสนอง → schedule ปิดทำงาน ControlTask แจ้ง warning
- NTP Sync: **ไม่ auto** — ต้องสั่งเองผ่าน Serial (`time=HH:MM`) หรือ Web UI
- NTP sync ทำได้เมื่ออยู่ใน STA mode และเชื่อมต่อ WiFi แล้วเท่านั้น

### Air Pump Schedule (`data/schedule.json`)

```json
{
   "air_pump": {
      "enabled": true,
      "windows": [
         { "start": "07:00", "end": "12:00" },
         { "start": "14:00", "end": "17:30" }
      ]
   }
}
```

---

## 📡 Network & Web UI

| รายการ          | ค่า                       |
| --------------- | ------------------------- |
| Default mode    | AP (hotspot)              |
| AP SSID         | `SmartFarm-Setup`         |
| AP IP           | `192.168.4.1`             |
| ตั้งค่า WiFi    | `http://192.168.4.1/wifi` |
| Dashboard       | `http://192.168.4.1/`     |
| API             | `GET /api/status`         |
| เปลี่ยนเป็น STA | PIN_SW_NET = HIGH         |

เมื่อต่อ STA สำเร็จ → AP ยังเปิดอยู่ (AP+STA พร้อมกัน)

---

## 🖥️ Serial Commands

**Baud:** 115200

| คำสั่ง            | ผล                              |
| ----------------- | ------------------------------- |
| `-auto` / `--a`   | เข้าโหมด AUTO                   |
| `-manual` / `--m` | เข้าโหมด MANUAL                 |
| `-idle` / `--i`   | เข้าโหมด IDLE                   |
| `-pump on/off`    | สั่ง pump (ต้อง MANUAL)         |
| `-mist on/off`    | สั่ง mist (ต้อง MANUAL)         |
| `-air on/off`     | สั่ง air pump (ต้อง MANUAL)     |
| `-clear`          | ปิดทุก relay (ไม่เปลี่ยนโหมด)   |
| `time=HH:MM[:SS]` | ตั้งเวลา RTC + request NTP sync |
| `-help`           | แสดงคำสั่งทั้งหมด               |

---

## 🔁 Runtime Flow

```
AppBoot → init I2C → LittleFS → drivers → clock → network → IDLE → start tasks
         │
         ├── InputTask   (1s)   อ่าน BH1750, SHT40, WaterLevel, Switches → SharedState
         ├── ControlTask (1s)   อ่านเวลา → FarmManager.update() → สั่ง Relay
         ├── CommandTask (50ms) รับ Serial → parse → แก้ SharedState
         ├── NetworkTask (20ms) WiFi state machine + net switch debounce
         └── WebUiTask          HTTP server → dashboard + /api/status
```

---

## 📁 โครงสร้างโปรเจค

```
SmartFarm_POC/
├── include/
│   ├── Config.h                  ← PIN map + ค่าคงที่ทั้งหมด
│   ├── interfaces/               ← ISensor, IActuator, IClock, Types
│   ├── domain/                   ← AirPumpSchedule, FarmModels
│   ├── application/              ← FarmManager (brain)
│   ├── infrastructure/           ← SharedState, SystemContext, AppBoot
│   ├── drivers/                  ← BH1750, SHT40, DS3231, Relay, Switch
│   └── tasks/                    ← Task entry points
├── src/
│   ├── main.cpp                  ← Composition Root
│   ├── drivers/
│   ├── tasks/                    ← inputTask, controlTask, commandTask, networkTask
│   ├── application/
│   ├── infrastructure/
│   └── domain/
├── data/
│   ├── schedule.json             ← ตารางเวลาปั๊มลม (LittleFS)
│   └── www/                      ← Web UI (dashboard, wifi)
├── test/
│   └── test_farmmanager.cpp
└── platformio.ini
```

---

## 🛠️ Hardware Summary

| ชิป/อุปกรณ์      | Interface                    | หน้าที่                |
| ---------------- | ---------------------------- | ---------------------- |
| BH1750           | I2C (SDA=8, SCL=9)           | วัดแสง (lux)           |
| SHT40            | I2C (SDA=8, SCL=9) addr 0x44 | อุณหภูมิ + ความชื้น    |
| DS3231           | I2C (SDA=8, SCL=9)           | RTC นาฬิกา             |
| DS18B20          | 1-Wire (GPIO4)               | อุณหภูมิน้ำ            |
| XKC-Y25          | Digital (GPIO1, 2)           | ระดับน้ำ (non-contact) |
| SSR/Relay Module | Digital (GPIO38, 40, 41)     | ควบคุม pump/mist/air   |

---

## 📝 Debug Flags (`Config.h`)

```cpp
#define DEBUG_BH1750_LOG      1   // เซนเซอร์แสง
#define DEBUG_TEMP_LOG        0   // อุณหภูมิ / ความชื้น
#define DEBUG_EC_LOG          0   // EC (อนาคต)
#define DEBUG_TIME_LOG        1   // เวลา RTC
#define DEBUG_WATER_LEVEL_LOG 1   // Water Level
#define DEBUG_CONTROL_LOG     1   // ControlTask
```

---

> **⚠️ Reminder:** กฎแยกเลเยอร์ต้องรักษาไว้เสมอ — Application/Domain ห้ามแตะ Arduino API และ relay ถูกสั่งได้เฉพาะใน `controlTask` เท่านั้น
