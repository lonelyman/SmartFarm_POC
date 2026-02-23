# SmartFarm_POC

โปรเจคต้นแบบ SmartFarm (ESP32 + FreeRTOS) — เอกสารทั้งหมดถูกรวบรวมไว้ที่ไฟล์ README หลักนี้

สรุป: README ภาพรวมสถาปัตยกรรม แนวปฏิบัติการพัฒนา วิธีขยายระบบ และพฤติกรรม runtime (โหมด, สวิตช์, schedule เวลา ฯลฯ)  

---

## Quick Start

- ติดตั้ง PlatformIO แล้วรัน build:

   platformio run

- อัพโหลดโค้ดขึ้นบอร์ด (จาก VS Code / PlatformIO: กดปุ่ม ▶ Upload)

- อัพเดต snapshot โครงสร้างโปรเจค:

   tree > tree.txt

---

## รายการไฟล์และโฟลเดอร์สำคัญ

- `include/`
   - header (Interfaces, Types, Domain API, Application API, Infrastructure headers)
- `src/`
   - implementation (drivers, tasks, main)
- `lib/`
   - ไลบรารีเฉพาะโปรเจค (PlatformIO)
- `test/`
   - โค้ดสำหรับทดสอบ (PlatformIO Test Runner)
- `platformio.ini`
   - การตั้งค่า build / board / library
- `tree.txt`
   - snapshot ของโครงสร้างโปรเจค

---

## สถาปัตยกรรม (6-Layer Summary)

เป้าหมาย: แยก Business Logic ออกจากฮาร์ดแวร์ เพื่อให้สามารถพอร์ต logic ไปยังแพลตฟอร์มอื่นได้ง่าย

เลเยอร์หลัก:

- **Interfaces**: `include/interfaces`
   - `Types.h`, `ISensor`, `IActuator` (Pure C++)
- **Domain**: `include/domain`
   - ตรรกะธุรกิจ (StateMachine, Rules) — _ยังว่างไว้รอเติม_
- **Application**: `include/application`
   - `FarmManager` ประสานการตัดสินใจระดับระบบ (AUTO / IDLE / MANUAL)
- **Infrastructure**: `include/infrastructure`
   - `GlobalState`, `SharedState` (thread-safety, snapshot, FreeRTOS mutex)
- **Drivers**: `include/drivers`, `src/drivers`
   - การเข้าถึงฮาร์ดแวร์ (BH1750, FakeTemp, Relay, RTC ฯลฯ)
- **Tasks/Main**: `src/tasks`, `src/main.cpp`
   - สร้าง FreeRTOS Task, อ่านเซนเซอร์, อัพเดต `SharedState`, อ่านสวิตช์, อ่าน Serial

กฎสำคัญ:

- เลเยอร์บน (Interfaces/Domain/Application) **ห้าม**เรียกใช้ Arduino API หรือ hardware ตรง ๆ
- Business Logic อยู่ที่ `Application`/`Domain` เท่านั้น
- `SharedState` เป็น single source of truth  
  ใช้ mutex/semaphore เพื่อกัน race ระหว่างแต่ละ Task

---

## Hardware & IO Overview

### Board & Core

- Board: ESP32 DOIT DEVKIT V1
- Framework: Arduino + FreeRTOS (ผ่าน PlatformIO)

### Sensors & Actuators (ปัจจุบัน)

- Light Sensor: **BH1750**
   - I2C: `SDA = 21`, `SCL = 22`
- Temperature: **Esp32FakeTemperature**
   - ยังไม่มีเซนเซอร์จริง ใช้ค่า fake ผ่าน driver นี้ (ชื่อ: "Main-Temp")
- Relays (ใช้กับ SSR / โมดูลรีเลย์):

   constexpr int PIN_RELAY_WATER_PUMP = 25; // ปั๊มน้ำ
   constexpr int PIN_RELAY_MIST = 26; // ปั๊มหมอก
   constexpr int PIN_RELAY_AIR_PUMP = 27; // ปั๊มลม

Future Reserved:

    constexpr int PIN_RELAY_FREE1 = 13;
    constexpr int PIN_RELAY_FREE2 = 16;
    constexpr int PIN_RELAY_FREE3 = 17;
    constexpr int PIN_RELAY_FREE4 = 14;

### Front Panel Switches (โหมดระบบ)

ใช้สวิตช์หน้าเครื่อง 2 ตัว (ต่อเข้า GPIO พร้อม `INPUT_PULLUP`):

    constexpr int PIN_SW_MODE_A = 18;
    constexpr int PIN_SW_MODE_B = 19;

Mapping (ปัจจุบัน):

- A = HIGH, B = HIGH → `SystemMode::IDLE`
- A = LOW, B = HIGH → `SystemMode::AUTO`
- A = HIGH, B = LOW → `SystemMode::MANUAL`

Debounce โดย `BUTTON_DEBOUNCE_MS = 50` ms

---

## Runtime Modes

ระบบมี 3 โหมดหลัก (ประกาศใน `Types.h`):

1. **IDLE (0)**
   - ตั้งจากสวิตช์หน้าเครื่อง หรือ Serial (`-idle` / `--i`)
   - ปิดทุกรีเลย์: น้ำ, หมอก, ปั๊มลม
   - ใน `FarmManager` จะเรียก `forceAllOff()` ทุกครั้งในโหมดนี้
   - ใช้เป็นโหมดพัก / ปลอดภัย / เทียบเท่า “ปิดตู้”

2. **AUTO (1)**
   - ตั้งจากสวิตช์ หรือ Serial (`-auto` / `--a`)
   - Logic ปัจจุบัน:
      - ใช้ค่าอุณหภูมิ (จาก Fake Temperature) คุม **หมอก (MIST)**
      - ใช้เวลา (Fake time / RTC + schedule) คุม **ปั๊มลม (AIR PUMP)**
   - ปั๊มน้ำยังไม่มี auto-logic (รอ requirement รอบหน้า)

3. **MANUAL (2)**
   - ตั้งจากสวิตช์ หรือ Serial (`-manual` / `--m`)
   - เมื่อเข้า MANUAL:
      - ระบบสั่งปิดรีเลย์ทุกตัว 1 ครั้ง (เริ่มจากพื้นสะอาด)
   - จากนั้น **ผู้ใช้คุมเอง** ผ่าน Serial:
      - `-mist on/off`
      - `-pump on/off`
      - `-air on/off`
   - ในโหมดนี้ `FarmManager` จะ **ไม่สั่ง actuator ใด ๆ**  
     (เพื่อไม่ให้ logic auto มาตีกับคนคุมเอง)

---

## Serial Command Cheatsheet

Serial baud: **115200**

### Mode Commands

- `-auto` หรือ `--a`  
  → ตั้งโหมด **AUTO**
- `-manual` หรือ `--m`  
  → ตั้งโหมด **MANUAL** (และเคลียร์รีเลย์ทั้งหมดก่อน)
- `-idle` หรือ `--i`  
  → ตั้งโหมด **IDLE** (ปิดทุกอย่าง)

### Manual Relay Control (ใช้ได้เฉพาะโหมด MANUAL)

- หมอก (Mist)
   - `-mist on`
   - `-mist off`
- ปั๊มน้ำ (Water Pump)
   - `-pump on`
   - `-pump off`
- ปั๊มลม (Air Pump)
   - `-air on`
   - `-air off`

ถ้าใช้คำสั่ง on/off ในโหมดที่ไม่ใช่ MANUAL → จะขึ้นข้อความแจ้งเตือนใน Serial

### Clear All Outputs

- `-clear`
   - ปิดทุกรีเลย์ (น้ำ/หมอก/ลม) แต่ **ไม่เปลี่ยนโหมด**
   - ใช้กรณีอยากดับทุกอย่างทันที แต่ยังอยู่โหมด MANUAL ต่อไปได้

ภายใน `CommandTask.cpp` มี cooldown:

    static const uint32_t CMD_COOLDOWN_MS = 200; // กัน spam command รัวๆ

---

## Time Source & Air Pump Schedule

### Time Source

กำหนดที่ `Config.h`:

    #define USE_FAKE_TIME

- ถ้า **มี** `USE_FAKE_TIME`
   - ระบบไม่อ่าน DS3231 แต่ใช้ค่า:

      constexpr uint16_t FAKE_MINUTES_OF_DAY = 7 \* 60 + 30; // 07:30

- ถ้า **เอา `USE_FAKE_TIME` ออก**
   - ระบบจะเรียก `rtcTime.begin()` ใน `setup()`
   - ใช้ `RtcDs3231Time` (ไลบรารี RTClib) อ่าน DS3231 จริง:

      bool getMinutesOfDay(uint16_t &minutes);
      bool getTimeOfDay(TimeOfDay &out);

### Air Pump Schedule (Hardcode Version)

เวอร์ชันแรก: นิยามช่วงเวลาทำงานปั๊มลมใน `Config.h` เป็น “นาทีของวัน”:

    // ช่วง 1: 07:00–12:00
    constexpr uint16_t AIR_WINDOW1_START_MIN = 7 * 60;   // 07:00
    constexpr uint16_t AIR_WINDOW1_END_MIN   = 12 * 60;  // 12:00

    // ช่วง 2: 14:00–17:30
    constexpr uint16_t AIR_WINDOW2_START_MIN = 14 * 60;      // 14:00
    constexpr uint16_t AIR_WINDOW2_END_MIN   = 17 * 60 + 30; // 17:30

ภายใน `FarmManager` จะมีฟังก์ชันประมาณนี้:

    bool isWithinWindow(uint16_t minutes,
                        uint16_t startMin,
                        uint16_t endMin);

    bool shouldAirOn(uint16_t minutesOfDay); // ใช้ WINDOW1 + WINDOW2

จากนั้น `runAirBySchedule(minutesOfDay)` จะตัดสินใจเปิด/ปิดปั๊มลมตามเวลา ในโหมด AUTO เท่านั้น

### Air Pump Schedule (JSON Design – อนาคต)

มีแผนรองรับ config ผ่านไฟล์ `data/schedule.json` โดยใช้ LittleFS:

    {
      "air_pump": {
        "enabled": true,
        "windows": [
          { "start": "07:00", "end": "12:00" },
          { "start": "14:00", "end": "17:30" }
        ]
      }
    }

แนวคิด:

- `enabled`
   - ใช้เป็น feature flag เปิด/ปิดการอ่าน schedule จากไฟล์ โดยไม่ต้องแก้โค้ด
- `windows[]`
   - รายการช่วงเวลาที่ควรเปิดปั๊มลม (สามารถเพิ่ม/ลดได้โดยแก้ JSON)

ตำแหน่งไฟล์ (แนะนำ):

- สร้างไฟล์: `data/schedule.json`
- อัปโหลดไฟล์ระบบไฟล์ขึ้นบอร์ด:

   pio run -t uploadfs

> ตอนนี้ logic ยังใช้ค่าจาก `Config.h` เป็นหลัก  
> ส่วนอ่าน JSON จาก LittleFS จะเป็น step ถัดไป

---

## Logging & Debug Flags

ควบคุมปริมาณ log ผ่าน `Config.h`:

    #define DEBUG_BH1750_LOG   1  // log ค่าแสงจาก BH1750 ใน InputTask
    #define DEBUG_TEMP_LOG     0  // log ค่า temp (fake)
    #define DEBUG_EC_LOG       0  // เผื่อสำหรับ EC sensor ในอนาคต
    #define DEBUG_TIME_LOG     1  // log เวลา (HH:MM + minutesOfDay)
    #define DEBUG_CONTROL_LOG  1  // log จาก ControlTask (mode + relay states)

ตัวอย่างการใช้งาน:

    #if DEBUG_BH1750_LOG
    Serial.printf("[InputTask] BH1750 lux=%.1f\n", lux.value);
    #endif

    #if DEBUG_TIME_LOG
    logMinutesAsClock("TIME", minutesOfDay);
    #endif

    #if DEBUG_CONTROL_LOG
    Serial.printf("[ControlTask] mode=%d pump=%d mist=%d air=%d\n",
                  (int)status.mode,
                  waterPump.isOn() ? 1 : 0,
                  mistSystem.isOn() ? 1 : 0,
                  airPump.isOn() ? 1 : 0);
    #endif

แนวทาง:

- log ที่ “ยิงทุก loop” → ควรอยู่ใต้ `#if DEBUG_*` เสมอ
- log ที่ยิงแค่ตอน state เปลี่ยน เช่น:
   - เปลี่ยนโหมด (AUTO/MANUAL/IDLE)
   - เปิด/ปิดรีเลย์  
     สามารถปล่อยถาวรได้ เพราะไม่ spam

---

## Runtime Flow (Task Overview)

ภาพรวมการทำงานระหว่าง Task หลัก ๆ:

### InputTask

- เรียกทุก ~1s
- ทำหน้าที่:
   - อ่านค่า BH1750 ผ่าน `Esp32Bh1750Light`
   - อ่านค่า temp ผ่าน `Esp32FakeTemperature`
   - อัปเดตค่าลง `SharedState`:
      - `state.updateSensors(...)`
      - `state.updateTemperature(...)`
   - อาจพิมพ์ log ตาม `DEBUG_BH1750_LOG` / `DEBUG_TEMP_LOG`

### ControlTask

- เรียกทุก ~1s
- ทำหน้าที่:
   1. อ่านเวลาเป็น `minutesOfDay`
      - FAKE (`FAKE_MINUTES_OF_DAY`) หรือ RTC (`RtcDs3231Time`)
   2. ดึง snapshot จาก `SharedState`:
      - `SystemStatus status = state.getSnapshot();`
   3. เรียกสมองกล:
      - `manager.update(status, minutesOfDay);`
   4. sync สถานะรีเลย์กลับเข้า `SharedState`:
      - `state.updateActuators(waterPump.isOn(), mistSystem.isOn(), airPump.isOn());`
   5. พิมพ์ log ตาม `DEBUG_TIME_LOG` / `DEBUG_CONTROL_LOG`

### CommandTask

- รออ่าน Serial Input (`String input = Serial.readStringUntil('\n')`)
- แปลงคำสั่งเป็น action:
   - เปลี่ยนโหมด: `-auto`, `-manual`, `-idle`, `--a`, `--m`, `--i`
   - manual relay: `-mist/pump/air on|off`
   - clear: `-clear`
- ใช้ `SharedState` และ object รีเลย์ร่วมกับ Task อื่น โดยมี cooldown กัน spam

---

## Contact

SmartFarm_POC maintainers
