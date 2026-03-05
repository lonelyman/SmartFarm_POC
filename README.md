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

## 📌 GPIO Pin Map

### ใช้งานอยู่

| GPIO    | ประเภท          | ใช้เป็น                                     |
| ------- | --------------- | ------------------------------------------- |
| 5       | I/O (strapping) | SW_MANUAL_MIST — Active LOW, pull-up ภายนอก |
| 15      | I/O (strapping) | SW_MANUAL_AIR — Active LOW, pull-up ภายนอก  |
| 18      | I/O ✅          | SW_MANUAL_PUMP — Active LOW, pull-up ภายนอก |
| 19      | I/O ✅          | WATER_LEVEL_CH2_ALARM_LED                   |
| 21      | I/O ✅          | I2C SDA (BH1750, SHT40, DS3231)             |
| 22      | I/O ✅          | I2C SCL (BH1750, SHT40, DS3231)             |
| 23      | I/O ✅          | WATER_LEVEL_CH1_ALARM_LED                   |
| 25      | I/O ✅          | RELAY_WATER_PUMP                            |
| 26      | I/O ✅          | RELAY_MIST                                  |
| 27      | I/O ✅          | RELAY_AIR_PUMP                              |
| 32      | I/O ✅          | WATER_LEVEL_CH1_SENSOR                      |
| 33      | I/O ✅          | WATER_LEVEL_CH2_SENSOR                      |
| 34      | Input only      | SW_MODE_A                                   |
| 35      | Input only      | SW_MODE_B                                   |
| 36 (VP) | Input only      | SW_NET — HIGH=STA, LOW=AP                   |

### ว่าง

| GPIO    | ประเภท     | หมายเหตุ                            |
| ------- | ---------- | ----------------------------------- |
| 39 (VN) | Input only | PIN_SW_FREE — ไม่มี internal pullup |
| 13      | I/O ✅     | RELAY_FREE1 (จองไว้เผื่อ)           |
| 14      | I/O ✅     | RELAY_FREE4 (จองไว้เผื่อ)           |
| 16      | I/O ✅     | RELAY_FREE2 (จองไว้เผื่อ)           |
| 17      | I/O ✅     | RELAY_FREE3 (จองไว้เผื่อ)           |

### ห้ามใช้

| GPIO | เหตุผล                                    |
| ---- | ----------------------------------------- |
| 6–11 | SPI Flash ภายใน — แตะแล้วบอร์ดค้าง/พัง    |
| 12   | Strapping — HIGH ตอน boot ทำให้ flash พัง |
| 0, 2 | Strapping — กระทบ boot sequence           |

---

## 📁 โครงสร้างโปรเจคสำคัญ

- `include/` – headers (Interfaces, Types, Domain, Application, Infrastructure, Drivers)
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

3. **Application** – `include/application`
   - ตัดสินใจควบคุมฟาร์ม (`FarmManager`)
   - รับ `FarmInput` คืน `FarmDecision`
   - **ห้ามใช้ Arduino API ทุกกรณี**

4. **Infrastructure** – `include/infrastructure`
   - `SharedState` (mutex, single source of truth)
   - `SystemContext` (composition root)
   - Network, FileSystem, Clock adapters

5. **Drivers** – `include/drivers` + `src/drivers`
   - โค้ดติดต่อฮาร์ดแวร์จริง (BH1750, SHT40, DS3231, Relay, ManualSwitch)

6. **Tasks / Main** – `src/tasks`, `src/main.cpp`
   - FreeRTOS tasks, wiring ทุกอย่างผ่าน `SystemContext`

### 🔧 กฎสำคัญ

- เลเยอร์บน (Interfaces/Domain/Application) **ห้าม**ใช้ Arduino API, FreeRTOS หรือฮาร์ดแวร์
- **Business logic** อยู่เพียงที่ `Application` / `Domain`
- `SharedState` เป็น single source of truth ป้องกัน race ด้วย mutex
- การอ่าน/เขียนรีเลย์กระทำเฉพาะใน `controlTask` หรือ driver เท่านั้น
- คำสั่งจาก Serial หรือ Web → ผ่าน `SharedState` เสมอ

---

## 🔌 Hardware Overview

### บอร์ด

- **ESP32 DOIT DEVKIT V1** (esp32doit-devkit-v1)
- Framework: Arduino + FreeRTOS (PlatformIO)

### เซนเซอร์

| เซนเซอร์            | ชิป     | Interface                         |
| ------------------- | ------- | --------------------------------- |
| แสง                 | BH1750  | I2C (SDA=21, SCL=22)              |
| อุณหภูมิ + ความชื้น | SHT40   | I2C (SDA=21, SCL=22) address 0x44 |
| ระดับน้ำ CH1        | XKC-Y25 | GPIO 32                           |
| ระดับน้ำ CH2        | XKC-Y25 | GPIO 33                           |

### Actuators

| ชื่อ        | GPIO |
| ----------- | ---- |
| ปั๊มน้ำ     | 25   |
| หมอก (Mist) | 26   |
| ปั๊มลม      | 27   |

### สวิตช์

| สวิตช์      | GPIO | ฟังก์ชัน                | วงจร                      |
| ----------- | ---- | ----------------------- | ------------------------- |
| MODE A      | 34   | เลือกโหมด               | Input only                |
| MODE B      | 35   | เลือกโหมด               | Input only                |
| NET         | 36   | AP/STA                  | HIGH=STA, LOW=AP          |
| MANUAL PUMP | 18   | คุม pump (เฉพาะ MANUAL) | Active LOW + pull-up 10kΩ |
| MANUAL MIST | 5    | คุม mist (เฉพาะ MANUAL) | Active LOW + pull-up 10kΩ |
| MANUAL AIR  | 15   | คุม air (เฉพาะ MANUAL)  | Active LOW + pull-up 10kΩ |

**ตารางโหมด (MODE A/B):**

- A=HIGH, B=HIGH → IDLE
- A=LOW, B=HIGH → AUTO
- A=HIGH, B=LOW → MANUAL

---

## ⚙️ Runtime Modes

### IDLE

- รีเลย์ทุกตัวปิด, ล้าง latch ทั้งหมด
- reset boot guard และ mist guard

### AUTO

- **Boot Guard**: หน่วง 10 วินาทีหลัง boot ก่อนเริ่มทำงาน
- **หมอก**: ควบคุมด้วย temp + humidity (ดูหัวข้อ Mist Logic)
- **ปั๊มลม**: ตาม schedule.json
- **ปั๊มน้ำ**: ยังไม่มี logic อัตโนมัติ
- สวิตช์ MANUAL กดไปไม่มีผล

### MANUAL

- เคลียร์ manual overrides และปิดรีเลย์ทั้งหมดเมื่อเข้าโหมด
- สั่งผ่าน Serial, Web UI, หรือสวิตช์ทางกายภาพ (GPIO 5, 15, 18)
- สวิตช์ทางกายภาพ: กดค้าง = เปิด, ปล่อย = ปิด

---

## 💧 Mist Logic (AUTO mode)

### การตัดสินใจ (priority order)

1. ทั้ง temp และ humidity valid → `decideMistByTempAndHumidity()`
2. มีแค่ humidity → `decideMistByHumidity()` (fallback)
3. มีแค่ temp → `decideMistByTemp()` (fallback)

### Hysteresis (ปรับใน `Config.h`)

```cpp
HYSTERESIS_TEMP_ON  = 32.0f   // เปิดเมื่อ temp ≥ 32°C
HYSTERESIS_TEMP_OFF = 29.0f   // ปิดเมื่อ temp ≤ 29°C
HYSTERESIS_HUMIDITY_ON  = 65.0f  // เปิดพ่นเมื่อ humidity ≤ 65%
HYSTERESIS_HUMIDITY_OFF = 75.0f  // ปิดพ่นเมื่อ humidity ≥ 75%
```

### ตาราง decision (temp+humidity)

| Temp    | Humidity | ผล                           |
| ------- | -------- | ---------------------------- |
| ≥32°C   | ≤65%     | ✅ เปิดพ่น                   |
| ≥32°C   | ≥75%     | ❌ ไม่พ่น (ชื้นพอแล้ว)       |
| ≤29°C   | ใดก็ได้  | ❌ ไม่พ่น (ไม่ร้อน)          |
| ระหว่าง | ระหว่าง  | ⏸️ คง state เดิม (dead-band) |

### Mist Time Guard

```cpp
MIST_MAX_ON_MS  = 3 นาที   // พ่นได้สูงสุด
MIST_MIN_OFF_MS = 5 นาที   // พักขั้นต่ำ
```

### Boot Guard

```cpp
BOOT_GUARD_MS = 10 วินาที  // หน่วงหลัง boot ก่อน AUTO ทำงาน
```

---

## 🕒 Time Source & NTP Sync

### DS3231 RTC

- ใช้เป็นแหล่งเวลาเดียว (ผ่าน `IClock` adapter)
- หาก RTC ไม่ตอบสนอง → `getMinutesOfDay()` คืน false, minutes=0, schedule ปิด

### NTP Sync

- ระบบ **ไม่ได้ sync อัตโนมัติ** ต้องสั่งเองเท่านั้น
- `syncRtcFromNtp()` จะทำงานได้เฉพาะเมื่อ STA เชื่อมต่อสำเร็จแล้ว
- วิธีสั่ง sync ผ่าน Serial: `time=HH:MM[:SS]` หรือผ่าน Web UI

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

- Boot เข้า **AP mode** (SSID: SmartFarm-Setup) เป็นค่าเริ่มต้น
- ตั้งค่า WiFi ผ่าน `http://192.168.4.1/wifi`
- Dashboard: `http://192.168.4.1/` แสดง sensor + actuator + network status
- สวิตช์ GPIO36: HIGH → ต่อ STA, LOW → กลับ AP
- เมื่อต่อ STA สำเร็จ → AP ยังเปิดอยู่ด้วย (AP+STA พร้อมกัน)

### /api/status fields

```json
{
   "mode": 0,
   "pump": 0,
   "mist": 0,
   "air": 0,
   "lux": 0.0,
   "luxValid": 0,
   "temp": 0.0,
   "tempValid": 0,
   "hum": 0.0,
   "humValid": 0,
   "staConnected": 0,
   "apIp": "...",
   "staIp": "...",
   "netMsg": "..."
}
```

---

## 🖥️ Serial Command Cheatsheet

**Baud**: 115200

| คำสั่ง            | ผล                            |
| ----------------- | ----------------------------- |
| `-auto` / `--a`   | เข้าโหมด AUTO                 |
| `-manual` / `--m` | เข้าโหมด MANUAL               |
| `-idle` / `--i`   | เข้าโหมด IDLE                 |
| `-mist on/off`    | สั่ง mist (ต้อง MANUAL)       |
| `-pump on/off`    | สั่ง pump (ต้อง MANUAL)       |
| `-air on/off`     | สั่ง air (ต้อง MANUAL)        |
| `-clear`          | ปิดทุก relay (ไม่เปลี่ยนโหมด) |
| `time=HH:MM[:SS]` | ตั้งเวลา RTC และ sync NTP     |
| `-help`           | แสดง cheat sheet              |

---

## 📝 Debug Flags (`Config.h`)

```cpp
#define DEBUG_BH1750_LOG      1
#define DEBUG_TEMP_LOG        0
#define DEBUG_TIME_LOG        1
#define DEBUG_WATER_LEVEL_LOG 1
#define DEBUG_CONTROL_LOG     1
```

---

## 🔁 Runtime Flow

1. **AppBoot**: init I2C → LittleFS → drivers → clock → network → IDLE → start tasks
2. **InputTask** (1s): อ่าน BH1750 + SHT40 (temp+humidity) + water level + manual switches → push to SharedState
3. **ControlTask** (1s): อ่านเวลา → snapshot → update mode → FarmManager.update() → apply relays → sync back
4. **CommandTask** (50ms): รับ Serial → parse → แก้ SharedState
5. **NetworkTask** (20ms): ดูแล WiFi state machine + net switch
6. **WebUiTask**: serve HTTP dashboard + API

---

> **หมายเหตุ:** กฎแยกเลเยอร์ต้องรักษาไว้เสมอ — Application/Domain ห้ามแตะ Arduino API และ relay ถูกสั่งได้เฉพาะใน `controlTask` เท่านั้น
