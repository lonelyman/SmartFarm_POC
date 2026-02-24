#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

/**
 * @brief โหมดการทำงานของระบบ
 */
enum class SystemMode
{
    IDLE,  // พักเครื่อง ปลอดภัย ทุกรีเลย์ควรปิด
    AUTO,  // สมองทำงานอัตโนมัติจากเซนเซอร์
    MANUAL // คนคุมเอง สั่งรีเลย์ตรง ๆ
};

/**
 * @brief รูปแบบข้อมูลมาตรฐานของเซนเซอร์ทุกตัวในระบบ
 */
struct SensorReading
{
    float value;
    bool isValid;
    uint32_t timestamp;

    SensorReading() : value(0.0f), isValid(false), timestamp(0) {}
    SensorReading(float v, bool iv, uint32_t ts)
        : value(v), isValid(iv), timestamp(ts) {}
};

/**
 * @brief สถานะรวมของฟาร์ม (Single Source of Truth)
 */
struct SystemStatus
{
    SensorReading light;
    SensorReading ec;
    SensorReading temperature;

    bool isPumpActive = false;    // ปั๊มน้ำ
    bool isMistActive = false;    // หมอก
    bool isAirPumpActive = false; // ปั๊มลม

    // default ให้เครื่องอยู่ใน IDLE เพื่อความปลอดภัย
    SystemMode mode = SystemMode::IDLE;
};

// ===================== เวลาใน 1 วัน (สำหรับใช้กับ DS3231) =====================
struct TimeOfDay
{
    uint8_t hour;   // 0-23
    uint8_t minute; // 0-59
    uint8_t second; // 0-59

    TimeOfDay() : hour(0), minute(0), second(0) {}
    TimeOfDay(uint8_t h, uint8_t m, uint8_t s)
        : hour(h), minute(m), second(s) {}
};

// helper: แปลงเป็น "นาทีของวัน" 0..1439
inline uint16_t toMinutesOfDay(const TimeOfDay &t)
{
    return static_cast<uint16_t>(t.hour) * 60 + t.minute;
}

// ========== Manual override (สำหรับโหมด MANUAL) ==========
// สั่งด้วยมือ: ความต้องการจากผู้ใช้ (Desired State)
struct ManualOverrides
{
    bool wantPumpOn = false; // อยากให้ปั๊มน้ำ ON?
    bool wantMistOn = false; // อยากให้หมอก ON?
    bool wantAirOn  = false; // อยากให้ปั๊มลม ON?
    bool isUpdated  = false; // ยังไม่ใช้ตอนนี้ แต่เผื่ออนาคต
};

#endif