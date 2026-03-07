// include/infrastructure/SharedState.h
#pragma once

#include <Arduino.h>
#include <cstring>
#include <cstddef>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../interfaces/Types.h"

// ============================================================
//  SharedState — Thread-safe state ระหว่าง FreeRTOS tasks
//
//  ทุก task อ่าน/เขียนผ่านที่นี่เท่านั้น
//  ห้าม task คุยกันโดยตรง
//
//  Pattern:
//    - Sensor/Switch → write (inputTask)
//    - Control       → read snapshot + write actuator state (controlTask)
//    - Command/WebUI → write command, read status (commandTask, webUiTask)
//    - Network       → read/write net state (networkTask)
//
//  pending request pattern (one-shot):
//    requester เรียก request*()  → set pending = true
//    consumer  เรียก consume*()  → copy + clear pending, return true ครั้งเดียว
// ============================================================

// --- Pending Requests ---

struct TimeSetRequest
{
    bool pending = false;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
};

enum class NetCmdType : uint8_t
{
    None = 0,
    NetOn,
    NetOff,
    SyncNtp,
};

struct NetCommand
{
    bool pending = false;
    NetCmdType type = NetCmdType::None;
};

// --- Network State Machine ---

enum class NetState : uint8_t
{
    AP_PRIMARY = 0,     // boot เข้า AP เป็นหลัก (offline-first)
    STA_CONNECTING = 1, // กำลังพยายามต่อ STA
    STA_CONNECTED = 2,  // ต่อ STA สำเร็จ
    STA_FAILED = 3,     // ต่อ STA ไม่สำเร็จ → กลับ AP
    STA_STOPPED = 4,    // ผู้ใช้สั่ง NetOff → กลับ AP
};

// ============================================================

class SharedState
{
public:
    SharedState();

    // --- Mode ---
    void setMode(SystemMode mode);

    // --- Sensors ---
    void updateSensors(float lux, bool luxValid, float ec, bool ecValid, uint32_t ts);
    void updateTemperature(float temp, bool tempValid, uint32_t ts);
    void updateHumidity(float hum, bool humValid, uint32_t ts);
    void updateWaterTemps(const WaterTempReading *readings, uint8_t count);
    void updateWaterLevelSensors(bool ch1Low, bool ch2Low, uint32_t ts);

    // --- Actuators ---
    void updateActuators(bool pump, bool mist, bool air);

    // --- Snapshot (read-only copy สำหรับ task ที่ต้องการ consistent view) ---
    SystemStatus getSnapshot() const;

    // --- Manual Overrides ---
    ManualOverrides getManualOverrides() const;
    void setManualOverrides(const ManualOverrides &m);

    // --- Pending: ตั้งเวลา RTC ---
    void requestSetClockTime(uint8_t hour, uint8_t minute, uint8_t second);
    bool consumeSetClockTime(TimeSetRequest &out);

    // --- Network State ---
    void setNetState(NetState st);
    NetState getNetState() const;

    // --- Pending: Network Commands ---
    void requestNetOn();
    void requestNetOff();
    void requestSyncNtp();
    bool consumeNetCommand(NetCommand &out);

    // --- Network Message (feedback ให้ UI) ---
    void setNetMessage(const char *msg);
    void getNetMessage(char *out, size_t outLen) const;

private:
    mutable SemaphoreHandle_t _mutex; // mutable เพื่อให้ const method lock ได้

    SystemStatus _status;
    ManualOverrides _manualOverrides;
    TimeSetRequest _timeSetReq;
    NetCommand _netCmd;
    NetState _netState = NetState::AP_PRIMARY;
    char _netMsg[96];
};