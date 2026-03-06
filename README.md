# 🌱 SmartFarm POC

> **⚠️ อยู่ระหว่างพัฒนา — ยังไม่เสร็จสมบูรณ์**  
> โปรเจคนี้อยู่ในขั้น Proof of Concept ยังมีฟีเจอร์บางส่วนที่ยังไม่ได้ implement และโครงสร้างอาจเปลี่ยนแปลงได้โดยไม่แจ้งล่วงหน้า

โครงงานต้นแบบ SmartFarm ควบคุมระบบน้ำ/หมอก/อากาศในฟาร์มอัตโนมัติ  
ใช้ **ESP32-S3 + FreeRTOS** และออกแบบแบบ Clean Architecture เพื่อแยก business logic ออกจากฮาร์ดแวร์ให้ชัดเจน

---

## 🔄 Migration: ESP32 → ESP32-S3 (N16R8)

### บอร์ดเดิม vs บอร์ดใหม่

| รายการ               | เดิม                        | ใหม่                                   |
| -------------------- | --------------------------- | -------------------------------------- |
| **บอร์ด**            | ESP32 DOIT DEVKIT V1        | ESP32-S3-WROOM-1 N16R8 (44-Pin Type-C) |
| **CPU**              | Xtensa LX6 Dual-Core 240MHz | Xtensa LX7 Dual-Core 240MHz            |
| **Flash**            | 4 MB                        | 16 MB QIO                              |
| **PSRAM**            | ไม่มี / 4 MB SPI            | 8 MB Octal (OPI) ในตัว                 |
| **USB**              | USB-UART Bridge (CH340)     | USB CDC Native (Type-C)                |
| **Framework**        | Arduino + FreeRTOS          | Arduino + FreeRTOS                     |
| **platformio board** | `esp32doit-devkit-v1`       | `esp32-s3-devkitc-1`                   |

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
   - `true` = IN=LOW → Relay ON
   - `false` = IN=HIGH → Relay ON
- `WATER_LEVEL_ALARM_LOW`
   - `true` = LOW = alarm (น้ำต่ำ)
   - `false` = HIGH = alarm (น้ำต่ำ)
- `ALARM_LED_ACTIVE_HIGH`
   - `true` = HIGH = LED on
   - `false` = LOW = LED on

---

## 🧷 PIN Map (ESP32-S3 N16R8) — source of truth: `include/Config.h`

### สรุปขาที่ใช้จริง

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

### Switches (Manual / Mode / Net) — Active LOW

สวิตช์ทุกตัวในโปรเจกต์นี้เป็น **toggle switch แบบบิด Active LOW**  
ใช้ **external R10kΩ pull-up ร่วมกับ INPUT_PULLUP** ภายใน ESP32-S3 (~45kΩ) ต่อขนานกัน  
เพื่อให้ทนต่อ noise ในตู้ควบคุม (relay / motor / pump)

**วงจรต่อขา (ตัวอย่าง — ใช้เหมือนกันทุกตัว):**

```
(1)[3V3] ──(1)[R10kΩ](2)──(2)[GPIO]
                                │
                           (2)[ขา1 / ขา2](3)──(3)[GND]
```

> จุด (2) คือจุดร่วมของ R10kΩ + GPIO + ขา 1 ของสวิตช์

**BOM pull-up resistors:**

| Switch      | GPIO | R     |
| ----------- | ---- | ----- |
| Manual Pump | 18   | R10kΩ |
| Manual Mist | 5    | R10kΩ |
| Manual Air  | 15   | R10kΩ |
| Mode A      | 6    | R10kΩ |
| Mode B      | 7    | R10kΩ |
| Net Switch  | 39   | R10kΩ |

รวม **R10kΩ 1/4W × 6 ตัว**

---

### Mode Switch — Rotary 3 ตำแหน่ง (4 ขา / 2 วงจรอิสระ)

```
ตำแหน่ง    GPIO6 (A)   GPIO7 (B)   Mode
─────────────────────────────────────────
บิดซ้าย    LOW         HIGH        AUTO
กลาง       HIGH        HIGH        IDLE
บิดขวา     HIGH        LOW         MANUAL
```

ใช้คู่ขาซ้าย (A1/A2) กับ GPIO6 และคู่ขวา (B3/B4) กับ GPIO7

---

### Net Switch — Toggle 2 ตำแหน่ง (4 ขา / ใช้คู่ซ้ายเท่านั้น)

```
ตำแหน่ง    GPIO39      Mode
────────────────────────────
บิดซ้าย    HIGH        AP_PRIMARY
บิดขวา     LOW         STA_PREFERRED
```

คู่ขวา (ขา 3-4) ไม่ต้องต่อ — ปล่อยว่าง

---

### Manual Switches — Toggle 2 ตำแหน่ง (4 ขา / ใช้คู่ซ้ายเท่านั้น)

```
ตำแหน่ง    GPIO        isOn()
────────────────────────────────
บิดซ้าย    HIGH        false (OFF)
บิดขวา     LOW         true  (ON)
```

---

### I2C (BH1750, SHT40, DS3231) — ต้องมี pull-up (บังคับ)

- SDA=GPIO8, SCL=GPIO9
- ต้องมี **external pull-up 4.7kΩ–10kΩ ไป 3.3V** ที่ทั้ง SDA และ SCL
- ถ้าลืม pull-up: I2C scan มัก "ไม่เจอ device" แม้ต่อสายถูก

---

### DS18B20 (1-Wire) — ต้องมี pull-up (บังคับ)

- DATA=GPIO4
- ต้องมี **external pull-up 4.7kΩ ไป 3.3V**
- สายยาว/หลายตัว: ปรับช่วง 2.2k–4.7k ตามผลทดสอบจริง

---

### Relay / SSR

- ขา: Water=38, Mist=40, Air=41
- ขั้ว ON/OFF ปรับได้ด้วย `RELAY_ACTIVE_LOW`
- **Boot glitch warning:** ถ้า active-low และขา floating ตอนบูต รีเลย์อาจ ON ชั่วคราว  
  → `AppBoot::initRelayPins()` set "INACTIVE" ให้ขารีเลย์ก่อนสร้าง FreeRTOS task เสมอ

---

## 🔁 ตาราง PIN เปลี่ยน (ESP32 → ESP32-S3)

| Signal                           | GPIO เดิม (ESP32) | GPIO ใหม่ (ESP32-S3) | เหตุผล                               |
| -------------------------------- | ----------------: | -------------------: | ------------------------------------ |
| `PIN_I2C_SDA`                    |                21 |                **8** | ย้ายให้เป็นคู่มาตรฐานกับ SCL         |
| `PIN_I2C_SCL`                    |                22 |                **9** | GPIO22 ไม่มีบน S3                    |
| `PIN_RELAY_WATER_PUMP`           |                25 |               **38** | GPIO25 ไม่มีบน S3                    |
| `PIN_RELAY_MIST`                 |                26 |               **40** | อยู่ในกลุ่ม reserved 26–37           |
| `PIN_RELAY_AIR_PUMP`             |                27 |               **41** | อยู่ในกลุ่ม reserved 26–37           |
| `PIN_WATER_LEVEL_CH1_LOW_SENSOR` |                32 |                **1** | อยู่ในกลุ่ม reserved 26–37           |
| `PIN_WATER_LEVEL_CH2_LOW_SENSOR` |                33 |                **2** | อยู่ในกลุ่ม reserved 26–37           |
| `PIN_WATER_LEVEL_CH1_ALARM_LED`  |                23 |               **47** | GPIO23 ไม่มีบน S3                    |
| `PIN_WATER_LEVEL_CH2_ALARM_LED`  |                19 |               **10** | ขาเดิมชน USB D−                      |
| `PIN_SW_MODE_A`                  |                34 |                **6** | ย้ายจาก input-only มาเป็น I/O ทั่วไป |
| `PIN_SW_MODE_B`                  |                35 |                **7** | เหตุผลเดียวกับ Mode A                |
| `PIN_SW_NET`                     |                36 |               **39** | อยู่ในกลุ่ม reserved 26–37           |
| `PIN_SW_FREE`                    |                39 |               **42** | ย้ายสำรองไปขาที่เป็น I/O ทั่วไป      |

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

| รายการ          | ค่า                                  |
| --------------- | ------------------------------------ |
| Default mode    | AP (hotspot)                         |
| AP SSID         | `SmartFarm-Setup`                    |
| AP IP           | `192.168.4.1`                        |
| ตั้งค่า WiFi    | `http://192.168.4.1/wifi`            |
| Dashboard       | `http://192.168.4.1/`                |
| API             | `GET /api/status`                    |
| เปลี่ยนเป็น STA | บิด Net Switch ไปทางขวา (GPIO39=LOW) |

เมื่อต่อ STA สำเร็จ → AP ยังเปิดอยู่ (AP+STA พร้อมกัน)

---

## 💻 Serial Commands

เชื่อมต่อ Serial Monitor ที่ 115200 baud แล้วพิมพ์คำสั่ง:

| คำสั่ง               | ผล                              |
| -------------------- | ------------------------------- |
| `-auto` หรือ `--a`   | เปลี่ยน mode เป็น AUTO          |
| `-manual` หรือ `--m` | เปลี่ยน mode เป็น MANUAL        |
| `-idle` หรือ `--i`   | เปลี่ยน mode เป็น IDLE          |
| `-pump on/off`       | สั่ง pump (MANUAL เท่านั้น)     |
| `-mist on/off`       | สั่ง mist (MANUAL เท่านั้น)     |
| `-air on/off`        | สั่ง air pump (MANUAL เท่านั้น) |
| `-clear`             | reset manual overrides          |
| `-net on/off`        | เปิด/ปิด STA WiFi               |
| `time=HH:MM[:SS]`    | ตั้งเวลา RTC                    |
| `-help`              | แสดงคำสั่งทั้งหมด               |

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
│   ├── interfaces/               ← ISensor, IActuator, IClock, IModeSource, Types
│   ├── domain/                   ← AirPumpSchedule, FarmModels
│   ├── application/              ← FarmManager (brain)
│   ├── infrastructure/           ← SharedState, SystemContext, AppBoot
│   ├── drivers/                  ← BH1750, SHT40, DS3231, Relay, Switch
│   └── tasks/                    ← Task entry points
├── src/
│   ├── main.cpp                  ← Composition Root
│   ├── drivers/
│   ├── tasks/                    ← inputTask, controlTask, commandTask, networkTask, webUiTask
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

---

## ✅ สถานะการ Review Code (อ้างอิง SystemContext)

| Field ใน SystemContext                      | ไฟล์                                                                     |                 สถานะ                 |
| ------------------------------------------- | ------------------------------------------------------------------------ | :-----------------------------------: |
| `state` — SharedState                       | `SharedState.h/.cpp`                                                     |           ยังไม่ได้ review            |
| `ui` — IUi / Esp32WebUi                     | `Esp32WebUi.h/.cpp`                                                      |              ✅ reviewed              |
| `airSchedule` — AirPumpSchedule             | `AirPumpSchedule.h/.cpp`                                                 |           ยังไม่ได้ review            |
| `lightSensor` — Esp32Bh1750Light            | `Esp32Bh1750Light.h/.cpp`                                                |           ยังไม่ได้ review            |
| `tempSensor` — Esp32Sht40                   | `Esp32Sht40.h/.cpp`                                                      |           ยังไม่ได้ review            |
| `waterLevelInput` — Esp32WaterLevelInput    | `Esp32WaterLevelInput.h`                                                 |           ยังไม่ได้ review            |
| `waterTempSensor` — Esp32Ds18b20            | `Esp32Ds18b20.h/.cpp`                                                    |           ยังไม่ได้ review            |
| `waterPump/mistSystem/airPump` — Esp32Relay | `Esp32Relay.h/.cpp`                                                      |           ยังไม่ได้ review            |
| `swManualPump/Mist/Air` — Esp32ManualSwitch | `Esp32ManualSwitch.h/.cpp`                                               |              ✅ reviewed              |
| `modeSource` — Esp32ModeSwitchSource        | `Esp32ModeSwitchSource.h/.cpp`                                           |              ✅ reviewed              |
| `netModeSource` — Esp32NetModeSwitch        | `Esp32NetModeSwitch.h/.cpp`                                              |              ✅ reviewed              |
| `clock` — RtcClock / RtcDs3231Time          | `RtcClock.h`, `RtcDs3231Time.h/.cpp`                                     |           ยังไม่ได้ review            |
| `network` — Esp32WiFiNetwork                | `Esp32WiFiNetwork.h/.cpp`                                                |           ยังไม่ได้ review            |
| `manager` — FarmManager                     | `FarmManager.h/.cpp`                                                     |           ยังไม่ได้ review            |
| Tasks                                       | `SensorTasks.cpp`, `NetworkTask.cpp`, `CommandTask.cpp`, `WebUiTask.cpp` | ✅ reviewed (SensorTasks) / ยังไม่ครบ |
| Infrastructure                              | `AppBoot.cpp`, `SystemContext.h`, `main.cpp`                             |              ✅ reviewed              |
