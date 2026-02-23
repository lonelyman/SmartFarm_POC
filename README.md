# SmartFarm_POC

โปรเจคต้นแบบ SmartFarm (ESP32 + FreeRTOS) — เอกสารทั้งหมดถูกรวบรวมไว้ที่ไฟล์ README หลักนี้

สรุป: README หลักนี้เป็นแหล่งข้อมูลเดียวสำหรับภาพรวมสถาปัตยกรรม แนวปฏิบัติการพัฒนา และวิธีขยายระบบ (sub-README จะเป็น pointer กลับมาที่ไฟล์นี้)

---

## Quick Start

- ติดตั้ง PlatformIO แล้วรัน build:

```bash
platformio run
```

- อัพเดต snapshot โครงสร้างโปรเจค:

```bash
tree > tree.txt
```

---

## รายการไฟล์และโฟลเดอร์สำคัญ

- `include/` : header (Interfaces, Types, Domain API, Application API, Infrastructure headers)
- `src/` : implementation (drivers, tasks, main)
- `lib/` : ไลบรารีเฉพาะโปรเจค (PlatformIO)
- `test/` : โค้ดสำหรับทดสอบ (PlatformIO Test Runner)
- `platformio.ini` : การตั้งค่า build
- `tree.txt` : snapshot ของโครงสร้างโปรเจค

---

## สถาปัตยกรรม (6-Layer Summary)

เป้าหมาย: แยก Business Logic ออกจากฮาร์ดแวร์ เพื่อให้สามารถพอร์ต logic ไปยังแพลตฟอร์มอื่นได้ง่าย

เลเยอร์หลัก:

- Interfaces: `include/interfaces` — `Types.h`, `ISensor`, `IActuator` (Pure C++)
- Domain: `include/domain` — ตรรกะธุรกิจ (StateMachine, Rules)
- Application: `include/application` — `FarmManager` ประสานการตัดสินใจ
- Infrastructure: `include/infrastructure` — `GlobalState`, `SharedState` (thread-safety, snapshot)
- Drivers: `src/drivers` และ `include/drivers` — การเข้าถึงฮาร์ดแวร์
- Tasks/Main: `src/tasks`, `src/main.cpp` — สร้าง Task, อ่านเซนเซอร์, อัพเดต SharedState

กฎสำคัญ:

- เลเยอร์บน (Interfaces/Domain/Application) ห้ามเรียกใช้ฮาร์ดแวร์โดยตรง
- Logic อยู่ที่ `Application`/`Domain` เท่านั้น — Drivers ห้ามมี business logic
- SharedState เป็น single source of truth — ใช้ mutex/semphore เพื่อความปลอดภัยของ thread

---

## แนวปฏิบัติการพัฒนา (How to Add Components)

1. กำหนด `Type` ใหม่ ถ้าจำเป็น (`include/interfaces/Types.h`)
2. สร้าง `Driver` ใน `src/drivers` ที่สืบทอดจาก `ISensor`/`IActuator` (Driver ใช้ Arduino API ได้)
3. เพิ่มตรรกะใน `include/application` หรือ `include/domain` (FarmManager)
4. ผูกใน `src/main.cpp` / `src/tasks/*` โดยอ่านค่าจาก Driver แล้วอัพเดต `SharedState`

---

## Naming & Portability

- แยกชัดว่าอะไรเป็น Generic Interface (`ISensor`) และอะไรเป็น Implementation เฉพาะ (`Bh1750Light`)
- ส่วนที่เป็น Logic/Manager ควรพอร์ตไปยัง PLC หรือระบบอื่นได้โดยแทบไม่ต้องแก้

---

## README ย่อย

เพื่อหลีกเลี่ยงข้อมูลซ้ำซ้อน ไฟล์ README ย่อยในโฟลเดอร์ต่าง ๆ (`src/README.md`, `include/README`, `lib/README`, `test/README`) ถูกย่อให้เป็น pointer สั้น ๆ กลับมาที่ README นี้ ซึ่งเป็นแหล่งข้อมูลหลัก

ถ้าต้องการให้ย้ายเนื้อหาเชิงลึกทั้งหมดจาก `src/README.md` มายังไฟล์นี้แบบครบถ้วน ให้บอกได้เลย ฉันจะเพิ่มทั้งหมด

---

Contact: SmartFarm maintainers
