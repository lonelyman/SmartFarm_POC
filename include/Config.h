#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  SmartFarm POC — Hardware Configuration
//  Board : ESP32-S3-WROOM-1  N16R8  44-Pin  (ESP32-S3-DevKitC-1)
//  Flash : 16 MB QIO   |   PSRAM : 8 MB Octal (OPI)
//  IDE   : PlatformIO + Arduino framework
//
//  ⚠️  GPIO26–37 : บน module นี้ (N16R8 Octal PSRAM) ถูก reserve โดย Flash/PSRAM
//                  ห้ามใช้บน module นี้เท่านั้น (S3 รุ่นไม่มี PSRAM อาจใช้บางขาได้)
//  ⚠️  GPIO0     : Strapping → ต้องมี pull-up 10kΩ ไปยัง 3.3V
//  ⚠️  GPIO19–20 : USB D−/D+ → ห้ามต่ออุปกรณ์ภายนอก
//  ⚠️  GPIO38–47 : ยืนยันแล้วว่า break out ครบที่ header บนบอร์ดล็อตนี้
//                  ถ้าเปลี่ยนบอร์ด/ล็อต ให้ตรวจ pinout ก่อนเสมอ
// ============================================================

// ════════════════════════════════════════════════════════════
//  SECTION 1 : I2C BUS
//  Devices : BH1750 (Light), SHT40 (Temp/Hum), DS3231 (RTC)
//
//  เลือก SDA=8, SCL=9 (คู่ติดกัน) เพื่อ:
//    - เดินสายสั้น ไม่ขวาง เป็นมาตรฐานโซนเดียวกัน
//    - ตรงกับ S3 Arduino default (ทำให้คนอ่านต่อตาม spec ทันที)
//    - GPIO8/9 เป็นขา I/O ทั่วไป เหมาะทำ I2C และไม่ชน USB/strap
//
//  ⚡ ต้องมี external pull-up 4.7kΩ–10kΩ ไปยัง 3.3V ที่ทั้ง SDA และ SCL
//     (ถ้าลืม: I2C scan จะไม่เจอ device เลย แม้ต่อถูกทุกอย่าง)
// ════════════════════════════════════════════════════════════
constexpr int PIN_I2C_SDA = 8; // GPIO8  — I2C SDA  (S3 Arduino default)
constexpr int PIN_I2C_SCL = 9; // GPIO9  — I2C SCL  (S3 Arduino default, คู่กับ SDA=8)

// ════════════════════════════════════════════════════════════
//  SECTION 2 : 1-Wire BUS
//  Devices : DS18B20 (Water Temperature)
//
//  ⚡ ต้องมี external pull-up 4.7kΩ ไปยัง 3.3V ที่ DATA pin เสมอ
//     เริ่มที่ 4.7kΩ เสมอ — ถ้าสายยาว/หลายตัวแล้วสัญญาณไม่ชัด
//     ค่อยปรับช่วง 2.2k–4.7k ตามผลทดสอบจริง (ไม่มีค่าตายตัว)
// ════════════════════════════════════════════════════════════
constexpr int PIN_ONE_WIRE = 4;            // GPIO4 — 1-Wire DATA (ผ่าน Adapter Module)
constexpr uint8_t DS18B20_MAX_SENSORS = 4; // รองรับสูงสุด 4 ตัว

// ════════════════════════════════════════════════════════════
//  SECTION 3 : RELAY OUTPUTS
//  ต่อกับโมดูลรีเลย์ / SSR
//
//  RELAY_ACTIVE_LOW:
//    true  = โมดูล Active LOW (IN=LOW → relay ON)
//    false = โมดูล Active HIGH (IN=HIGH → relay ON)
//
//  ⚠️  ค่านี้ต้องตั้งตามโมดูลที่ใช้จริง — ดูวงจรหรือทดสอบด้วยการสั่ง ON
//      แล้วสังเกตไฟ LED สถานะ/เสียงคลิกของรีเลย์
//      ทดสอบครั้งแรก: ถอดโหลดจริงออกก่อน (ไม่ต่อปั๊ม) ดูแค่ไฟสถานะ
//
//  Esp32Relay driver ใช้ flag นี้แปลงสัญญาณอัตโนมัติ
//
//  ⚠️  Boot glitch: บาง GPIO ช่วง boot มีสัญญาณกระตุกชั่วคราว
//      ถ้า RELAY_ACTIVE_LOW=true และขา floating ตอนบูต → relay อาจ ON ชั่วคราวได้
//      แก้: AppBoot ต้อง pinMode + digitalWrite(inactive) ก่อนสร้าง FreeRTOS task
//           (ดูใน AppBoot::initRelayPins — ทำก่อน initDrivers)
//
//  ✅ GPIO38/40/41 ยืนยันแล้วว่า break out ที่ header บนบอร์ดล็อตนี้
//     ถ้าเปลี่ยนบอร์ด: ตรวจ pinout ก่อนเสมอ
// ════════════════════════════════════════════════════════════
constexpr bool RELAY_ACTIVE_LOW = true; // ปรับตามโมดูลที่ใช้จริง

constexpr int PIN_RELAY_WATER_PUMP = 38; // GPIO38 — ปั๊มน้ำ   (เปลี่ยนจาก 25 — ไม่มีบน S3)
constexpr int PIN_RELAY_MIST = 40;       // GPIO40 — ปั๊มหมอก  (เปลี่ยนจาก 26 — อยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)
constexpr int PIN_RELAY_AIR_PUMP = 41;   // GPIO41 — ปั๊มลม    (เปลี่ยนจาก 27 — อยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)

// ════════════════════════════════════════════════════════════
//  SECTION 4 : DIGITAL INPUTS — Manual Switches (หน้าตู้)
//  Active LOW — ใช้ INPUT_PULLUP (pull-up ภายใน ESP32-S3)
// ════════════════════════════════════════════════════════════
constexpr int PIN_SW_MANUAL_PUMP = 18; // GPIO18 — Manual: ปั๊มน้ำ (คง GPIO เดิม)
constexpr int PIN_SW_MANUAL_MIST = 5;  // GPIO5  — Manual: หมอก    (คง GPIO เดิม)
constexpr int PIN_SW_MANUAL_AIR = 15;  // GPIO15 — Manual: ปั๊มลม  (คง GPIO เดิม)

// ════════════════════════════════════════════════════════════
//  SECTION 5 : DIGITAL INPUTS — Mode & Network Switches
//  Mode Switch : 2-bit encode → AUTO / MANUAL / IDLE
//  Net Switch  : HIGH = STA (เชื่อมต่อ WiFi) | LOW = AP (hotspot)
//  Active LOW — ใช้ INPUT_PULLUP
//
//  ⚠️  GPIO39 ใช้เป็น input เท่านั้น — ห้ามใช้เป็น output (เหมาะกับสวิตช์เท่านั้น)
// ════════════════════════════════════════════════════════════
constexpr int PIN_SW_MODE_A = 6; // GPIO6  — Mode bit A  (เปลี่ยนจาก 34 — อยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)
constexpr int PIN_SW_MODE_B = 7; // GPIO7  — Mode bit B  (เปลี่ยนจาก 35 — อยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)
constexpr int PIN_SW_NET = 39;   // GPIO39 — AP/STA      (เปลี่ยนจาก 36 — ขาเดิมอยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)
constexpr int PIN_SW_FREE = 42;  // GPIO42 — สำรอง / ขยายอนาคต (เปลี่ยนจาก 39) — ใช้ได้ทั้ง I/O

// ════════════════════════════════════════════════════════════
//  SECTION 6 : DIGITAL INPUTS — Water Level Sensors (XKC-Y25)
//
//  XKC-Y25 output type: ตรวจสอบรุ่นก่อนใช้งาน
//    - Push-pull       : ไม่ต้องการ pull-up ภายนอก (INPUT ปกติได้)
//    - Open-collector  : ต้องการ pull-up → ใช้ INPUT_PULLUP หรือต่อ R ภายนอก
//  ✅ Code ใช้ INPUT_PULLUP เป็น default (ปลอดภัยทั้ง 2 กรณี)
//
//  WATER_LEVEL_ALARM_LOW: logic ขาเซนเซอร์เมื่อน้ำต่ำกว่าระดับ (alarm state)
//    true  = LOW  = alarm  (เซนเซอร์ดึงขาลง GND เมื่อน้ำต่ำ)
//    false = HIGH = alarm  (เซนเซอร์ดึงขาขึ้น VCC เมื่อน้ำต่ำ)
//  ⚠️  ตั้งค่าให้ตรงกับรุ่นเซนเซอร์จริงและยืนยันกับ driver/inputTask
//      ถ้า flag ไม่ตรงกับ driver: alarm จะกลับขั้ว (น้ำปกติ = เตือน, น้ำต่ำ = เงียบ)
//  📌  Contract: driver ต้องอ่าน WATER_LEVEL_ALARM_LOW แล้ว normalize ผลลัพธ์เป็น
//      isLow=true เสมอ ห้าม hardcode logic ขั้วสัญญาณไว้ใน driver โดยตรง
// ════════════════════════════════════════════════════════════
constexpr bool WATER_LEVEL_ALARM_LOW = true; // ปรับตามรุ่นเซนเซอร์ที่ใช้จริง

constexpr int PIN_WATER_LEVEL_CH1_LOW_SENSOR = 1; // GPIO1 — ถัง CH1 (เปลี่ยนจาก 32 — อยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)
constexpr int PIN_WATER_LEVEL_CH2_LOW_SENSOR = 2; // GPIO2 — ถัง CH2 (เปลี่ยนจาก 33 — อยู่ในกลุ่ม reserved 26–37 บนโมดูลนี้)

// ════════════════════════════════════════════════════════════
//  SECTION 7 : DIGITAL OUTPUTS — Water Level Alarm LEDs
//
//  ALARM_LED_ACTIVE_HIGH:
//    true  = LED ติดเมื่อ GPIO HIGH (ต่อ LED → GND)  ← วงจรทั่วไป
//    false = LED ติดเมื่อ GPIO LOW  (ต่อ LED → VCC)  ← active-low wiring
//
//  ✅ GPIO10  : โซน 1–21 เดินสายง่าย ยืนยัน break out แล้ว
//  ✅ GPIO47  : ยืนยัน break out ที่ header บนบอร์ดล็อตนี้แล้ว
//              (หากเปลี่ยนบอร์ด: ตรวจ pinout ก่อน — บางล็อตไม่ expose GPIO47)
// ════════════════════════════════════════════════════════════
constexpr bool ALARM_LED_ACTIVE_HIGH = true; // ปรับตามวงจร LED ที่ต่อจริง

constexpr int PIN_WATER_LEVEL_CH1_ALARM_LED = 47; // GPIO47 — LED เตือน CH1 (เปลี่ยนจาก 23 — ไม่มีบน S3)
constexpr int PIN_WATER_LEVEL_CH2_ALARM_LED = 10; // GPIO10 — LED เตือน CH2 (เปลี่ยนจาก 19 — ขาเดิมชน USB D−)

// ════════════════════════════════════════════════════════════
//  SECTION 8 : TIMING — Debounce
// ════════════════════════════════════════════════════════════
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;     // debounce ปุ่ม manual / mode (ms)
constexpr unsigned long NET_SWITCH_DEBOUNCE_MS = 80; // debounce network switch (ms)

// ════════════════════════════════════════════════════════════
//  SECTION 9 : NETWORK & TIME (NTP)
// ════════════════════════════════════════════════════════════
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC (7 * 3600) // UTC+7 (ไทย)
#define DAYLIGHT_OFFSET_SEC 0

// ════════════════════════════════════════════════════════════
//  SECTION 10 : MIST CONTROL — Hysteresis
//  Logic ใน FarmManager::decideMistByTemp / decideMistByHumidity
// ════════════════════════════════════════════════════════════
constexpr float HYSTERESIS_TEMP_ON = 32.0f;      // °C  — เปิดพ่นเมื่อสูงกว่านี้
constexpr float HYSTERESIS_TEMP_OFF = 29.0f;     // °C  — ปิดพ่นเมื่อต่ำกว่านี้
constexpr float HYSTERESIS_HUMIDITY_ON = 65.0f;  // %RH — เปิดพ่นเมื่อความชื้นต่ำกว่า
constexpr float HYSTERESIS_HUMIDITY_OFF = 75.0f; // %RH — ปิดพ่นเมื่อความชื้นสูงกว่า

// ════════════════════════════════════════════════════════════
//  SECTION 11 : MIST TIME GUARD
//  ป้องกันปั๊มหมอกทำงานต่อเนื่องนานเกินไป
// ════════════════════════════════════════════════════════════
constexpr uint32_t MIST_MAX_ON_MS = 3 * 60 * 1000UL;  // พ่นได้สูงสุด 3 นาที
constexpr uint32_t MIST_MIN_OFF_MS = 5 * 60 * 1000UL; // พักอย่างน้อย 5 นาที

// ════════════════════════════════════════════════════════════
//  SECTION 12 : AIR PUMP SCHEDULE (fallback hardcode)
//  ใช้เมื่อไม่มี schedule.json บน LittleFS
//  หน่วย : นาทีนับจากเที่ยงคืน (0–1439)
// ════════════════════════════════════════════════════════════
constexpr uint16_t AIR_WINDOW1_START_MIN = 7 * 60;     // 07:00
constexpr uint16_t AIR_WINDOW1_END_MIN = 12 * 60;      // 12:00
constexpr uint16_t AIR_WINDOW2_START_MIN = 14 * 60;    // 14:00
constexpr uint16_t AIR_WINDOW2_END_MIN = 17 * 60 + 30; // 17:30

// ════════════════════════════════════════════════════════════
//  SECTION 13 : SERIAL & TASK CONFIG (FreeRTOS)
// ════════════════════════════════════════════════════════════
constexpr unsigned long SERIAL_BAUD = 115200; // Serial Monitor baud rate

constexpr uint32_t INPUT_TASK_INTERVAL_MS = 1000;   // อ่าน sensor ทุก 1 วิ
constexpr uint32_t CONTROL_TASK_INTERVAL_MS = 1000; // คุม actuator ทุก 1 วิ
constexpr uint32_t COMMAND_TASK_INTERVAL_MS = 50;   // loop เช็ค Serial ทุก 50ms

constexpr uint32_t INPUT_TASK_STACK = 4096;   // bytes
constexpr uint32_t CONTROL_TASK_STACK = 4096; // bytes
constexpr uint32_t COMMAND_TASK_STACK = 4096; // bytes
constexpr uint32_t LOOP_IDLE_MS = 1000;       // ms

// ════════════════════════════════════════════════════════════
//  SECTION 14 : BOOT GUARD
//  หน่วง actuator ทุกตัวหลัง boot เพื่อให้ระบบ stable ก่อนเริ่ม AUTO
//
//  ⚠️  boot glitch mitigation:
//      AppBoot::initRelayPins() ต้องทำ pinMode + digitalWrite(INACTIVE)
//      ให้ทุก relay ก่อนสร้าง FreeRTOS task — ไม่ใช่รอให้ Esp32Relay::begin() จัดการ
//      เพราะช่วง boot ขา floating อาจ trigger relay Active-LOW ชั่วคราวได้
// ════════════════════════════════════════════════════════════
constexpr uint32_t BOOT_GUARD_MS = 10000UL; // หน่วง 10 วินาทีหลัง boot

// ════════════════════════════════════════════════════════════
//  SECTION 15 : FEATURE FLAGS — Enable/Disable Sensors
// ════════════════════════════════════════════════════════════
#define ENABLE_WATER_LEVEL_CH1 0 // 1=เปิด, 0=ปิด — ปิดถ้าไม่มีเซนเซอร์ เพื่อลด false alarm
#define ENABLE_WATER_LEVEL_CH2 0 // 1=เปิด, 0=ปิด — ปิดถ้าไม่มีเซนเซอร์ เพื่อลด false alarm

// ════════════════════════════════════════════════════════════
//  SECTION 16 : DEBUG LOG SWITCHES
//  1 = เปิด log | 0 = ปิด log
// ════════════════════════════════════════════════════════════
#define DEBUG_BH1750_LOG 1      // เซนเซอร์แสง BH1750
#define DEBUG_TEMP_LOG 0        // อุณหภูมิ / ความชื้น SHT40
#define DEBUG_EC_LOG 0          // EC sensor (อนาคต)
#define DEBUG_TIME_LOG 1        // เวลา RTC (HH:MM)
#define DEBUG_WATER_LEVEL_LOG 1 // Water Level XKC-Y25
#define DEBUG_CONTROL_LOG 1     // ControlTask (mode / pump / mist / air)

#endif // CONFIG_H