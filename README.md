# 🌱 SmartFarm POC

> **⚠️ อยู่ระหว่างพัฒนา — ยังไม่เสร็จสมบูรณ์**
> โปรเจคนี้อยู่ในขั้น Proof of Concept ยังมีฟีเจอร์บางส่วนที่ยังไม่ได้ implement และโครงสร้างอาจเปลี่ยนแปลงได้โดยไม่แจ้งล่วงหน้า

โครงงานต้นแบบ SmartFarm ควบคุมระบบน้ำ/หมอก/อากาศในฟาร์มอัตโนมัติ  
ใช้ **ESP32-S3 + FreeRTOS** และออกแบบแบบ Clean Architecture เพื่อแยก business logic ออกจากฮาร์ดแวร์ให้ชัดเจน

---

## 🔄 Migration: ESP32 → ESP32-S3 (N16R8)

### บอร์ดเดิม vs บอร์ดใหม่

| รายการ | เดิม | ใหม่ |
|---|---|---|
| **บอร์ด** | ESP32 DOIT DEVKIT V1 | ESP32-S3-WROOM-1 N16R8 (44-Pin Type-C) |
| **CPU** | Xtensa LX6 Dual-Core 240MHz | Xtensa LX7 Dual-Core 240MHz |
| **Flash** | 4 MB | 16 MB QIO |
| **PSRAM** | ไม่มี / 4 MB SPI | 8 MB Octal (OPI) ในตัว |
| **USB** | USB-UART Bridge (CH340) | USB CDC Native (Type-C) |
| **Framework** | Arduino + FreeRTOS | Arduino + FreeRTOS |
| **platformio board** | `esp32doit-devkit-v1` | `esp32-s3-devkitc-1` |

### Policy: GPIO ที่ห้ามใช้ (ยึดตาม `include/Config.h`)

บนโมดูล **ESP32-S3-WROOM-1 N16R8 (Octal PSRAM)** โปรเจกต์นี้กำหนด policy ว่า **ไม่ใช้ GPIO26–37** เพื่อกันชน Flash/PSRAM และลดความเสี่ยงการบูต/แฟลชเพี้ยน

เพิ่มเติม:
- **GPIO0**: strapping pin (ควรมี pull-up 10kΩ ไป 3.3V)
- **GPIO19–20**: USB D−/D+ (ห้ามต่ออุปกรณ์ภายนอก)
- **GPIO39**: input-only (โปรเจกต์ใช้เป็น net switch เท่านั้น)

---

## 🧩 Hardware Flags (สำคัญ)

เพื่อรองรับหลายรุ่นโมดูล/การต่อวงจร โปรเจกต์มี flag ใน `include/Config.h` เพื่อ normalize พฤติกรรม (ห้าม hardcode ขั้วสัญญาณใน driver):

- `RELAY_ACTIVE_LOW`
  - `true`  = IN=LOW → Relay ON
  - `false` = IN=HIGH → Relay ON
- `WATER_LEVEL_ALARM_LOW`
  - `true`  = LOW  = alarm (น้ำต่ำ)
  - `false` = HIGH = alarm (น้ำต่ำ)
- `ALARM_LED_ACTIVE_HIGH`
  - `true`  = HIGH = LED on
  - `false` = LOW  = LED on

---

## 🧷 PIN Map (ESP32-S3 N16R8) — source of truth: `include/Config.h`

### สรุปขาที่ใช้จริง

| หมวด | Signal | GPIO |
|---|---|---:|
| **I2C** | SDA / SCL | **8 / 9** |
| **1-Wire** | DS18B20 DATA | **4** |
| **Relay** | Water / Mist / Air | **38 / 40 / 41** |
| **Manual Switch** | Pump / Mist / Air | **18 / 5 / 15** |
| **Mode Switch** | A / B | **6 / 7** |
| **Network Switch** | AP/STA | **39** *(input-only)* |
| **Water Level** | CH1 / CH2 | **1 / 2** |
| **Alarm LED** | CH1 / CH2 | **47 / 10** |

---

## 🪛 Wiring Notes (เอาข้อดีมารวมกัน)

### Switches (Manual/Mode/Net) — Active LOW + INPUT_PULLUP (default)
โปรเจกต์ใช้ **Active LOW + `INPUT_PULLUP` (pull-up ภายใน)** เป็นค่า default (สายสั้น/ตู้คอนโทรลทั่วไป)

> **กรณีสายยาว/มี noise:** สามารถ “เพิ่ม external pull-up 10kΩ” ได้ (แต่ไม่บังคับ)

**วงจร (ตัวเลือกเสริม เมื่ออยากเพิ่ม pull-up ภายนอก):**
```
3.3V ─── R10kΩ ─── GPIO
                     │
                  Switch / Toggle
                     │
                    GND
```

### I2C (BH1750, SHT40, DS3231) — ต้องมี pull-up (บังคับ)
- SDA=GPIO8, SCL=GPIO9
- ต้องมี **external pull-up 4.7kΩ–10kΩ ไป 3.3V** ที่ทั้ง SDA และ SCL  
- ถ้าลืม pull-up: I2C scan มัก “ไม่เจอ device” แม้ต่อสายถูก

### DS18B20 (1-Wire) — ต้องมี pull-up (บังคับ)
- DATA=GPIO4
- ต้องมี **external pull-up 4.7kΩ ไป 3.3V**
- สายยาว/หลายตัว: ปรับช่วง 2.2k–4.7k ตามผลทดสอบจริง (ไม่มีค่าตายตัว)

### Relay / SSR
- ขา: Water=38, Mist=40, Air=41
- ขั้ว ON/OFF ปรับได้ด้วย `RELAY_ACTIVE_LOW`
- **Boot glitch warning:** ถ้า active-low และขา floating ตอนบูต รีเลย์อาจ ON ชั่วคราว  
  → `AppBoot::initRelayPins()` ต้อง set “INACTIVE” ให้ขารีเลย์ก่อนสร้าง task

---

## 🔁 ตาราง PIN เปลี่ยน (ESP32 → ESP32-S3)

> หลักคิด: ระบุ “สาเหตุของขาเดิม” ที่ใช้ต่อไม่ได้/ชน policy แล้วค่อยชี้ไป “ขาใหม่”

| Signal | GPIO เดิม (ESP32) | GPIO ใหม่ (ESP32-S3) | เหตุผล |
|---|---:|---:|---|
| `PIN_I2C_SDA` | 21 | **8** | ย้ายให้เป็นคู่มาตรฐานกับ SCL |
| `PIN_I2C_SCL` | 22 | **9** | GPIO22 ไม่มีบน S3 |
| `PIN_RELAY_WATER_PUMP` | 25 | **38** | GPIO25 ไม่มีบน S3 |
| `PIN_RELAY_MIST` | 26 | **40** | ขาเดิม 26 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้) |
| `PIN_RELAY_AIR_PUMP` | 27 | **41** | ขาเดิม 27 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้) |
| `PIN_WATER_LEVEL_CH1_LOW_SENSOR` | 32 | **1** | ขาเดิม 32 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้) |
| `PIN_WATER_LEVEL_CH2_LOW_SENSOR` | 33 | **2** | ขาเดิม 33 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้) |
| `PIN_WATER_LEVEL_CH1_ALARM_LED` | 23 | **47** | GPIO23 ไม่มีบน S3 |
| `PIN_WATER_LEVEL_CH2_ALARM_LED` | 19 | **10** | ขาเดิม 19 ชน USB D− |
| `PIN_SW_MODE_A` | 34 | **6** | ย้ายจากขา input-only (บอร์ดเดิม) มาเป็น I/O ทั่วไป (อ่านง่าย/ใช้งานนิ่ง) |
| `PIN_SW_MODE_B` | 35 | **7** | เหตุผลเดียวกับ Mode A |
| `PIN_SW_NET` | 36 | **39** | ขาเดิม 36 อยู่ในกลุ่ม reserved 26–37 (โมดูลนี้) |
| `PIN_SW_FREE` | 39 | **42** | ย้ายสำรองไปขาที่เป็น I/O ทั่วไป |

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

## 📡 Network & Web UI

| รายการ | ค่า |
|---|---|
| Default mode | AP (hotspot) |
| AP SSID | `SmartFarm-Setup` |
| AP IP | `192.168.4.1` |
| ตั้งค่า WiFi | `http://192.168.4.1/wifi` |
| Dashboard | `http://192.168.4.1/` |
| API | `GET /api/status` |
| เปลี่ยนเป็น STA | `PIN_SW_NET = HIGH` |

เมื่อต่อ STA สำเร็จ → AP ยังเปิดอยู่ (AP+STA พร้อมกัน)

---

## 🚧 สิ่งที่ยังไม่เสร็จ

- [ ] Logic ปั๊มน้ำอัตโนมัติ (AUTO mode ยังไม่มี decision สำหรับ water pump)
- [ ] EC / pH sensor integration (วางแผนไว้ในอนาคต)
- [ ] Web Dashboard: แสดง water temperature จาก DS18B20 ยังไม่ครบ
- [ ] Unit test ครอบคลุมยังน้อย (มีแค่ `test_farmmanager.cpp`)
- [ ] OTA Update
- [ ] Data logging / export ข้อมูลเซนเซอร์ย้อนหลัง

---

## 📁 โครงสร้างโปรเจค

```
SmartFarm_POC/
├── include/
│   ├── Config.h                  ← PIN map + ค่าคงที่ทั้งหมด (source of truth)
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
- เลเยอร์ล่าง (Interfaces / Domain / Application) **ห้ามแตะ Arduino API**
- Relay สั่งได้เฉพาะใน `controlTask` เท่านั้น
- คำสั่งจาก Serial / Web ต้องผ่าน `SharedState` เสมอ (thread-safe)
