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

- `RELAY_ACTIVE_LOW` — `true` = IN=LOW → Relay ON
- `WATER_LEVEL_ALARM_LOW` — `true` = LOW = น้ำต่ำ (alarm)
- `ALARM_LED_ACTIVE_HIGH` — `true` = HIGH = LED on

---

## 🧷 PIN Map (ESP32-S3 N16R8) — source of truth: `include/Config.h`

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

สวิตช์ทุกตัวใช้ **external R10kΩ pull-up ร่วมกับ INPUT_PULLUP** ภายใน ESP32-S3 (~45kΩ) ต่อขนานกัน

```
(1)[3V3] ──(1)[R10kΩ](2)──(2)[GPIO]
                                │
                           (2)[ขา1 / ขา2](3)──(3)[GND]
```

**BOM pull-up resistors:** R10kΩ 1/4W × 6 ตัว (GPIO6, GPIO7, GPIO18, GPIO5, GPIO15, GPIO39)

---

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

---

### I2C (BH1750, SHT40, DS3231) — ต้องมี pull-up (บังคับ)

```
[DEVICE SDA](1) ──(1)[GPIO8 (SDA)](1.1)──(1.1)[R4.7kΩ](1.2)──(1.2)[3V3]
[DEVICE SCL](2) ──(2)[GPIO9 (SCL)](2.1)──(2.1)[R4.7kΩ](2.2)──(2.2)[3V3]
```

**R4.7kΩ ค่อมครั้งเดียวที่ bus** — ไม่ต้องค่อมซ้ำถ้ามีหลาย device  
**BOM:** R4.7kΩ 1/4W × 2 ตัว

### DS18B20 (1-Wire) — ต้องมี pull-up (บังคับ)

```
[DS18B20 DATA](3)──(3)[GPIO4](3.1)──(3.1)[R4.7kΩ](3.2)──(3.2)[3V3]
```

สายยาว/หลายตัว: ปรับช่วง 2.2kΩ–4.7kΩ ตามผลทดสอบจริง

### Relay / SSR

- ขา: Water=38, Mist=40, Air=41
- ขั้ว ON/OFF ปรับได้ด้วย `RELAY_ACTIVE_LOW`
- **Boot glitch warning:** `AppBoot::initRelayPins()` set INACTIVE ก่อนสร้าง FreeRTOS task เสมอ

---

## 🔁 ตาราง PIN เปลี่ยน (ESP32 → ESP32-S3)

| Signal                           | GPIO เดิม | GPIO ใหม่ | เหตุผล                 |
| -------------------------------- | --------: | --------: | ---------------------- |
| `PIN_I2C_SDA`                    |        21 |     **8** | ย้ายให้เป็นคู่มาตรฐาน  |
| `PIN_I2C_SCL`                    |        22 |     **9** | GPIO22 ไม่มีบน S3      |
| `PIN_RELAY_WATER_PUMP`           |        25 |    **38** | GPIO25 ไม่มีบน S3      |
| `PIN_RELAY_MIST`                 |        26 |    **40** | reserved 26–37         |
| `PIN_RELAY_AIR_PUMP`             |        27 |    **41** | reserved 26–37         |
| `PIN_WATER_LEVEL_CH1_LOW_SENSOR` |        32 |     **1** | reserved 26–37         |
| `PIN_WATER_LEVEL_CH2_LOW_SENSOR` |        33 |     **2** | reserved 26–37         |
| `PIN_WATER_LEVEL_CH1_ALARM_LED`  |        23 |    **47** | GPIO23 ไม่มีบน S3      |
| `PIN_WATER_LEVEL_CH2_ALARM_LED`  |        19 |    **10** | ชน USB D−              |
| `PIN_SW_MODE_A`                  |        34 |     **6** | ย้ายจาก input-only     |
| `PIN_SW_MODE_B`                  |        35 |     **7** | ย้ายจาก input-only     |
| `PIN_SW_NET`                     |        36 |    **39** | reserved 26–37         |
| `PIN_SW_FREE`                    |        39 |    **42** | ย้ายสำรองไป I/O ทั่วไป |

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

| คำสั่ง            | ผล                              |
| ----------------- | ------------------------------- |
| `-auto` / `--a`   | mode → AUTO                     |
| `-manual` / `--m` | mode → MANUAL                   |
| `-idle` / `--i`   | mode → IDLE                     |
| `-pump on/off`    | สั่ง pump (MANUAL เท่านั้น)     |
| `-mist on/off`    | สั่ง mist (MANUAL เท่านั้น)     |
| `-air on/off`     | สั่ง air pump (MANUAL เท่านั้น) |
| `-clear`          | reset manual overrides          |
| `-net on/off`     | เปิด/ปิด STA WiFi               |
| `time=HH:MM[:SS]` | ตั้งเวลา RTC                    |
| `-help`           | แสดงคำสั่งทั้งหมด               |

---

## 🗑️ ไฟล์ที่ควรลบออก (Obsolete)

ไฟล์เหล่านี้ยังอยู่ในโปรเจคแต่ไม่มีใคร include หรือเรียกใช้แล้ว หลังจาก refactor Schedule เป็น `TimeSchedule` + `ScheduledRelay`:

| ไฟล์                               | เหตุผล                                      |
| ---------------------------------- | ------------------------------------------- |
| `include/domain/AirPumpSchedule.h` | แทนด้วย `TimeSchedule` + `TimeWindow` แล้ว  |
| `src/domain/AirPumpSchedule.cpp`   | มี `loadAirPumpSchedule()` ที่ไม่มีใครเรียก |

> ⚠️ `test/test_farmmanager.cpp` ยังใช้ `AirPumpSchedule`, `FarmManager(&sched)`, และ `dec.airOn` ซึ่งถูกลบไปทั้งหมดแล้ว ต้องอัปเดต test ก่อนลบไฟล์เหล่านี้

---

## 🚧 สิ่งที่ยังไม่เสร็จ

- [ ] **ลบ `AirPumpSchedule.h/.cpp`** หลังจากอัปเดต test แล้ว
- [ ] **อัปเดต `test/test_farmmanager.cpp`** ให้ใช้ `TimeSchedule` และ `FarmManager()` ใหม่ (ไม่รับ schedule ใน constructor แล้ว)
- [ ] Logic ปั๊มน้ำอัตโนมัติ (AUTO mode — `FarmManager.applyAuto()` ยัง `pumpOn = false` เสมอ)
- [ ] EC / pH sensor integration
- [ ] Web Dashboard: แสดง water temperature จาก DS18B20 ยังไม่ครบ
- [ ] OTA Update
- [ ] Data logging / export ข้อมูลเซนเซอร์ย้อนหลัง

---

## 📁 โครงสร้างโปรเจค

```
SmartFarm_POC/
├── include/
│   ├── Config.h                        ← PIN map + ค่าคงที่ทั้งหมด (source of truth)
│   ├── interfaces/
│   │   ├── Types.h                     ← Domain types (SystemMode, SensorReading, ...)
│   │   ├── IActuator.h
│   │   ├── IClock.h
│   │   ├── IModeSource.h
│   │   ├── INetModeSource.h
│   │   ├── INetwork.h
│   │   ├── ISchedule.h                 ← interface ตารางเวลา (generic)
│   │   ├── ISensor.h
│   │   └── IUi.h
│   ├── domain/
│   │   ├── TimeWindow.h                ← struct ช่วงเวลา [start, end)
│   │   ├── TimeSchedule.h              ← pure logic ตารางเวลา ไม่มี Arduino
│   │   └── AirPumpSchedule.h           ← ⚠️ OBSOLETE — ลบได้
│   ├── application/
│   │   ├── FarmModels.h                ← FarmInput / FarmDecision
│   │   ├── FarmManager.h               ← brain: ตัดสินใจ pump + mist เท่านั้น
│   │   └── ScheduledRelay.h            ← เชื่อม ISchedule + IActuator
│   ├── infrastructure/
│   │   ├── SystemContext.h             ← Object graph holder
│   │   ├── SharedState.h               ← Thread-safe state (FreeRTOS mutex)
│   │   ├── AppBoot.h
│   │   ├── RtcClock.h
│   │   ├── ScheduleStore.h             ← โหลด JSON → ISchedule
│   │   ├── Esp32ModeSwitchSource.h
│   │   ├── Esp32WebUi.h
│   │   ├── Esp32WiFiNetwork.h
│   │   ├── NetTimeSync.h
│   │   └── WifiConfigStore.h
│   ├── drivers/
│   │   ├── Esp32Bh1750Light.h
│   │   ├── Esp32Sht40.h
│   │   ├── Esp32Ds18b20.h
│   │   ├── Esp32WaterLevelInput.h
│   │   ├── Esp32Relay.h
│   │   ├── Esp32ManualSwitch.h
│   │   ├── Esp32NetModeSwitch.h
│   │   └── RtcDs3231Time.h
│   └── tasks/
│       ├── TaskEntrypoints.h
│       └── WebUiTask.h
├── src/
│   ├── main.cpp                        ← Composition Root
│   ├── domain/
│   │   ├── TimeSchedule.cpp
│   │   └── AirPumpSchedule.cpp         ← ⚠️ OBSOLETE — ลบได้
│   ├── application/
│   │   ├── FarmManager.cpp
│   │   ├── ScheduledRelay.cpp
│   │   └── WebUiTask.cpp
│   ├── drivers/
│   ├── infrastructure/
│   │   ├── AppBoot.cpp
│   │   ├── ScheduleStore.cpp
│   │   └── ...
│   └── tasks/
│       ├── SensorTasks.cpp             ← inputTask + controlTask
│       ├── NetworkTask.cpp
│       └── CommandTask.cpp
├── data/
│   ├── schedule.json                   ← ตารางเวลา (LittleFS)
│   └── www/                            ← Web UI
└── test/
    └── test_farmmanager.cpp            ← ⚠️ ต้องอัปเดต (ใช้ API เก่า)
```

---

## 🏗️ สถาปัตยกรรม (Clean Architecture 6 Layer)

```
┌─────────────────────────────────────┐
│  Tasks / Main (FreeRTOS wiring)     │  ← src/tasks, main.cpp
├─────────────────────────────────────┤
│  Infrastructure (State, Network)    │  ← SharedState, AppBoot, WiFi
├─────────────────────────────────────┤
│  Drivers (Hardware Adapters)        │  ← BH1750, SHT40, Relay, Switch
├─────────────────────────────────────┤
│  Application                        │
│    FarmManager   — pump + mist      │  ← sensor-based hysteresis
│    ScheduledRelay — air pump        │  ← time-based schedule
├─────────────────────────────────────┤
│  Domain (TimeSchedule, TimeWindow)  │  ← pure logic ไม่มี Arduino
├─────────────────────────────────────┤
│  Interfaces (Contracts)             │  ← ISensor, IActuator, ISchedule
└─────────────────────────────────────┘
```

**กฎสำคัญ:**

- Domain layer **ห้ามแตะ Arduino API** — ทดสอบได้โดยไม่ต้องมี hardware
- Relay สั่งได้เฉพาะใน `controlTask` เท่านั้น
- คำสั่งจาก Serial / Web ต้องผ่าน `SharedState` เสมอ (thread-safe)
- Air pump ควบคุมโดย `ScheduledRelay` — ไม่ผ่าน `FarmDecision` อีกต่อไป

---

## ✅ สถานะการ Review Code

| ไฟล์                                     |     สถานะ     |
| ---------------------------------------- | :-----------: |
| `interfaces/Types.h`                     |      ✅       |
| `interfaces/IActuator.h`                 |      ✅       |
| `interfaces/ISensor.h`                   |      ✅       |
| `interfaces/IClock.h`                    |      ✅       |
| `interfaces/IModeSource.h`               |      ✅       |
| `interfaces/INetModeSource.h`            |      ✅       |
| `interfaces/ISchedule.h`                 |      ✅       |
| `interfaces/INetwork.h`                  |      ⏳       |
| `interfaces/IUi.h`                       |      ⏳       |
| `domain/TimeWindow.h`                    |      ✅       |
| `domain/TimeSchedule.h/.cpp`             |      ✅       |
| `domain/AirPumpSchedule.h/.cpp`          |  🗑️ obsolete  |
| `application/FarmModels.h`               |      ✅       |
| `application/FarmManager.h/.cpp`         |      ✅       |
| `application/ScheduledRelay.h/.cpp`      |      ✅       |
| `drivers/Esp32Relay.h/.cpp`              |      ✅       |
| `drivers/Esp32ManualSwitch.h/.cpp`       |      ✅       |
| `drivers/Esp32NetModeSwitch.h/.cpp`      |      ✅       |
| `drivers/Esp32ModeSwitchSource.h/.cpp`   |      ✅       |
| `drivers/Esp32Bh1750Light.h/.cpp`        |      ✅       |
| `drivers/Esp32Sht40.h/.cpp`              |      ✅       |
| `drivers/Esp32Ds18b20.h/.cpp`            |      ✅       |
| `drivers/Esp32WaterLevelInput.h`         |      ✅       |
| `drivers/RtcDs3231Time.h/.cpp`           |      ✅       |
| `infrastructure/RtcClock.h/.cpp`         |      ✅       |
| `infrastructure/SystemContext.h`         |      ✅       |
| `infrastructure/AppBoot.h/.cpp`          |      ✅       |
| `infrastructure/ScheduleStore.h/.cpp`    |      ✅       |
| `infrastructure/SharedState.h/.cpp`      |      ⏳       |
| `infrastructure/Esp32WebUi.h/.cpp`       |      ⏳       |
| `infrastructure/Esp32WiFiNetwork.h/.cpp` |      ⏳       |
| `infrastructure/NetTimeSync.h/.cpp`      |      ⏳       |
| `infrastructure/WifiConfigStore.h/.cpp`  |      ⏳       |
| `tasks/SensorTasks.cpp`                  |      ✅       |
| `tasks/NetworkTask.cpp`                  |      ⏳       |
| `tasks/CommandTask.cpp`                  |      ⏳       |
| `src/main.cpp`                           |      ✅       |
| `test/test_farmmanager.cpp`              | ⚠️ ต้องอัปเดต |
