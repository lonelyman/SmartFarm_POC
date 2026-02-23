# SmartFarm_POC

โปรเจคต้นแบบ SmartFarm (ESP32 + FreeRTOS) — เอกสารภาพรวมและแผนสถาปัตยกรรมของโปรเจค

นี่คือไฟล์ README ที่เป็นภาพรวมของ repository: ใช้สำหรับผู้อ่านที่ต้องการเห็นโครงสร้างโปรเจคอย่างรวดเร็วและทราบจุดประสงค์ของโฟลเดอร์ต่าง ๆ

---

## โครงสร้างโปรเจค (Project structure)

โครงสร้างด้านล่างเป็น snapshot ที่เก็บไว้ใน `tree.txt` — รันคำสั่ง `tree > tree.txt` เมื่อมีการเปลี่ยนแปลงโครงสร้าง

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

---

คำแนะนำสั้น ๆ:

- ใช้ `include/` สำหรับ header ที่แชร์ข้ามโมดูล
- วางโค้ด platform-specific ใน `src/drivers` และอิมพลีเมนต์ interfaces ใน `include/drivers`
- โครงสร้างละเอียดและแนวปฏิบัติอ่านได้ใน `src/README.md`
