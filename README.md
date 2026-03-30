# SmartFarm POC

ระบบควบคุมฟาร์มอัจฉริยะสำหรับ ESP32-S3 พัฒนาด้วย PlatformIO + Arduino Framework
ออกแบบตาม Clean Architecture — แยก Domain / Application / Infrastructure ออกจากกัน ทดสอบได้โดยไม่ต้องมี hardware

---

## สารบัญ

1. [ภาพรวมระบบ](#ภาพรวมระบบ)
2. [Hardware](#hardware)
3. [โครงสร้างโปรเจกต์](#โครงสร้างโปรเจกต์)
4. [Architecture](#architecture)
5. [การทำงานของระบบ (Tasks)](#การทำงานของระบบ-tasks)
6. [Sensors & Actuators](#sensors--actuators)
7. [GPIO Pin Map](#gpio-pin-map)
8. [Control Logic](#control-logic)
9. [Web UI & API](#web-ui--api)
10.   [Serial CLI](#serial-cli)
11.   [Network & WiFi](#network--wifi)
12.   [Air Pump Schedule](#air-pump-schedule)
13.   [การติดตั้งและ Build](#การติดตั้งและ-build)
14.   [Configuration](#configuration)
15.   [Unit Tests](#unit-tests)
16.   [สิ่งที่ยังไม่ implement (TODO)](#สิ่งที่ยังไม่-implement-todo)

---

## ภาพรวมระบบ

SmartFarm POC ควบคุมอุปกรณ์ในฟาร์ม 3 ชุดหลัก:

| อุปกรณ์                    | การควบคุม                                            |
| -------------------------- | ---------------------------------------------------- |
| **Water Pump** (ปั๊มน้ำ)   | Manual / Auto (logic ยังไม่ implement)               |
| **Mist System** (ปั๊มหมอก) | Auto (อุณหภูมิ + ความชื้น + hysteresis + time guard) |
| **Air Pump** (ปั๊มลม)      | ตามตาราง schedule (JSON)                             |

ระบบรองรับ 3 โหมด:

- **IDLE** — ปิดทุกอย่าง (safe state)
- **AUTO** — ระบบตัดสินใจเองจากเซนเซอร์
- **MANUAL** — สั่งงานตรงผ่าน physical switch / Serial CLI / Web UI

---

## Hardware

| รายการ        | รายละเอียด                                |
| ------------- | ----------------------------------------- |
| **MCU**       | ESP32-S3-WROOM-1 N16R8 (44-Pin DevKitC-1) |
| **Flash**     | 16 MB QIO                                 |
| **PSRAM**     | 8 MB Octal (OPI)                          |
| **Framework** | Arduino via PlatformIO                    |
| **OS**        | FreeRTOS (built-in)                       |

### ข้อควรระวังด้าน GPIO (N16R8)

- **GPIO 26–37** — ถูก reserve โดย Flash/PSRAM ห้ามใช้
- **GPIO 0** — Strapping pin ต้องมี pull-up 10kΩ ไปยัง 3.3V
- **GPIO 19–20** — USB D−/D+ ห้ามต่ออุปกรณ์ภายนอก
- **GPIO 39** — Input only ห้ามใช้เป็น output

---

## โครงสร้างโปรเจกต์

```
SmartFarm_POC/
├── include/
│   ├── Config.h                    # GPIO pins, timing, thresholds ทั้งหมด
│   ├── interfaces/                 # Abstract interfaces (ISensor, IActuator, ฯลฯ)
│   ├── domain/                     # Domain logic (TimeSchedule, TimeWindow)
│   ├── application/                # Business logic (FarmManager, ScheduledRelay)
│   ├── drivers/                    # Hardware drivers (ESP32-specific)
│   ├── infrastructure/             # System glue (SharedState, AppBoot, WebUI, ฯลฯ)
│   └── tasks/                      # FreeRTOS task entry points
├── src/
│   ├── main.cpp                    # Composition Root — wire objects เข้าหากัน
│   └── (mirrors include/ structure)
├── data/
│   ├── schedule.json               # ตาราง air pump (อัปโหลดไปยัง LittleFS)
│   ├── wifi.json                   # WiFi credentials (อัปโหลดไปยัง LittleFS)
│   └── www/                        # Web UI (dashboard, wifi setup)
├── test/
│   └── test_farmmanager.cpp        # Unit tests (Unity framework)
├── default_16MB.csv                # Partition table 16MB
└── platformio.ini
```

---

## Architecture

ระบบออกแบบตาม **Clean Architecture** แบ่งชั้นอย่างเคร่งครัด โดยมีกฎหลักคือ **ชั้นในไม่รู้จักชั้นนอก** — Domain และ Application layer ไม่มีการ include Arduino API, driver, หรือ infrastructure ใด ๆ ทำให้สามารถทดสอบ business logic บน host ได้โดยไม่ต้องมี hardware

```
╔══════════════════════════════════════════════════════╗
║  interfaces/  (Types.h, ISensor, IActuator, IClock…) ║  ← ชั้นในสุด
║  ไม่มี Arduino API — compile ได้บน host/desktop     ║
╠══════════════════════════════════════════════════════╣
║  domain/  (TimeSchedule, TimeWindow)                 ║
║  Pure logic — ไม่มี I/O, ไม่มี hardware              ║
╠══════════════════════════════════════════════════════╣
║  application/  (FarmManager, ScheduledRelay,         ║
║                 CommandService, FarmModels)           ║
║  Business rules — ใช้ interfaces เท่านั้น            ║
╠══════════════════════════════════════════════════════╣
║  drivers/                  infrastructure/           ║
║  (Esp32Relay, Esp32Sht40,  (SharedState, RtcClock,   ║  ← ชั้นนอกสุด
║   Esp32Bh1750Light, …)      AppBoot, Esp32WebUi, …)  ║
╠══════════════════════════════════════════════════════╣
║  tasks/  (inputTask, controlTask, networkTask, …)    ║
║  main.cpp  (Composition Root)                        ║
╚══════════════════════════════════════════════════════╝
```

---

### กฎข้อบังคับของแต่ละชั้น

#### ชั้น interfaces/ + Types.h — กฎเหล็กสูงสุด

ไฟล์ในชั้นนี้คือสัญญากลางของระบบทั้งหมด ทุกชั้นอื่นอ้างอิงมาที่นี่ได้ แต่ที่นี่ต้องไม่อ้างอิงใคร

| กฎ                                         | รายละเอียด                                                 |
| ------------------------------------------ | ---------------------------------------------------------- |
| ❌ ห้าม include Arduino.h                  | ทำให้ชั้นนี้ขึ้นอยู่กับ framework — ทดสอบบน host ไม่ได้    |
| ❌ ห้าม include driver หรือ infrastructure | ทำให้ชั้นใน depend on ชั้นนอก — ผิดหลัก Clean Architecture |
| ✅ include ได้เฉพาะ stdint.h / cstdint     | standard C++ เท่านั้น                                      |
| ✅ ทุก interface ต้องมี virtual destructor | ป้องกัน memory leak เมื่อ delete ผ่าน pointer              |

**Interfaces ที่มีในระบบ:**

| Interface        | หน้าที่                                            | Implementation                   |
| ---------------- | -------------------------------------------------- | -------------------------------- |
| `ISensor`        | อ่านค่าเซนเซอร์ (`read()`, `begin()`)              | `Esp32Bh1750Light`, `Esp32Sht40` |
| `IActuator`      | สั่งงาน output (`turnOn()`, `turnOff()`, `isOn()`) | `Esp32Relay`                     |
| `IClock`         | อ่าน/ตั้งเวลา + sync NTP                           | `RtcClock`                       |
| `INetwork`       | WiFi AP/STA management                             | `Esp32WiFiNetwork`               |
| `IModeSource`    | อ่าน SystemMode จาก physical switch                | `Esp32ModeSwitchSource`          |
| `INetModeSource` | อ่าน AP/STA mode จาก physical switch               | `Esp32NetModeSwitch`             |
| `ISchedule`      | ตรวจสอบ time window (`isInWindow()`)               | `TimeSchedule`                   |
| `IUi`            | Web UI lifecycle (`begin()`, `tick()`)             | `Esp32WebUi`                     |

**Types.h — Domain types ที่ใช้ร่วมกันทั้งระบบ:**

- `SystemMode` — `IDLE` / `AUTO` / `MANUAL`
- `SensorReading` — value + isValid + timestamp
- `SystemStatus` — snapshot ของ sensor + actuator ทั้งหมด
- `ManualOverrides` — คำสั่ง manual จากผู้ใช้
- `WaterLevelSensors`, `WaterTempReading`, `TimeOfDay`

---

#### ชั้น domain/ — Pure Logic ปลอด Hardware

Domain คือ "กฎทางธุรกิจ" ที่ไม่ขึ้นกับว่าจะรันบน ESP32 หรือ PC กฎนี้เปลี่ยนเฉพาะเมื่อ "โลกความเป็นจริงของฟาร์ม" เปลี่ยน ไม่ใช่เพราะเปลี่ยน library หรือ hardware

| กฎ                                                  | รายละเอียด                              |
| --------------------------------------------------- | --------------------------------------- |
| ❌ ห้าม include Arduino.h หรือ FreeRTOS             | domain logic ต้องทดสอบได้บน host        |
| ❌ ห้าม include driver, infrastructure, หรือ task   | ชั้น domain ไม่รู้จักว่าตัวเองรันบนอะไร |
| ✅ include ได้เฉพาะ interfaces/                     | ใช้ผ่าน abstraction เท่านั้น            |
| ✅ ต้องทดสอบด้วย unit test ได้โดยไม่ต้องมี hardware | นี่คือเหตุผลหลักที่แยกชั้นนี้ออกมา      |

**Classes ในชั้น domain:**

- **`TimeWindow`** — โครงสร้างข้อมูล `[startMin, endMin)` ของช่วงเวลา 1 รอบ
- **`TimeSchedule`** — รายการ `TimeWindow` สูงสุด 24 ช่วง implement `ISchedule` — เมธอด `isInWindow(minutesOfDay)` วนลูปตรวจสอบทุก window, `addWindow()` / `clear()` / `setEnabled()` ไม่มี I/O ใด ๆ

---

#### ชั้น application/ — Business Rules

Application layer คือ "สมองของระบบ" รู้จักเฉพาะ domain types และ interfaces ไม่รู้ว่า hardware ตัวจริงคืออะไร

| กฎ                                                           | รายละเอียด                                                        |
| ------------------------------------------------------------ | ----------------------------------------------------------------- |
| ❌ ห้าม include driver โดยตรง                                | ใช้ผ่าน interface เท่านั้น เช่น `IActuator&` ไม่ใช่ `Esp32Relay&` |
| ❌ ห้าม include infrastructure (SharedState, AppBoot, WebUI) | application ไม่รู้จัก threading model                             |
| ✅ include ได้เฉพาะ interfaces/ และ domain/                  |                                                                   |
| ✅ ทุก dependency ต้อง inject เข้ามา (DI)                    | ไม่มี global state ภายใน class                                    |
| ✅ ทดสอบได้ด้วย mock objects                                 | แทน hardware ด้วย `MockActuator` ใน unit test                     |

**Classes ในชั้น application:**

**`FarmManager`** — ตัดสินใจเรื่อง water pump และ mist system

- รับ `FarmInput` (mode, sensor values, manual overrides) คืน `FarmDecision` (pumpOn, mistOn)
- ไม่รู้จัก SharedState, GPIO, หรือ FreeRTOS
- state ภายใน (hysteresis latch, boot guard, mist guard) เป็นของตัวเอง ไม่ใช้ global

**`ScheduledRelay`** — เชื่อม `ISchedule` กับ `IActuator`

- รับ `IActuator&` และ `ISchedule&` ผ่าน constructor (Dependency Injection)
- เมธอด `update(minutesOfDay)` — ถาม schedule ว่าควรเปิดไหม แล้วสั่ง actuator
- เพิ่ม device ใหม่ที่ทำงานตามตารางได้โดยไม่แก้ FarmManager เลย

**`CommandService`** — รวม command ทุกประเภทไว้ที่เดียว

- รับ `SystemContext&` — เป็น exception ที่รับ infrastructure เพราะเป็นตัวกลาง CLI/WebUI
- แต่ละ method ทำสิ่งเดียวชัดเจน: `relayPump(bool)`, `setModeAuto()`, `netOn()`, `clockSet()`
- ตรวจ guard ก่อนทำงาน เช่น `relayPump()` จะ ignore ถ้าไม่ได้อยู่ใน MANUAL mode

**`FarmInput` / `FarmDecision`** — Data Transfer Objects

- `FarmInput` — รวม sensor snapshot + mode + manual overrides ส่งให้ FarmManager
- `FarmDecision` — ผลลัพธ์ที่ FarmManager ตัดสินใจ (pumpOn, mistOn เท่านั้น — airOn ถูกตัดออก เพราะ ScheduledRelay เป็นเจ้าของ)

---

#### ชั้น drivers/ — Hardware Abstraction

Driver คือ "ล่ามระหว่าง interface กับ hardware จริง" แต่ละ driver implement interface ที่ตรงกัน

| กฎ                                              | รายละเอียด                                                           |
| ----------------------------------------------- | -------------------------------------------------------------------- |
| ✅ ต้อง implement interface ที่ตรงกันเสมอ       | `Esp32Relay` implement `IActuator`, `Esp32Sht40` implement `ISensor` |
| ❌ ห้าม call `Wire.begin()` ใน driver           | I2C init เป็นหน้าที่ของ `AppBoot::initI2C()` เท่านั้น                |
| ✅ อ่านค่า config จาก Config.h ได้              | เช่น `RELAY_ACTIVE_LOW`, `WATER_LEVEL_ALARM_LOW`                     |
| ✅ มี `begin()` สำหรับ init resource ครั้งเดียว | เรียกจาก `AppBoot::initDrivers()`                                    |
| ❌ ห้ามมี business logic                        | driver ทำแค่ "แปลง interface call เป็น GPIO/I2C/1-Wire"              |

**Drivers ที่มีในระบบ:**

| Driver                 | Implements       | Protocol | หมายเหตุ                                              |
| ---------------------- | ---------------- | -------- | ----------------------------------------------------- |
| `Esp32Relay`           | `IActuator`      | GPIO     | แปลง active HIGH/LOW ตาม `RELAY_ACTIVE_LOW` อัตโนมัติ |
| `Esp32Bh1750Light`     | `ISensor`        | I2C      | คืน lux เป็น `SensorReading`                          |
| `Esp32Sht40`           | `ISensor`        | I2C      | temp ผ่าน `read()`, humidity ผ่าน `getLastHumidity()` |
| `Esp32Ds18b20`         | — (raw)          | 1-Wire   | รองรับหลาย sensor บน bus เดียว                        |
| `Esp32WaterLevelInput` | — (raw)          | Digital  | normalize ขั้ว `WATER_LEVEL_ALARM_LOW` อัตโนมัติ      |
| `Esp32ManualSwitch`    | — (raw)          | Digital  | INPUT_PULLUP + debounce                               |
| `Esp32NetModeSwitch`   | `INetModeSource` | Digital  | INPUT_PULLUP + debounce                               |
| `RtcDs3231Time`        | — (raw)          | I2C      | อ่าน/เขียนเวลากับ DS3231                              |

---

#### ชั้น infrastructure/ — System Glue

Infrastructure คือ "กาวที่ประกอบทุกอย่างเข้าด้วยกัน" รู้จักทุกชั้น แต่ถูกรู้จักจากน้อยที่สุด

| กฎ                                                       | รายละเอียด                                                              |
| -------------------------------------------------------- | ----------------------------------------------------------------------- |
| ✅ include ได้จากทุกชั้น                                 | infrastructure เห็นทุกอย่าง                                             |
| ❌ ห้าม business logic ใน AppBoot                        | AppBoot ทำแค่ init และ wire — ไม่ตัดสินใจเรื่อง pump หรือ sensor        |
| ✅ SharedState เป็น single source of truth ระหว่าง tasks | ทุก task อ่าน/เขียนผ่าน SharedState เท่านั้น                            |
| ❌ ห้าม task คุยกันโดยตรง                                | ห้าม pass pointer ของ task หนึ่งให้อีก task หนึ่งโดยไม่ผ่าน SharedState |

**Classes หลักในชั้น infrastructure:**

**`SharedState`** — Thread-safe state ระหว่าง FreeRTOS tasks

- ป้องกัน race condition ด้วย `SemaphoreHandle_t _mutex`
- Pattern การเขียน: แต่ละ task มีหน้าที่ชัดเจน — `inputTask` เขียน sensor, `controlTask` เขียน actuator state, `commandTask`/WebUI เขียน command
- **Pending Request Pattern** — ผู้ส่งเรียก `request*()` set pending=true, ผู้รับเรียก `consume*()` copy + clear ใน call เดียว ป้องกันการประมวลผลซ้ำ

**`SystemContext`** — Object graph holder ส่งเข้า FreeRTOS task แทน global variable

- เป็น struct ธรรมดา ไม่มี method — ทำหน้าที่แค่รวม pointer ทุกตัวไว้ด้วยกัน
- สร้างใน `main.cpp` และส่งเป็น `void*` ผ่าน `xTaskCreatePinnedToCore`

**`AppBoot`** — Orchestrator การ boot

- ลำดับ init สำคัญมาก — `initRelayPins()` ต้องก่อน `initDrivers()` เสมอ (ป้องกัน boot glitch)
- ไม่มี logic อื่น — เรียก init function ตามลำดับแล้ว start tasks

**`RtcClock`** — Facade ครอบ `RtcDs3231Time` + `NetTimeSync`

- implement `IClock` ให้ application layer ใช้งานโดยไม่รู้ว่าข้างในเป็น DS3231

**`Esp32WebUi`** — Web server + REST API

- Lazy start — server.begin() เมื่อมี IP address เท่านั้น
- ใช้ `WebUiTask` เรียก `tick()` แยก core เพื่อไม่บล็อก sensor/control tasks

---

#### ชั้น tasks/ + main.cpp — Wiring & Runtime

| กฎ                                      | รายละเอียด                                                               |
| --------------------------------------- | ------------------------------------------------------------------------ |
| ❌ ห้าม business logic ใน main.cpp      | main.cpp ทำแค่สร้าง object และ wire — ไม่ตัดสินใจเรื่อง sensor หรือ pump |
| ❌ ห้าม task คุยกันโดยตรง               | ทุกการสื่อสารผ่าน SharedState เท่านั้น                                   |
| ✅ แต่ละ task มีหน้าที่เดียว            | inputTask อ่านเท่านั้น, controlTask ตัดสินใจ+สั่งเท่านั้น                |
| ✅ task ใช้ `SystemContext*` แทน global | ทดสอบได้ง่าย เปลี่ยน dependency ได้โดยไม่แก้ task                        |

**Data flow ระหว่าง tasks:**

```
inputTask ──write──▶ SharedState ◀──read── controlTask ──write──▶ SharedState
                         ▲                      │
              read/write │                      ▼
          commandTask / webUiTask            hardware
          (CLI / Web API)                  (relay GPIO)
                         ▲
              read/write │
                   networkTask
                  (WiFi state)
```

---

### สรุปกฎทิศทาง Dependency

```
main.cpp / tasks
    ↓ ใช้
infrastructure / drivers
    ↓ implement
interfaces
    ↑ ใช้
application / domain
```

**ทิศทางที่ต้องห้ามเด็ดขาด:**

- ❌ `domain` → `drivers` หรือ `infrastructure`
- ❌ `application` → `Esp32Relay` (ต้องผ่าน `IActuator`)
- ❌ `interfaces/Types.h` → Arduino.h
- ❌ task หนึ่ง → task อื่นโดยตรง (ต้องผ่าน SharedState)

---

## การทำงานของระบบ (Tasks)

ระบบใช้ **FreeRTOS** ทำงานบน 5 tasks:

| Task          | Core | Priority | หน้าที่                                         | Interval         |
| ------------- | ---- | -------- | ----------------------------------------------- | ---------------- |
| `inputTask`   | 1    | 1        | อ่าน sensor + switch ทุกรอบ → เขียน SharedState | 1,000 ms         |
| `controlTask` | 1    | 2        | อ่าน SharedState → FarmManager → สั่ง actuator  | 1,000 ms         |
| `commandTask` | 0    | 1        | รับ Serial CLI command                          | 50 ms            |
| `networkTask` | 0    | 1        | จัดการ WiFi STA/AP state machine                | 20 ms            |
| `webUiTask`   | 1    | 1        | serve Web UI (lazy start เมื่อมี IP)            | ตาม handleClient |

### ลำดับ Boot (AppBoot::setup)

1. `initRelayPins()` — ตั้ง relay เป็น inactive ก่อน (ป้องกัน boot glitch)
2. `initI2C()`
3. `initFileSystemAndSchedule()` — mount LittleFS + โหลด `schedule.json`
4. `initModeSource()` / `initNetModeSource()`
5. `initClock()` — RTC DS3231
6. `initDrivers()` — sensor + actuator ทุกตัว
7. `initNetwork()` — WiFi begin
8. `setInitialSafeState()` — mode = IDLE
9. `startTasks()` — สร้าง FreeRTOS tasks

---

## Sensors & Actuators

### Sensors

| Sensor      | Driver                 | Interface | หมายเหตุ                   |
| ----------- | ---------------------- | --------- | -------------------------- |
| **BH1750**  | `Esp32Bh1750Light`     | I2C       | ความเข้มแสง (lux)          |
| **SHT40**   | `Esp32Sht40`           | I2C       | อุณหภูมิ + ความชื้น        |
| **DS18B20** | `Esp32Ds18b20`         | 1-Wire    | อุณหภูมิน้ำ (สูงสุด 4 ตัว) |
| **XKC-Y25** | `Esp32WaterLevelInput` | Digital   | ระดับน้ำ 2 ช่อง (CH1, CH2) |
| **DS3231**  | `RtcDs3231Time`        | I2C       | Real-time clock            |

### Actuators

| Actuator    | Driver       | GPIO |
| ----------- | ------------ | ---- |
| Water Pump  | `Esp32Relay` | 38   |
| Mist System | `Esp32Relay` | 40   |
| Air Pump    | `Esp32Relay` | 41   |

### Switches

| Switch      | Driver                  | GPIO | หน้าที่                      |
| ----------- | ----------------------- | ---- | ---------------------------- |
| Manual Pump | `Esp32ManualSwitch`     | 18   | สั่ง pump ใน MANUAL mode     |
| Manual Mist | `Esp32ManualSwitch`     | 5    | สั่ง mist ใน MANUAL mode     |
| Manual Air  | `Esp32ManualSwitch`     | 15   | สั่ง air pump ใน MANUAL mode |
| Mode A/B    | `Esp32ModeSwitchSource` | 6, 7 | เลือก AUTO / MANUAL / IDLE   |
| Net Switch  | `Esp32NetModeSwitch`    | 39   | เลือก AP / STA mode          |

---

## GPIO Pin Map

```
I2C Bus
  SDA  = GPIO 8
  SCL  = GPIO 9

1-Wire
  DATA = GPIO 4   (DS18B20, ต้องมี pull-up 4.7kΩ)

Relay Outputs (Active LOW)
  Water Pump = GPIO 38
  Mist       = GPIO 40
  Air Pump   = GPIO 41

Manual Switches (INPUT_PULLUP, Active LOW)
  Pump = GPIO 18
  Mist = GPIO 5
  Air  = GPIO 15

Mode Switch
  Mode A     = GPIO 6
  Mode B     = GPIO 7
  Net Switch = GPIO 39  (Input Only)
  Reserved   = GPIO 42

Water Level Sensors (XKC-Y25, INPUT_PULLUP)
  CH1 = GPIO 1
  CH2 = GPIO 2

Water Level Alarm LEDs
  CH1 LED = GPIO 47
  CH2 LED = GPIO 10
```

---

## Control Logic

### FarmManager — Brain ของระบบ

`FarmManager::update(FarmInput)` รับ input จาก SharedState และคืน `FarmDecision` ทุก 1 วินาที

**ลำดับการตัดสินใจ:**

```
mode == IDLE   → ปิดทุกอย่าง, reset state
mode == MANUAL → ทำตาม ManualOverrides
mode == AUTO   → applyAuto() ด้านล่าง
```

### Mist System — AUTO Logic

```
1. Boot Guard (10 วินาที) → ปิด mist ก่อนเสมอ
2. คำนวณ sensorWantsOn:
   - มีทั้ง temp และ hum → decideMistByTempAndHumidity()
   - มีแค่ hum             → decideMistByHumidity()
   - มีแค่ temp            → decideMistByTemp()
3. Confirmation Counter (ป้องกัน spike)
   - ต้องเห็นสัญญาณเดิม ≥ 3 รอบติดต่อกัน
4. MistGuard (time guard)
   - เปิดได้สูงสุด 3 นาที (MIST_MAX_ON_MS)
   - พักอย่างน้อย 10 นาที (MIST_MIN_OFF_MS)
```

### Hysteresis Thresholds

| Parameter | ON เมื่อ | OFF เมื่อ |
| --------- | -------- | --------- |
| อุณหภูมิ  | ≥ 32 °C  | ≤ 29 °C   |
| ความชื้น  | ≤ 65 %RH | ≥ 75 %RH  |

> **Dead-band:** ค่าระหว่างเส้น ON/OFF → คงสถานะเดิม (hysteresis latch)

### Water Pump — AUTO Logic

ยังไม่ implement — อยู่ใน TODO

### Air Pump — Schedule-based

แยกออกจาก FarmManager — จัดการโดย `ScheduledRelay` ตาม `schedule.json`

---

## Web UI & API

Web server ทำงานบน port 80 (lazy start — เริ่มเมื่อมี IP address)

### Pages

| URL           | หน้าที่                                             |
| ------------- | --------------------------------------------------- |
| `/`           | Dashboard แสดงสถานะ sensor + actuator แบบ real-time |
| `/wifi`       | ตั้งค่า WiFi (SSID / Password)                      |
| `/wifi-saved` | หน้ายืนยันหลังบันทึก WiFi                           |

### REST API

| Method | Endpoint      | หน้าที่                               |
| ------ | ------------- | ------------------------------------- |
| GET    | `/api/status` | JSON สถานะระบบทั้งหมด                 |
| POST   | `/api/mode`   | เปลี่ยน mode (`auto`/`manual`/`idle`) |
| POST   | `/api/relay`  | สั่ง relay (MANUAL mode เท่านั้น)     |
| POST   | `/api/net`    | สั่ง network (`on`/`off`/`ntp`)       |
| POST   | `/api/time`   | ตั้งเวลา RTC (`HH:MM[:SS]`)           |
| POST   | `/api/wifi`   | บันทึก WiFi config + restart          |

### Network Modes

ระบบ boot เข้า **AP mode** เสมอ (offline-first):

- **AP** → `192.168.4.1` (hotspot ชื่อ `smartfarm`)
- **STA** → เชื่อม WiFi ที่บันทึกไว้ (AP ยังทำงานควบคู่)
- Timeout STA 15 วินาที → fallback กลับ AP

---

## Serial CLI

Baud rate: **115200**  
Prefix: **`sm`**

```
sm help                     แสดง command ทั้งหมด
sm status                   แสดงสถานะระบบ

sm mode auto                เปลี่ยนเป็น AUTO mode
sm mode manual              เปลี่ยนเป็น MANUAL mode
sm mode idle                เปลี่ยนเป็น IDLE mode

sm relay pump on|off        สั่ง water pump (MANUAL mode เท่านั้น)
sm relay mist on|off        สั่ง mist system (MANUAL mode เท่านั้น)
sm relay air  on|off        สั่ง air pump (MANUAL mode เท่านั้น)

sm net on                   เชื่อม WiFi (STA mode)
sm net off                  ตัด WiFi กลับ AP-only
sm net ntp                  sync เวลาจาก NTP (ต้อง STA connected)

sm clock set HH:MM          ตั้งเวลา RTC
sm clock set HH:MM:SS       ตั้งเวลา RTC (ระบุวินาที)

sm clear                    ล้าง manual overrides
```

---

## Network & WiFi

### บันทึก WiFi Credentials

1. เปิด browser → `http://192.168.4.1/wifi`
2. กรอก SSID และ Password → Save
3. บอร์ด restart อัตโนมัติ
4. สลับ Net Switch → STA หรือใช้ `sm net on` เพื่อเชื่อม

### NTP Sync

```bash
sm net on       # เชื่อม WiFi ก่อน
sm net ntp      # sync เวลาจาก pool.ntp.org (UTC+7)
```

---

## Air Pump Schedule

แก้ไขไฟล์ `data/schedule.json` แล้วอัปโหลดด้วย `pio run --target uploadfs`

```json
{
   "air_pump": {
      "enabled": true,
      "windows": [
         { "start": "07:00", "end": "08:00" },
         { "start": "09:00", "end": "10:00" },
         { "start": "11:00", "end": "12:00" }
      ]
   }
}
```

- กำหนด time window ได้หลายช่วงต่อวัน
- ตั้ง `"enabled": false` เพื่อหยุดการทำงานทั้งหมดโดยไม่ต้องลบ schedule

---

## การติดตั้งและ Build

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension หรือ CLI)
- ESP32-S3-DevKitC-1 (N16R8)
- USB cable (data)

### Build & Upload

```bash
# Build firmware
pio run

# Upload firmware
pio run --target upload

# Upload filesystem (Web UI + schedule.json + wifi.json)
pio run --target uploadfs

# Serial monitor
pio device monitor
```

### ครั้งแรก (First Boot)

1. `pio run --target uploadfs` — อัปโหลด LittleFS ก่อน
2. `pio run --target upload` — อัปโหลด firmware
3. เปิด Serial Monitor → รอ `🚀 SmartFarm System: Ready.`
4. เชื่อม WiFi `smartfarm` → ตั้งค่าผ่าน `http://192.168.4.1/wifi`

---

## Configuration

ตั้งค่าทั้งหมดอยู่ใน **`include/Config.h`** — แก้ไขแล้วคอมไพล์ใหม่:

| Section           | สิ่งที่ตั้งได้                        |
| ----------------- | ------------------------------------- |
| I2C               | SDA/SCL pins                          |
| 1-Wire            | DATA pin, จำนวน DS18B20 สูงสุด        |
| Relay             | Active HIGH/LOW, GPIO pins            |
| Manual Switches   | GPIO pins                             |
| Mode/Net Switches | GPIO pins                             |
| Water Level       | GPIO pins, alarm logic                |
| Alarm LEDs        | GPIO pins, active logic               |
| Debounce          | ปุ่ม, net switch                      |
| NTP               | server, timezone (UTC+7)              |
| Mist Thresholds   | temp/humidity ON/OFF                  |
| Mist Time Guard   | max ON duration, min OFF duration     |
| Mist Confirmation | รอบที่ต้องยืนยันก่อนเปิด/ปิด          |
| Task Config       | stack size, interval, baud rate       |
| Boot Guard        | delay หลัง boot ก่อนเริ่ม AUTO        |
| Feature Flags     | เปิด/ปิด water level sensor แต่ละช่อง |
| Debug Logs        | เปิด/ปิด log แต่ละ module             |

---

## Unit Tests

ใช้ **Unity** framework ทดสอบ FarmManager และ ScheduledRelay โดยไม่ต้องมี hardware

```bash
# รัน test บน device
pio test
```

### Test ที่มี

- `test_idle_resets_actuators` — IDLE ต้องปิดทุกอย่างและ reset state
- `test_auto_mist_temp_hysteresis` — hysteresis ทำงานถูกต้อง
- `test_manual_mode_direct_control` — MANUAL mode ทำตาม override โดยตรง
- `test_scheduled_relay_*` — ScheduledRelay ทำงานตาม time window

---

## สิ่งที่ยังไม่ implement (TODO)

- **Water pump AUTO logic** — ปัจจุบัน `pumpOn = false` เสมอใน AUTO mode
- **EC Sensor** — มี field `ec` ใน SystemStatus แต่ยังไม่มี driver
- **Water level → pump interlock** — หยุดปั๊มเมื่อน้ำต่ำ
- **OTA Update** — อัปเดต firmware ผ่าน WiFi
- **Logging / History** — บันทึกค่า sensor ย้อนหลัง

---

## Libraries

| Library                          | Version | หน้าที่               |
| -------------------------------- | ------- | --------------------- |
| claws/BH1750                     | ^1.3.0  | Light sensor          |
| adafruit/RTClib                  | ^2.1.1  | DS3231 RTC            |
| bblanchon/ArduinoJson            | ^7.0.0  | JSON parsing/building |
| adafruit/Adafruit SHT4x          | ^1.0.4  | Temp/Humidity sensor  |
| adafruit/Adafruit Unified Sensor | ^1.1.14 | Sensor abstraction    |
| paulstoffregen/OneWire           | ^2.3.8  | 1-Wire protocol       |
| milesburton/DallasTemperature    | ^3.11.0 | DS18B20               |
