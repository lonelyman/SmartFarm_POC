# SmartFarm_POC

โครงงานต้นแบบ SmartFarm (ESP32 + FreeRTOS) – เอกสารหลักอธิบายสถาปัตยกรรม, แนวทางพัฒนา, พฤติกรรม runtime, และวิธีขยายระบบ

---

## 🎬 Quick Start

1. ติดตั้ง [PlatformIO](https://platformio.org) แล้วรัน build:

   ```sh
   platformio run
   ```

2. อัปโหลดโค้ดขึ้นบอร์ด (จาก VS Code/PlatformIO: ▶ Upload)
3. อัปโหลดระบบไฟล์ LittleFS (จำเป็นสำหรับ schedule.json และ web UI):

   ```sh
   pio run -t uploadfs
   ```

4. ถ้าแก้โครงสร้างโปรเจค อย่าลืมอัปเดต snapshot:

   ```sh
   tree > tree.txt
   ```

---

## 📁 โครงสร้างโปรเจคสำคัญ

- `include/` – headers (Interfaces, Types, Domain APIs, Application APIs, Infrastructure, Drivers)
- `src/` – implementation (drivers, tasks, main, domain logic)
- `test/` – unit test (`test_farmmanager.cpp`)
- `data/www/` – Web UI (dashboard.html, wifi.html)
- `data/schedule.json` – ตารางเวลาปั๊มลม (LittleFS)
- `platformio.ini` – การตั้งค่า build/board/library
- `tree.txt` – snapshot โครงสร้างโปรเจค

---

## 🏗️ สถาปัตยกรรม (6‑Layer + Adapters)

เป้าหมายหลัก: **แยก business logic ออกจากฮาร์ดแวร์** เพื่อให้โค้ดพอร์ตง่ายและทดสอบได้

1. **Interfaces** – `include/interfaces`
   - สัญญาระหว่างโมดูล: `ISensor`, `IActuator`, `IClock`, `IModeSource`, `Types.h`
   - ห้าม include Arduino หรือ hardware ใดๆ

2. **Domain** – `include/domain`
   - โครงข้อมูลธุรกิจ (`AirPumpSchedule`, `TimeWindow`)
   - ไม่มี logic ที่เกี่ยวกับฮาร์ดแวร์หรือ FreeRTOS

3. **Application** – `include/application`
   - ตัดสินใจควบคุมฟาร์ม (`FarmManager`)
   - รับ `FarmInput` คืน `FarmDecision`
   - **ห้ามใช้ Arduino API** ทุกกรณี

4. **Infrastructure** – `include/infrastructure`
   - `SharedState` (mutex, single source of truth)
   - `SystemContext` (composition root)
   - Network, FileSystem, Clock adapters

5. **Drivers** – `include/drivers` + `src/drivers`
   - โค้ดติดต่อฮาร์ดแวร์จริง (BH1750, SHT40, RTC, Relay)

6. **Tasks / Main** – `src/tasks`, `src/main.cpp`
   - FreeRTOS tasks, wiring ทุกอย่างผ่าน `SystemContext`

### 🔧 กฎสำคัญ

- เลเยอร์บน (Interfaces/Domain/Application) **ห้าม**ใช้ Arduino API, FreeRTOS หรือฮาร์ดแวร์
- **Business logic** อยู่เพียงที่ `Application` / `Domain`
- `SharedState` เป็น single source of truth ป้องกัน race ด้วย mutex
- การอ่าน/เขียนรีเลย์กระทำเฉพาะใน `controlTask` หรือ driver เท่านั้น
- คำสั่งจาก Serial หรือ Web → ผ่าน `SharedState` เสมอ

---

## 🔌 Hardware & IO Overview

### บอร์ด
- **ESP32 DOIT DEVKIT V1** (esp32doit-devkit-v1)
- Framework: Arduino + FreeRTOS (PlatformIO)

### เซนเซอร์

| เซนเซอร์ | ชิป | Interface | หมายเหตุ |
|---|---|---|---|
| แสง | BH1750 | I2C (SDA=21, SCL=22) | lux |
| อุณหภูมิ + ความชื้น | **SHT40** | I2C (SDA=21, SCL=22) | address 0x44 |
| ระดับน้ำ CH1 | XKC-Y25 | GPIO 32 | enable ใน Config.h |
| ระดับน้ำ CH2 | XKC-Y25 | GPIO 33 | enable ใน Config.h |

### Actuators (Relay)

| ชื่อ | GPIO |
|---|---|
| ปั๊มน้ำ | 25 |
| หมอก (Mist) | 26 |
| ปั๊มลม | 27 |
| สำรอง FREE1-4 | 13, 16, 17, 14 |

### สวิตช์

| สวิตช์ | GPIO | ฟังก์ชัน |
|---|---|---|
| MODE A | 34 | เลือกโหมด AUTO/MANUAL/IDLE |
| MODE B | 35 | เลือกโหมด AUTO/MANUAL/IDLE |
| NET AP | 36 (VP) | สลับ AP/STA |
| NET STA | 39 (VN) | สลับ AP/STA |

**ตารางโหมด:**
- A=HIGH, B=HIGH → IDLE
- A=LOW, B=HIGH → AUTO
- A=HIGH, B=LOW → MANUAL

---

## ⚙️ Runtime Modes

### IDLE
- รีเลย์ทุกตัวปิด, ล้าง latch ทั้งหมด
- reset boot guard และ mist guard
- ใช้เป็นโหมดพัก/ปลอดภัย

### AUTO
- **Boot Guard**: หน่วง 10 วินาทีหลัง boot ก่อนเริ่มทำงาน (ป้องกัน relay กระตุกและรอ sensor warm-up)
- **หมอก**: ควบคุมด้วย temp + humidity พร้อมกัน (ดูหัวข้อ Mist Logic)
- **ปั๊มลม**: ตาม schedule (schedule.json)
- **ปั๊มน้ำ**: ยังไม่มี logic อัตโนมัติ

### MANUAL
- เคลียร์ manual overrides และปิดรีเลย์ทั้งหมดเมื่อเข้าโหมด
- ผู้ใช้สั่งผ่าน Serial หรือ Web UI

---

## 💧 Mist Logic (AUTO mode)

### การตัดสินใจ (priority order)

1. **ทั้ง temp และ humidity valid** → `decideMistByTempAndHumidity()`
2. **มีแค่ humidity** → `decideMistByHumidity()` (fallback)
3. **มีแค่ temp** → `decideMistByTemp()` (fallback)

### Hysteresis (ปรับใน `Config.h`)

```cpp
// อุณหภูมิ
HYSTERESIS_TEMP_ON  = 32.0f   // เปิดเมื่อ temp ≥ 32°C
HYSTERESIS_TEMP_OFF = 29.0f   // ปิดเมื่อ temp ≤ 29°C

// ความชื้น
HYSTERESIS_HUMIDITY_ON  = 65.0f  // เปิดพ่นเมื่อ humidity ≤ 65%
HYSTERESIS_HUMIDITY_OFF = 75.0f  // ปิดพ่นเมื่อ humidity ≥ 75%
```

### ตาราง decision (temp+humidity)

| Temp | Humidity | ผล |
|---|---|---|
| ≥32°C | ≤65% | ✅ เปิดพ่น |
| ≥32°C | ≥75% | ❌ ไม่พ่น (ชื้นพอแล้ว) |
| ≤29°C | ≤65% | ❌ ไม่พ่น (ไม่ร้อน) |
| ระหว่าง | ระหว่าง | ⏸️ คง state เดิม (dead-band) |

### Mist Time Guard

ป้องกันพ่นไม่หยุดในกรณี temp ไม่ยอมลด:

```cpp
MIST_MAX_ON_MS  = 3 * 60 * 1000UL  // พ่นได้สูงสุด 3 นาที
MIST_MIN_OFF_MS = 5 * 60 * 1000UL  // พักอย่างน้อย 5 นาที
```

Flow: พ่นครบ 3 นาที → บังคับหยุด 5 นาที → sensor ตัดสินใจใหม่

---

## 🕒 Time Source & Air Pump Schedule

### DS3231 RTC
- ใช้เป็นแหล่งเวลาเดียว (ผ่าน `IClock` adapter)
- หาก RTC ไม่ตอบสนอง → `getMinutesOfDay()` คืน false, minutes=0, schedule ปิด
- เมื่อมี WiFi → `NetTimeSync` ซิงค์ RTC จาก NTP อัตโนมัติ

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

Fallback: หากไฟล์หายหรือ parse ล้มเหลว → schedule ปิด (`enabled=false`)

---

## 📡 Network & Web UI

- Boot เข้า **AP mode** (SSID: SmartFarm-Setup) เป็นค่าเริ่มต้น
- ตั้งค่า WiFi ผ่าน `http://192.168.4.1/wifi`
- Dashboard: `http://192.168.4.1/` แสดง sensor + actuator + network status แบบ real-time
- เมื่อต่อ STA สำเร็จ → ย้ายไปใช้ IP ใน LAN

### /api/status fields

```json
{
  "mode": 0,
  "pump": 0, "mist": 0, "air": 0,
  "lux": 0.0, "luxValid": 0,
  "temp": 0.0, "tempValid": 0,
  "hum": 0.0, "humValid": 0,
  "staConnected": 0,
  "apIp": "...", "staIp": "...",
  "netMsg": "..."
}
```

---

## 🖥️ Serial Command Cheatsheet

**Baud**: 115200

| คำสั่ง | ผล |
|---|---|
| `-auto` / `--a` | เข้าโหมด AUTO |
| `-manual` / `--m` | เข้าโหมด MANUAL |
| `-idle` / `--i` | เข้าโหมด IDLE |
| `-mist on/off` | สั่ง mist (ต้อง MANUAL) |
| `-pump on/off` | สั่ง pump (ต้อง MANUAL) |
| `-air on/off` | สั่ง air (ต้อง MANUAL) |
| `-clear` | ปิดทุก relay (ไม่เปลี่ยนโหมด) |
| `time=HH:MM[:SS]` | ตั้งเวลา RTC |
| `-help` | แสดง cheat sheet |

---

## 📝 Debug Flags (`Config.h`)

```cpp
#define DEBUG_BH1750_LOG    1
#define DEBUG_TEMP_LOG      0  // SHT40 temp
#define DEBUG_TIME_LOG      1
#define DEBUG_WATER_LEVEL_LOG 1
#define DEBUG_CONTROL_LOG   1
```

---

## 🔁 Runtime Flow

1. **AppBoot**: init I2C → LittleFS → drivers → clock → network → IDLE → start tasks
2. **InputTask** (1s): อ่าน BH1750 + SHT40 (temp+humidity) + water level → push to SharedState
3. **ControlTask** (1s): อ่านเวลา → snapshot → update mode → FarmManager.update() → apply relays → sync back
4. **CommandTask** (50ms): รับ Serial → parse → แก้ SharedState
5. **NetworkTask**: ดูแล WiFi/NTP
6. **WebUiTask**: serve HTTP dashboard + API

---

> **หมายเหตุ:** กฎแยกเลเยอร์ต้องรักษาไว้เสมอ — Application/Domain ห้ามแตะ Arduino API และ relay ถูกสั่งได้เฉพาะใน `controlTask` เท่านั้น
