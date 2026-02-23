# 🌿 SmartFarm POC Architecture Blueprint (2026)

> วิสัยทัศน์: **"สมองอิสระจากฮาร์ดแวร์"**  
> ออกแบบบน ESP32 + FreeRTOS แต่สามารถยกสมอง (Logic) ไปรันบน PLC Opta ได้โดยแก้เฉพาะส่วนฮาร์ดแวร์/ระบบปฏิบัติการ

## 1. สถาปัตยกรรม 6 เลเยอร์ (6-Layer Architecture)

| เลเยอร์            | โฟลเดอร์                         | หน้าที่หลัก                                                   | กฎเหล็ก                                                                        |
| ------------------ | -------------------------------- | ------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| **Interfaces**     | `include/interfaces`             | ภาษาสากลของระบบ: `Types.h`, `ISensor`, `IActuator`            | ❌ ห้ามมี `Arduino/FreeRTOS` <br> ✅ ต้องเป็น Pure C++                         |
| **Domain**         | `include/domain`                 | ตรรกะล้วน ๆ (เช่น `StateMachine`, Calibration, Rules)         | ❌ ห้ามเรียก Hardware API <br> ✅ ต้องรัน/ทดสอบบน PC ได้                       |
| **Application**    | `include/application`            | สมองกลาง (`FarmManager`) ประสานข้อมูลและการสั่งงาน            | ✅ รับ Snapshot (`SystemStatus`) → ตัดสินใจ → สั่งผ่าน Interfaces              |
| **Infrastructure** | `include/infrastructure`         | โครงสร้างข้อมูล + ระบบสนับสนุน (`GlobalState`, `SharedState`) | ✅ ใช้ FreeRTOS/Mutex ได้เฉพาะในเลเยอร์นี้                                     |
| **Drivers**        | `include/drivers`, `src/drivers` | คนทำงานจริงบนฮาร์ดแวร์ (`Esp32Light`, `Esp32Relay`)           | ✅ ใช้ Arduino API ได้เต็มที่ <br> ❌ ห้ามมี Business Logic (เงื่อนไขทางเกษตร) |
| **Tasks/Main**     | `src/tasks`, `src/main.cpp`      | ระบบประสาท/กาวเชื่อม (สร้าง Task, จัดลำดับการทำงาน)           | ✅ เชื่อมทุกเลเยอร์เข้าด้วยกันผ่าน Interfaces + SharedState                    |

## 2. ธรรมนูญการพัฒนา (Development Rules)

### กฎชุดที่ 1: การแยกเลเยอร์ (Layer Isolation)

- ทุกครั้งที่เพิ่มไฟล์ใหม่ ให้ถามก่อนว่า: **“สิ่งนี้อยู่เลเยอร์ไหน?”**
- เลเยอร์บน (Interfaces, Domain, Application)  
  ต้อง **ไม่รู้จัก** เลเยอร์ล่าง (Infrastructure, Drivers, Tasks) โดยตรง

````markdown
# 🌿 SmartFarm POC — Architecture Blueprint (2026)

> วิสัยทัศน์: **"สมองอิสระจากฮาร์ดแวร์"**
>
> ออกแบบบน ESP32 + FreeRTOS แต่สามารถยกสมอง (Logic) ไปรันบน PLC Opta ได้โดยแก้เฉพาะส่วนฮาร์ดแวร์/ระบบปฏิบัติการ

---

## ภาพรวมสถาปัตยกรรม: 6 เลเยอร์ (6-Layer Architecture)

ตารางสรุปเลเยอร์ของระบบ

| เลเยอร์            | โฟลเดอร์                         | หน้าที่หลัก                                                   | กฎเหล็ก                                                                     |
| ------------------ | -------------------------------- | ------------------------------------------------------------- | --------------------------------------------------------------------------- |
| **Interfaces**     | `include/interfaces`             | ภาษาสากลของระบบ: `Types.h`, `ISensor`, `IActuator`            | ❌ ห้ามมี `Arduino/FreeRTOS` • ✅ ต้องเป็น Pure C++                         |
| **Domain**         | `include/domain`                 | ตรรกะล้วน ๆ (เช่น `StateMachine`, Calibration, Rules)         | ❌ ห้ามเรียก Hardware API • ✅ ต้องรัน/ทดสอบบน PC ได้                       |
| **Application**    | `include/application`            | สมองกลาง (`FarmManager`) ประสานข้อมูลและการสั่งงาน            | ✅ รับ Snapshot (`SystemStatus`) → ตัดสินใจ → สั่งผ่าน Interfaces           |
| **Infrastructure** | `include/infrastructure`         | โครงสร้างข้อมูล + ระบบสนับสนุน (`GlobalState`, `SharedState`) | ✅ ใช้ FreeRTOS/Mutex ได้เฉพาะในเลเยอร์นี้                                  |
| **Drivers**        | `include/drivers`, `src/drivers` | คนทำงานจริงบนฮาร์ดแวร์ (`Esp32Light`, `Esp32Relay`)           | ✅ ใช้ Arduino API ได้เต็มที่ • ❌ ห้ามมี Business Logic (เงื่อนไขทางเกษตร) |
| **Tasks / Main**   | `src/tasks`, `src/main.cpp`      | ระบบประสาท/กาวเชื่อม (สร้าง Task, จัดลำดับการทำงาน)           | ✅ เชื่อมทุกเลเยอร์เข้าด้วยกันผ่าน Interfaces + SharedState                 |

---

## ธรรมนูญการพัฒนา (Development Rules)

ต่อไปนี้คือกฎและแนวปฏิบัติสำหรับการพัฒนา เพื่อรักษาโครงสร้างและความสามารถในการพอร์ต

### 1) การแยกเลเยอร์ (Layer Isolation)

- ก่อนเพิ่มไฟล์ใหม่ ให้ถามตัวเองว่า: **"สิ่งนี้อยู่เลเยอร์ไหน?"**
- เลเยอร์บน (Interfaces, Domain, Application) ต้อง **ไม่รู้จัก** เลเยอร์ล่าง (Infrastructure, Drivers, Tasks) โดยตรง
- ถ้าจำเป็นต้องสื่อสารกับเลเยอร์ล่าง ให้ทำผ่าน `Interfaces` / `Types` เท่านั้น

### 2) ภาษากลางของระบบ (Common Language)

- ใช้ `Types.h` เป็น **มาตรฐานกลาง** ระหว่างเลเยอร์ทั้งหมด
- `SensorReading` ควรมีช่องข้อมูลพื้นฐานเสมอ:
   - `value` — ค่าที่อ่านได้ (เช่น Lux, EC)
   - `isValid` — สถานะความถูกต้องของข้อมูล
   - `timestamp` — เวลาอ่านล่าสุด (หน่วยเป็น ms หรือหน่วยที่ตกลงกัน)
- ข้อมูลสถานะรวมของระบบ (SystemStatus) เป็นศูนย์กลางของข้อมูลสถานะทั้งหมด (ปัจจุบันนิยามใน `GlobalState.h` และใช้เป็น Snapshot ทั่วระบบ)

### 3) การจัดการสถานะ (State Integrity)

- **Single Source of Truth**: มีเพียงชุดสถานะรวมเดียว (`SystemStatus`) เก็บอยู่ใน `SharedState::_status`
- **Single Entry Point**: ทุก Task ต้องอ่าน/เขียนสถานะผ่านเมทอดของ `SharedState` เท่านั้น เช่น:
   - `updateSensors(...)`
   - `updateActuators(...)`
   - `getSnapshot()`
   - (อนาคต) `setMode(...)`, `getMode()`
- **Thread Safety**: การแก้ไขสถานะต้องป้องกัน race condition ด้วย Mutex (Semaphore) เสมอ
- **Snapshot Strategy**: เวลาจะส่งข้อมูลให้ `FarmManager` หรือส่วนแสดงผล ให้ใช้ `getSnapshot()` เพื่อคัดลอกข้อมูลออกมาก่อน และหลีกเลี่ยงการล็อก Mutex นาน ๆ

### 4) แยก Logic ออกจาก Hardware (Logic vs Hardware)

- **สมอง (Brain)** — อยู่ใน `Application` / `Domain` เช่น `FarmManager`, `LogicService`
   - หน้าที่: ตัดสินใจว่า **"ควรทำอะไร"** (Business logic / Policy)
   - ตัวอย่าง: ถ้าแสงต่ำกว่า 500 Lux → พ่นหมอก; ถ้าโหมด MANUAL → ห้ามให้ logic AUTO สั่งงานเอง
- **มือ/ขา (Hands)** — อยู่ใน `Drivers` เช่น `Esp32Light`, `Esp32Relay`
   - หน้าที่: ปฏิบัติการบนฮาร์ดแวร์ เช่น `turnOn()` → `digitalWrite(pin, HIGH)`, `read()` → `analogRead(pin)` แล้วแปลงหน่วย
- ห้ามใส่ค่ากำหนด/Threshold/เงื่อนไขทางธุรกิจไว้ใน Driver หรือ Task (เช่น `500.0f`, `7.0f EC`) — ค่าพวกนี้ต้องอยู่ใน `FarmManager`/`Domain`

---

## แนวทางการขยายระบบ (Scalability Guide)

เมื่อเพิ่มอุปกรณ์หรือฟีเจอร์ใหม่ ให้ทำตามขั้นตอนนี้เพื่อรักษาความชัดเจนและความสามารถในการทดสอบ/พอร์ต

1. Define Type / Status
   - ถ้าจำเป็น ให้เพิ่ม field ใหม่ใน `Types.h` หรือ `SystemStatus` (เช่น เพิ่ม `SensorReading temp` หรือสถานะ `isFanActive`)

2. Implement Driver
   - สร้างคลาสใหม่ใน `drivers/` (เช่น `Esp32Temp`, `Esp32Fan`)
   - ให้สืบทอดจาก Interface ที่เกี่ยวข้อง (`ISensor` / `IActuator`)
   - Driver ใช้ Arduino API ได้เต็มที่ แต่ห้ามใส่ business logic

3. Update Manager / Logic
   - เพิ่มตรรกะ/นโยบายใน `FarmManager` หรือไฟล์ใน `domain/`
   - ตัวอย่าง: ถ้าอุณหภูมิ > 30°C ให้เปิดพัดลม

4. Wire in Task / Main
   - ผูก Driver เข้ากับระบบใน `main.cpp` / `src/tasks/*.cpp`:
      - สร้าง instance ของ driver
      - อ่านค่าจาก driver แล้วเรียก `SharedState.updateSensors(...)`
      - ส่ง snapshot ให้ `FarmManager.update(...)` เพื่อให้ระบบตัดสินใจ

---

## เป้าหมายการพอร์ตระบบ (Portability Target)

คำแนะนำเพื่อให้ส่วนต่าง ๆ ของระบบพอร์ตได้ง่ายเมื่อย้ายไปแพลตฟอร์มอื่น (เช่น PLC Opta)

### สิ่งที่ "ยกไป PLC Opta ได้ตรง ๆ"

- Interfaces Layer: `Types.h`, `ISensor`, `IActuator`
- Logic / Manager: `FarmManager` และไฟล์ใน `domain/` (ตรรกะที่ไม่พึ่งพาฮาร์ดแวร์)

แนวคิด: แปลงส่วนที่เป็น logic ให้เป็น Function Block หรือ Library ฝั่ง PLC ได้ง่าย

### สิ่งที่ต้อง "เขียนใหม่/ปรับใหม่" บน PLC

- Drivers: เขียนใหม่ให้สอดคล้องกับ IO/Library ของ PLC (เช่น `OptaRelay`, `OptaAnalogInput`)
- SharedState / Infra: เปลี่ยนจาก FreeRTOS Mutex ไปเป็นกลไกที่เหมาะสมบน PLC (เช่น Global Variables + Task Model)
- Tasks / Main: แปลงจาก FreeRTOS Task เป็น Task/Program/FB ของ PLC

สรุปสั้น ๆ:

> **"ของที่รู้จักฮาร์ดแวร์/OS = เขียนใหม่"**
>
> **"ของที่คิด/ตัดสินใจด้วยข้อมูลล้วน ๆ = ยกไปใช้ต่อได้"**

---

## Naming Rule (แนวทางการตั้งชื่อ)

1. Interface / Type กลาง
   - ใช้ชื่อกลาง ๆ เช่น `ISensor`, `IActuator`, `ILogger`, `SystemStatus`
   - ห้ามผูกชื่อกับยี่ห้อ/รุ่น/บอร์ด

2. Implementation ที่ผูกกับรุ่น/ยี่ห้อ/โปรโตคอลเฉพาะ
   - ถ้าใช้ BH1750 จริง → ใช้ชื่อ `Bh1750LightSensor`
   - ถ้าใช้ relay GPIO active-high → ใช้ชื่อเช่น `GpioRelayActiveHigh`
   - ห้ามตั้งชื่อให้ดู generic ถ้ามัน "สลับอะไรไม่ได้"

3. Implementation ที่ออกแบบให้สลับวิธี/รุ่นได้หลายแบบจริง ๆ
   - ตัวอย่าง: adapter ที่ข้างในเลือกได้ว่าจะใช้ BH1750 หรือ VEML7700 ตาม config
   - ใช้ชื่อกึ่ง generic เช่น `I2cLightSensor` หรือ `ConfigurableLightSensor`

---

## Project tree (quick reference)

```plaintext
.
├── include
│   ├── README
│   ├── application
│   │   └── FarmManager.h
│   ├── domain
│   ├── drivers
│   │   ├── Esp32Bh1750Light.h
│   │   └── Esp32Relay.h
│   ├── infrastructure
│   │   ├── GlobalState.h
│   │   └── SharedState.h
│   └── interfaces
│       ├── IActuator.h
│       ├── ISensor.h
│       └── Types.h
├── lib
│   └── README
├── platformio.ini
├── src
│   ├── README.md
│   ├── application
│   ├── domain
│   ├── drivers
│   │   ├── Esp32Bh1750Light.cpp
│   │   └── Esp32Relay.cpp
│   ├── infrastructure
│   ├── main.cpp
│   └── tasks
│       ├── CommandTask.cpp
│       └── SensorTasks.cpp
├── test
│   └── README
└── tree.txt

15 directories, 19 files
```
````

Source of truth: `tree.txt` — รัน `tree > tree.txt` เมื่อโครงสร้างเปลี่ยน

```

```
