#ifndef CONFIG_H
#define CONFIG_H

// ====================== I2C BUS (สำหรับ BH1750 ฯลฯ) ======================
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

// ====================== ANALOG INPUTS (EC / อื่น ๆ) ======================
constexpr int PIN_ANALOG_EC = 32;
constexpr int PIN_ANALOG_FREE = 33;

// ====================== SWITCH INPUTS (ปุ่มหน้าตู้ / สวิตช์โหมด) ========
constexpr int PIN_SW_MODE_A = 34;
constexpr int PIN_SW_MODE_B = 35;
constexpr int PIN_SW_CLEAR = 36;
constexpr int PIN_SW_FREE = 39;

// ====================== ปรับเวลา debounce ปุ่ม (ms) ======================
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;

// ====================== DEBUG LOG SWITCHES ======================
// 1 = เปิด log, 0 = ปิด
#define DEBUG_BH1750_LOG 0  // log จากเซนเซอร์แสง BH1750
#define DEBUG_TEMP_LOG 0    // log จากอุณหภูมิ (fake ตอนนี้)
#define DEBUG_EC_LOG 0      // log จาก EC (อนาคต)
#define DEBUG_TIME_LOG 1    // log เวลา (HH:MM)
#define DEBUG_CONTROL_LOG 1 // log จาก ControlTask (mode/pump/mist/air)

// ====================== TIME SOURCE CONFIG ======================
// ถ้ายังไม่มี RTC จริง ให้ใช้ fake time ไปก่อน
// #define USE_FAKE_TIME

// ให้ระบบ "คิดว่า" ตอนนี้เป็นกี่นาทีของวัน (0..1439)
// เช่น 07:30 = 7*60 + 30
constexpr uint16_t FAKE_MINUTES_OF_DAY = 7 * 60 + 30; // 07:30 น.

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
#endif