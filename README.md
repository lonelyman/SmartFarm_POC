# 🌱 SmartFarm POC

SmartFarm เป็นระบบควบคุมฟาร์มอัตโนมัติที่ออกแบบให้ **แยก business logic ออกจาก
hardware** เพื่อให้สามารถ port ไป platform อื่น เช่น PLC หรือ Linux ได้ในอนาคต

ใช้

- ESP32‑S3
- FreeRTOS
- Clean Architecture

ควบคุมระบบ

- Water Pump
- Mist System
- Air Pump
- Lighting

ผ่าน

- Web Dashboard
- Serial CLI
- Schedule Automation
- Manual Switch

---

# 🧠 Architecture

โปรเจคใช้ Clean Architecture สำหรับ embedded

interfaces → domain → application → infrastructure → drivers

## interfaces

กำหนด abstraction ของระบบ เช่น

- IActuator
- IClock
- ISchedule
- INetwork
- ISensor
- IUi

ทำให้ business logic ไม่ผูกกับ hardware

## domain

pure logic

- TimeSchedule
- TimeWindow

ไม่มี dependency กับ Arduino หรือ ESP32

## application

รวม orchestration ของระบบ

- FarmManager
- ScheduledRelay
- CommandService

## infrastructure

สิ่งที่ผูกกับ framework / OS

- SharedState
- RtcClock
- Esp32WebUi
- Esp32WiFiNetwork
- ScheduleStore
- WifiConfigStore

## drivers

hardware drivers

- Esp32Relay
- Esp32Sht40
- Esp32Bh1750
- Esp32ManualSwitch
- RtcDs3231

## tasks (FreeRTOS)

- SensorTask
- CommandTask
- NetworkTask
- WebUiTask

---

# ⚙️ Boot Flow

เมื่อ ESP32 เริ่มทำงาน

boot → AppBoot → mount LittleFS → init SystemContext → start FreeRTOS
tasks

Tasks ที่เริ่ม

- SensorTask
- CommandTask
- NetworkTask
- WebUiTask

---

# 🌐 Network Modes

ระบบรองรับ

- STA Mode (เชื่อมต่อ WiFi)
- AP Mode (อุปกรณ์สร้าง WiFi)

## STA Mode

เข้าหน้า dashboard ผ่าน

http://device-ip

## AP Mode

อุปกรณ์สร้าง WiFi

SSID: SmartFarm-Setup

เข้า

http://192.168.4.1

เพื่อ setup WiFi

---

# 📟 Serial CLI

CLI ใช้ prefix

sm

ตัวอย่าง

sm help sm status sm mode auto sm relay pump on sm relay mist off sm net
on sm clock set 12:30

CLI Architecture

CommandTask → CommandParser → CommandService → SharedState

---

# 🌐 Web Dashboard

ไฟล์

/data/www/dashboard.html

API

GET /api/status

ตัวอย่าง response

{ "mode": 1, "pump": 1, "mist": 0, "air": 0, "lux": 120, "temp": 29.5 }

---

# ⏰ Schedule System

ไฟล์ schedule

/data/schedule.json

ตัวอย่าง

{ "air": \[ {"start": "06:00", "end": "06:15"}, {"start": "12:00",
"end": "12:15"} \] }

ใช้

- TimeSchedule
- ScheduledRelay

เพื่อควบคุม relay อัตโนมัติ

---

# 📂 Data Storage

LittleFS

- /wifi.json
- /schedule.json
- /www/\*

---

# 🔮 Future roadmap

- MQTT support
- remote management
- PLC port
- OTA update

---

# 📜 License

MIT
