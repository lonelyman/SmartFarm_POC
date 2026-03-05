#ifndef CONFIG_H
#define CONFIG_H

// ====================== I2C BUS  ======================
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;

// ====================== RELAY OUTPUTS (ต่อเข้า SSR / โมดูลรีเลย์) =======
constexpr int PIN_RELAY_WATER_PUMP = 25; // ปั๊มน้ำ
constexpr int PIN_RELAY_MIST = 26;       // ปั๊มหมอก
constexpr int PIN_RELAY_AIR_PUMP = 27;   // ปั๊มลม

// เผื่อในอนาคต: ไฟ, พัดลม, ฮีตเตอร์ ฯลฯ
constexpr int PIN_RELAY_FREE1 = 13;
constexpr int PIN_RELAY_FREE2 = 16;
constexpr int PIN_RELAY_FREE3 = 17;
constexpr int PIN_RELAY_FREE4 = 14;

// ====================== WATER LEVEL SENSORS (XKC-Y25) ======================
constexpr int PIN_WATER_LEVEL_CH1_LOW_SENSOR = 32;
constexpr int PIN_WATER_LEVEL_CH2_LOW_SENSOR = 33;

// ====================== WATER LEVEL ALARM LEDS =============================
constexpr int PIN_WATER_LEVEL_CH1_ALARM_LED = 23;
constexpr int PIN_WATER_LEVEL_CH2_ALARM_LED = 19;

// ====================== SWITCH INPUTS (ปุ่มหน้าตู้ / สวิตช์โหมด AUTO/IDLE/MANUAL) ========
constexpr int PIN_SW_MODE_A = 34;
constexpr int PIN_SW_MODE_B = 35;

// ====================== NETWORK SWITCH (AP/STA) ======================
constexpr int PIN_SW_NET = 36; // GPIO36 (VP) — HIGH=STA, LOW=AP

constexpr int PIN_SW_FREE = 39; // GPIO39 (VN) input-only

// ====================== ปรับเวลา debounce ปุ่ม (ms) ======================
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;

// ====================== DEBUG LOG SWITCHES ======================
// 1 = เปิด log, 0 = ปิด
#define DEBUG_BH1750_LOG 1      // log จากเซนเซอร์แสง BH1750
#define DEBUG_TEMP_LOG 0        // log จากอุณหภูมิ (fake ตอนนี้)
#define DEBUG_EC_LOG 0          // log จาก EC (อนาคต)
#define DEBUG_TIME_LOG 1        // log เวลา (HH:MM)
#define DEBUG_WATER_LEVEL_LOG 1 // log จาก WaterLevelInput
#define DEBUG_CONTROL_LOG 1     // log จาก ControlTask (mode/pump/mist/air)

#define ENABLE_WATER_LEVEL_CH1 0 // ถ้าไม่มีเซนเซอร์ CH1 ให้ disable เพื่อลด false alarm
#define ENABLE_WATER_LEVEL_CH2 0 // ถ้าไม่มีเซนเซอร์ CH2 ให้ disable เพื่อลด false alarm

// ==== Network & Time Config ====

#define NTP_SERVER "pool.ntp.org"
// เวลาไทย = UTC+7
#define GMT_OFFSET_SEC (7 * 3600)
#define DAYLIGHT_OFFSET_SEC 0

// ====================== TIME SOURCE CONFIG ======================
// ระบบจะใช้ DS3231 RTC เป็นแหล่งนาฬิกาเดียว
// หากอุปกรณ์ไม่ตอบสนอง (begin() คืนค่า false) การอ่านเวลา
// จะล้มเหลวและฟังก์ชันต่างๆ จะคืนค่า false พร้อม minutes=0
// (ซึ่งจะทำให้ schedule หยุดทำงานและ ControlTask จะบันทึก
//  warning)

// ====================== MIST CONTROL HYSTERESIS ======================
// ค่าอุณหภูมิที่ใช้ใน FarmManager::decideMistByTemp
constexpr float HYSTERESIS_TEMP_ON = 32.0f;
constexpr float HYSTERESIS_TEMP_OFF = 29.0f;

constexpr float HYSTERESIS_HUMIDITY_ON = 65.0f;  // %RH — เปิดพ่นเมื่อต่ำกว่านี้
constexpr float HYSTERESIS_HUMIDITY_OFF = 75.0f; // %RH — ปิดพ่นเมื่อสูงกว่านี้

// ====================== NETWORK SWITCH DEBOUNCE ======================
constexpr unsigned long NET_SWITCH_DEBOUNCE_MS = 80;

// ====================== AIR PUMP SCHEDULE (นาทีของวัน) ======================
// ตั้งช่วงเวลาเปิดปั๊มลมที่นี่ ปรับทีหลังได้ง่าย
// ตัวอย่าง: 07:00–12:00 และ 14:00–17:30

constexpr uint16_t AIR_WINDOW1_START_MIN = 7 * 60; // 07:00
constexpr uint16_t AIR_WINDOW1_END_MIN = 12 * 60;  // 12:00

constexpr uint16_t AIR_WINDOW2_START_MIN = 14 * 60;    // 14:00
constexpr uint16_t AIR_WINDOW2_END_MIN = 17 * 60 + 30; // 17:30

// ====================== SERIAL & TASK CONFIG ======================
constexpr unsigned long SERIAL_BAUD = 115200;

constexpr uint32_t INPUT_TASK_INTERVAL_MS = 1000;   // อ่าน sensor ทุก 1 วิ
constexpr uint32_t CONTROL_TASK_INTERVAL_MS = 1000; // คุม actuator ทุก 1 วิ
constexpr uint32_t COMMAND_TASK_INTERVAL_MS = 50;   // loop เช็ค Serial

constexpr uint32_t INPUT_TASK_STACK = 4096;
constexpr uint32_t CONTROL_TASK_STACK = 4096;
constexpr uint32_t COMMAND_TASK_STACK = 4096;
constexpr uint32_t LOOP_IDLE_MS = 1000;

// ====================== MIST TIME GUARD ======================
constexpr uint32_t MIST_MAX_ON_MS = 3 * 60 * 1000UL;  // พ่นได้สูงสุด 3 นาที
constexpr uint32_t MIST_MIN_OFF_MS = 5 * 60 * 1000UL; // พักอย่างน้อย 5 นาที

// ====================== BOOT GUARD ======================
constexpr uint32_t BOOT_GUARD_MS = 10000UL; // หน่วง 10 วินาทีหลัง boot

#endif