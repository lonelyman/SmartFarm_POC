#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../interfaces/Types.h"

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

class SharedState
{
public:
    SharedState()
    {
        _mutex = xSemaphoreCreateMutex();
        if (_mutex == NULL)
        {
            Serial.println("⚠️ ERROR: Mutex creation failed!");
        }

        // init safe defaults
        _manualOverrides = ManualOverrides{};
        _timeSetReq = TimeSetRequest{};
    }

    void setMode(SystemMode mode)
    {
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
        {
            _status.mode = mode;
            xSemaphoreGive(_mutex);
        }
    }

    // เดิม: อัปเดตแสง + EC
    void updateSensors(float lux, bool luxValid,
                       float ec, bool ecValid,
                       uint32_t ts)
    {
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
        {
            _status.light = SensorReading(lux, luxValid, ts);
            _status.ec = SensorReading(ec, ecValid, ts);
            xSemaphoreGive(_mutex);
        }
        else
        {
            Serial.println("⚠️ SharedState: Update Sensors Lock Timeout!");
        }
    }

    // 🌡 อัปเดตอุณหภูมิ
    void updateTemperature(float temp, bool tempValid, uint32_t ts)
    {
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
        {
            _status.temperature = SensorReading(temp, tempValid, ts);
            xSemaphoreGive(_mutex);
        }
        else
        {
            Serial.println("⚠️ SharedState: Update Temperature Lock Timeout!");
        }
    }

    void updateActuators(bool pump, bool mist, bool air)
    {
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
        {
            _status.isPumpActive = pump;
            _status.isMistActive = mist;
            _status.isAirPumpActive = air;
            xSemaphoreGive(_mutex);
        }
    }

    SystemStatus getSnapshot()
    {
        SystemStatus temp;
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
        {
            temp = _status;
            xSemaphoreGive(_mutex);
        }
        return temp;
    }

    // ✅ MANUAL overrides
    ManualOverrides getManualOverrides();
    void setManualOverrides(const ManualOverrides &m);
    // ✅ Water level sensors (CH1/CH2)
    void updateWaterLevelSensors(bool ch1Low, bool ch2Low, uint32_t ts);
    // ✅ Time set request (CommandTask -> SharedState -> ControlTask -> IClock)
    void requestSetClockTime(uint8_t hour, uint8_t minute, uint8_t second);
    bool consumeSetClockTime(TimeSetRequest &out);

    void requestNetOn();
    void requestNetOff();
    void requestSyncNtp();
    bool consumeNetCommand(NetCommand &out);

private:
    SystemStatus _status;
    SemaphoreHandle_t _mutex;

    // ✅ เก็บ “สิ่งที่คนสั่ง” แยกจากสถานะรีเลย์จริง
    ManualOverrides _manualOverrides;

    // ✅ เก็บคำขอตั้งเวลา (pending request)
    TimeSetRequest _timeSetReq;

    NetCommand _netCmd;
};

#endif