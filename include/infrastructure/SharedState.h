#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../interfaces/Types.h"

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

    // 🌡 ใหม่: เอาไว้รับค่าอุณหภูมิจาก Task/Driver
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

private:
    SystemStatus _status;
    SemaphoreHandle_t _mutex;
};

#endif