# SmartFarm_POC

โครงงานต้นแบบ SmartFarm (ESP32 + FreeRTOS) – เอกสารหลักอธิบายสถาปัตยกรรม, แนวทางพัฒนา, พฤติกรรม runtime, และวิธีขยายระบบ

สรุป: README นี้เก็บภาพรวมของโปรเจค, บันทึกกฎ/ข้อสังเกต, และทำหน้าที่เป็น single source of truth ต่อไฟล์ต่างๆ ที่เปลี่ยนไปหลังการปรับโค้ดใหม่

---

## 🎬 Quick Start

1. ติดตั้ง [PlatformIO](https://platformio.org) แล้วรัน build:

   ```sh
   platformio run
   ```

2. อัปโหลดโค้ดขึ้นบอร์ด (จาก VS Code/PlatformIO: ▶ Upload)
3. ถ้าแก้โครงสร้างโปรเจค อย่าลืมอัปเดต snapshot:

   ```sh
   tree > tree.txt
   ```

4. (ตัวเลือก) อัปโหลดระบบไฟล์ LittleFS หากใช้ `schedule.json`:

   ```sh
   pio run -t uploadfs
   ```

---

## 📁 โครงสร้างโปรเจคสำคัญ

- `include/` – headers (Interfaces, Types, Domain APIs, Application APIs, Infrastructure)
- `src/` – implementation (drivers, tasks, main, domain logic)
- `lib/` – ไลบรารีเฉพาะโปรเจค (แพลตฟอร์ม PlatformIO)
- `test/` – โค้ดสำหรับ unit test (PlatformIO Test Runner). ตัวอย่างมี `test_farmmanager.cpp` ทดสอบ logic ของ FarmManager
- `platformio.ini` – การตั้งค่า build/board/library
- `tree.txt` – snapshot โครงสร้างโปรเจค

---

## 🏗️ สถาปัตยกรรม (6‑Layer + Adapters)

เป้าหมายหลักยังคงเดิม: **แยก business logic ออกจากฮาร์ดแวร์** เพื่อให้โค้ดพอร์ตง่ายและทดสอบได้

เลเยอร์หลัก:

1. **Interfaces** – `include/interfaces`
   - สัญญาระหว่างโมดูล เช่น `ISensor`, `IActuator`, `IClock`, `IModeSource`, และ `Types.h`
   - ไม่มีการ include Arduino หรือ hardware ใดๆ
2. **Domain** – `include/domain`
   - โครงข้อมูลธุรกิจ (เช่น `AirPumpSchedule`, `TimeWindow`)
   - ไม่มี logic ที่เกี่ยวกับฮาร์ดแวร์หรือ FreeRTOS
3. **Application** – `include/application`
   - นำ domain models มาตัดสินใจ (e.g. `FarmManager`)
   - ฟังก์ชัน `update()` รับ `SystemStatus`, `ManualOverrides`, `minutesOfDay`
   - **logic ทั้งหมด** อยู่ที่นี่และใน domain; upper layers ห้ามใช้ Arduino APIs
4. **Infrastructure** – `include/infrastructure`
   - เตรียม context, shared state, driver wrappers, ไฟล์ระบบ, network
   - ตัวอย่าง: `SharedState` (mutex), `SystemContext`, `ScheduleStore`, `NetTimeSync`
5. **Drivers** – `include/drivers` + `src/drivers`
   - โค้ดติดต่อฮาร์ดแวร์ (BH1750, RTC, Relay, FakeTemperature ฯลฯ)
   - `IClock` adapter สำหรับ RTC/NTP/fake time
6. **Tasks / Main** – `src/tasks`, `src/main.cpp`
   - สร้าง FreeRTOS tasks, เปลี่ยนเป็น `SystemContext` แล้ว kick off loop
   - ใช้เฉพาะบนเลเยอร์นี้เรียก driver, infrastructure และ application

**Adapters**: mode source, clock source, filesystem, network – เปลี่ยนเป็นโมดูลที่ฉีดผ่าน `SystemContext`

### 🔧 กฎสำคัญ

- เลเยอร์บน (Interfaces/Domain/Application) **ห้าม**ใช้ Arduino API, FreeRTOS หรือฮาร์ดแวร์
- **Business logic** อยู่เพียงที่ `Application`/`Domain`
- `SharedState` เป็น single source of truth; ป้องกัน race ด้วย mutex
- การอ่าน/เขียนรีเลย์กระทำเฉพาะใน `controlTask` หรือ driver
- ถ้าต้องสั่งอะไรก็ตามจาก Serial → ไปผ่าน `SharedState` (manual overrides, clock requests)
- ทดสอบ logic โดย mock `SystemContext`/`IClock`/`IModeSource` ได้เลย

---

## 🔌 Hardware & IO Overview

**Config constants**

หลายค่าที่ยกขึ้นมาก่อนหน้า (เช่น hysteresis อุณหภูมิสำหรับ mist,
debounce สำหรับสวิตช์เน็ต, ช่วงเวลาเริ่ม/เลิกปั๊มลม) ถูกย้ายไป
`include/Config.h` เป็น `constexpr` เพื่อให้ปรับได้ง่ายโดยไม่ต้องแตะ logic.

### คอนฟิกบนบอร์ด

- **บอร์ด**: ESP32 DOIT DEVKIT V1
- **เฟรมเวิร์ก**: Arduino + FreeRTOS (PlatformIO)

### เลขพินสำคัญ (ดู `Config.h`)

```cpp
// I2C: BH1750, RTC
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;

// รีเลย์
constexpr int PIN_RELAY_WATER_PUMP = 25;
constexpr int PIN_RELAY_MIST       = 26;
constexpr int PIN_RELAY_AIR_PUMP   = 27;

// inputs อื่นๆ
constexpr int PIN_ANALOG_EC        = 32; // placeholder

// สวิตช์หน้าเครื่อง (mode switch & clear)
constexpr int PIN_SW_MODE_A = 34;
constexpr int PIN_SW_MODE_B = 35;
constexpr int PIN_SW_CLEAR  = 36; // ยังไม่ใช้
```

### เซนเซอร์/แอกชูเอเตอร์

- **Light**: BH1750 (I2C)
- **Temperature**: `Esp32FakeTemperature` (ตอนนี้ยังเป็น fake)
- **Water‑level**: analog input via `Esp32WaterLevelInput` (ยังไม่ถูกใช้งานจริง)
- **Relays**: ต่อย SSR / โมดูลรีเลย์ (ปั๊มน้ำ / หมอก / ปั๊มลม)

### สวิตช์โหมดเครือข่าย

- มีสวิตช์ทางกายภาพสองตัว (pins `PIN_SW_NET_AP` และ `PIN_SW_NET_STA`)
- ปกติอ่านค่าเป็น HIGH/LOW แล้ว `NetworkTask` จะสั่งให้ระบบ
  `requestNetOn()` / `requestNetOff()` ตามความต้องการ
- ตำแหน่งสวิตช์กำหนดโหมดที่ต้องการ:
   - AP PRIMARY = เปิดเป็น Access Point (ใช้ในการตั้งค่า Wi‑Fi หรือเมื่อ
     ไม่มีการเชื่อมต่ออยู่)
   - STA = เปิดเฉพาะสถานะลูกข่าย หากมี config อยู่ จะพยายาม
     เชื่อมต่อเข้า Wi‑Fi ที่บันทึกไว้
- การสลับจะทำงานแบบ edge‑trigger debounced (แก้ไขเวลาใน `Config.h`)
- วิธีนี้ให้ผู้ใช้สามารถบังคับให้บอร์ดอยู่ใน AP‑only หรือพยายามเชื่อม
  Internet ได้ทันทีโดยไม่ต้องใช้ Serial หรือเว็บ

### สวิตช์เลือกโหมด

- A = HIGH, B = HIGH → `IDLE`
- A = LOW, B = HIGH → `AUTO`
- A = HIGH, B = LOW → `MANUAL`

(สวิตช์ถูกอ่านผ่าน `IModeSource` adapter; debounced 50 ms)

---

## ⚙️ Runtime Modes

1. **IDLE**
   - สวิตช์หรือ Serial (`-idle`/`--i`)
   - รีเลย์ทุกตัวปิดและล้าง latch
   - ใช้เป็นโหมดพัก/ปลอดภัย
2. **AUTO**
   - สวิตช์หรือ Serial (`-auto`/`--a`)
   - **หมอก**: ควบคุมด้วยอุณหภูมิ (hysteresis 29–32°C)
   - **ปั๊มลม**: ตาม schedule (JSON หรือ hard‑coded)
   - ปั๊มน้ำยังไม่มี logic อัตโนมัติ
3. **MANUAL**
   - สวิตช์หรือ Serial (`-manual`/`--m`)
   - เข้าทุกครั้งจะเคลียร์ค่า manual overrides และปิดรีเลย์ทั้งหมด
   - ผู้ใช้สั่งผ่าน Serial (ดูด้านล่าง)
   - `FarmManager` จะ **ไม่สั่ง actuator**; relay อยู่ภายใต้ manual overrides

**Manual overrides** เป็น struct แยกต่างหากใน `SharedState` เพื่อกัน race ระหว่าง CommandTask และ ControlTask

---

## 🖥️ Serial Command Cheatsheet

**Baud**: 115200

### โหมด

- `-auto` / `--a` → AUTO
- `-manual` / `--m` → MANUAL (และ clear overrides)
- `-idle` / `--i` → IDLE

### ควบคุมรีเลย์ (ต้องอยู่ MANUAL)

- `-mist on` / `-mist off`
- `-pump on` / `-pump off`
- `-air  on` / `-air  off`

หากใช้คำสั่งในโหมดอื่น จะมี warning

### คำสั่งเสริม

- `-clear` ― ปิดทุกรีเลย์ **โดยไม่เปลี่ยนโหมด** (สั่ง temp หรือสวิตช์ CLEAR ก็ได้ในอนาคต)
- `time=HH:MM[:SS]` ― ส่งคำขอให้ ControlTask ตั้งนาฬิกา (ผ่าน `SharedState`)
- `-help` ― แสดง cheat sheet

คำสั่งจะมี cooldown 200 ms เพื่อป้องกัน spam

---

## 🛠️ Manual Overrides & Clock Control

- `SharedState` เก็บ `ManualOverrides` ที่มีฟิลด์ `wantPumpOn`, `wantMistOn`, `wantAirOn` และ flag `isUpdated`.
- `CommandTask` แก้ไข struct นี้เมื่อผู้ใช้สั่ง on/off/clear
- `ControlTask` จะดึงทั้งสถานะและ overrides แล้วส่งให้ `FarmManager`

การตั้งเวลา:

1. ผู้ใช้ป้อน `time=...`
2. CommandTask parse แล้วเรียก `SharedState::requestSetClockTime(...)`
3. ControlTask ในรอบถัดไป `consumeSetClockTime()` และส่งคำขอไปยัง `IClock`

วิธีนี้รักษา clean separation: task หรือ logic อื่นไม่ต้องรู้จักกับ RTC/NTP

---

## 🕒 Time Source & Air Pump Schedule

### Time Source

- ระบบจะใช้ DS3231 RTC เป็นแหล่งนาฬิกาเดียว
- ใน `Config.h` มีคำอธิบายว่าวิธีทำงานหากติดต่อกับ RTC ไม่สำเร็จ:
  เมื่อเรียก `ctx.clock->begin()` (จาก `AppBoot`) หาก `RtcDs3231Time::begin()` คืนค่า false
  _หมายความว่าอุปกรณ์ไม่ตอบหรือ I2C มีปัญหา_ – ต่อไปการเรียก
  `getMinutesOfDay()`/`getTimeOfDay()` จะคืนค่า false และส่ง `minutes=0`.
  ControlTask จะบันทึก warning และ schedule จะถูกปิด (เพราะเวลาไม่ถูกต้อง).
- `ControlTask` จะอ่านเวลา `getMinutesOfDay()` และโยนให้ `FarmManager`.
- เมื่อมีการเชื่อมต่ออินเทอร์เน็ต NetworkTask จะใช้ `NetTimeSync::syncRtcFromNtp()`
  เพื่อปรับ RTC ตาม NTP (`pool.ntp.org` – ต้องตั้งค่า Wi‑Fi/NTP ใน `Config.h`).

### Air Pump Schedule

ระบบรองรับทั้งแบบ hard‑coded และไฟล์ JSON เก็บใน LittleFS

**JSON ตัวอย่าง** (`data/schedule.json`):

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

- `enabled` = feature flag
- `windows` = รายการช่วงเวลา (`HH:MM` format)

`AppBoot::setup()` เมานต์ LittleFS และเรียก `loadAirScheduleFromFS()` (infrastructure) เพื่ออ่านไฟล์

**Fallback**: หากไฟล์ไม่อยู่หรือ parse ล้มเหลว, สถานะ schedule จะถูกปิด (`enabled=false`).

---

## 📡 Network & NTP

- เรื่อง Wi‑Fi ถูกมอบหมายให้ **NetworkTask** (ดู `src/tasks/NetworkTask.cpp`) ซึ่งเป็นเจ้าของการ connect/maintain.
- ผู้ใช้กรอก SSID/รหัสผ่านผ่าน **หน้าเว็บ UI** (`/wifi` path) เมื่อเข้าถึงบอร์ดในโหมด AP (SmartFarm-Setup).
  ข้อมูลจะถูกเก็บใน `wifi.json` โดย `WifiConfigStore` และ `Esp32WiFiNetwork` โหลดอัตโนมัติตอน boot.
  (คลาส `WifiProvisioning` ถูกลบออกจากโค้ด – โหมด provisioning เดิมถูกผนวกเข้าใน WebUI สุดเดียว)
- เมื่อเน็ตเวิร์กพร้อมแล้ว `NetTimeSync` ก็ถูกเรียกผ่าน NetworkTask เพื่อซิงค์เวลา
  (`connectWifiIfNeeded()`/`syncRtcFromNtp()`).
- หากมี Wi‑Fi credential ใน `wifi.json` และ `NetworkTask` ยืนยันการเชื่อมต่อไว้แล้ว,
  การ sync จาก `pool.ntp.org` จะอัปเดต RTC ได้อัตโนมัติ
- เรียก manual จาก Serial หรือเมนูเว็บได้เช่นเดิม

---

## 📝 Logging & Debug Flags

คุมปริมาณ log จาก `Config.h`:

```c
#define DEBUG_BH1750_LOG   0
#define DEBUG_TEMP_LOG     0
#define DEBUG_EC_LOG       0
#define DEBUG_TIME_LOG     1
#define DEBUG_CONTROL_LOG  1
```

เรียก `logMinutesAsClock()` ใน ControlTask เพื่อดูเวลาทุกวินาที

🔧 แนะนำ:

- log สถานะ/เวลา/relay เปลี่ยนปล่อยได้ถาวร
- log ที่ loop เยอะควรอยู่ใต้ `#if DEBUG_*` เสมอ

---

## 🔁 Runtime Flow (Task Overview)

1. **AppBoot** ทำการ:
   - initialises I2C, LittleFS, drivers, clock
   - โหลด air schedule ถ้ามี
   - กำหนดโหมดเริ่มต้นเป็น `IDLE`
   - สตาร์ท tasks (network/input/control/command)
2. **NetworkTask** (~1 s): ดูแลการเชื่อมต่อ Wi‑Fi/อินเทอร์เน็ต, รับคำสั่งจาก `Esp32WebUi` และเรียก `NetTimeSync` เมื่อจำเป็น
3. **InputTask** (~1 s): อ่านเซนเซอร์ (lux, temp, water level), อัปเดต `SharedState`
4. **CommandTask** (~50 ms): รอรับ Serial, parse คำสั่ง, แก้ `SharedState` (mode, overrides, time request)
5. **ControlTask** (~1 s):
   - ประมวลคำขอตั้งเวลา → apply to `IClock`
   - อ่านเวลา (`IClock`) หรือ fake
   - ดึง snapshot + manual overrides
   - นำ `IModeSource` อัปเดตโหมดถ้าเปลี่ยน
   - เรียก `manager.update(status, manual, minutesOfDay)`
   - เปลี่ยนสถานะรีเลย์ผ่าน `IActuator` objects
   - ซิงก์สถานะรีเลย์กลับเข้า `SharedState`

---

## 📬 Contact

SmartFarm_POC maintainers

---

> **หมายเหตุ:** README นี้อัปเดตตามการ refactor ล่าสุด ตรวจสอบให้แน่ใจว่า **กฎแยกเลเยอร์ยังคงปฏิบัติอยู่** และฟังก์ชันทั้งหมดใช้ `SystemContext`/`SharedState` อย่างถูกต้อง
