// include/interfaces/Types.h
#pragma once

#include <cstdint>

// ============================================================
//  Types.h — Domain Types ที่ใช้ร่วมกันทั้งระบบ
//  ห้าม include Arduino API ในไฟล์นี้
//  เพื่อให้ Domain / Application layer ทดสอบได้โดยไม่ต้องมี hardware
// ============================================================

// --- System Mode ---

enum class SystemMode : uint8_t
{
    IDLE = 0,   // พักเครื่อง — ทุก relay ปิด ปลอดภัยสุด
    AUTO = 1,   // ระบบตัดสินใจเองจากเซนเซอร์
    MANUAL = 2, // คนสั่งตรงผ่าน manual switch / Serial / Web
};

// --- Sensor Reading ---

struct SensorReading
{
    float value = 0.0f;
    bool isValid = false;
    uint32_t timestamp = 0;

    SensorReading() = default;
    SensorReading(float v, bool iv, uint32_t ts)
        : value(v), isValid(iv), timestamp(ts) {}
};

// --- Water Level ---

struct WaterLevelSensors
{
    bool ch1Low = false; // true = น้ำต่ำกว่าระดับ CH1
    bool ch2Low = false; // true = น้ำต่ำกว่าระดับ CH2
};

// --- Water Temperature (DS18B20) ---

constexpr uint8_t MAX_WATER_TEMP_SENSORS = 4;

struct WaterTempReading
{
    float tempC = 0.0f;
    bool isValid = false;
    char label[16] = "";
};

// --- System Status (Single Source of Truth ใน SharedState) ---

struct SystemStatus
{
    SensorReading light;
    SensorReading ec; // reserved — ยังไม่ implement
    SensorReading temperature;
    SensorReading humidity;
    WaterLevelSensors waterLevelSensors;

    WaterTempReading waterTemp[MAX_WATER_TEMP_SENSORS];
    uint8_t waterTempCount = 0;

    bool isPumpActive = false;
    bool isMistActive = false;
    bool isAirPumpActive = false;

    SystemMode mode = SystemMode::IDLE; // default IDLE — ปลอดภัยสุด
};

// --- Time ---

struct TimeOfDay
{
    uint8_t hour = 0;   // 0–23
    uint8_t minute = 0; // 0–59
    uint8_t second = 0; // 0–59

    TimeOfDay() = default;
    TimeOfDay(uint8_t h, uint8_t m, uint8_t s)
        : hour(h), minute(m), second(s) {}
};

// แปลง TimeOfDay → นาทีของวัน (0–1439)
inline uint16_t toMinutesOfDay(const TimeOfDay &t)
{
    return static_cast<uint16_t>(t.hour) * 60 + t.minute;
}

// --- Manual Overrides (MANUAL mode) ---

struct ManualOverrides
{
    bool wantPumpOn = false;
    bool wantMistOn = false;
    bool wantAirOn = false;
    bool isUpdated = false; // reserved — เผื่อ edge-trigger อนาคต
};