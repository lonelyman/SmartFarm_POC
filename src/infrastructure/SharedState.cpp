// src/infrastructure/SharedState.cpp
#include "infrastructure/SharedState.h"

// ============================================================
//  Helpers
// ============================================================

// RAII-style lock guard สำหรับ FreeRTOS mutex
// ใช้แค่ใน file นี้ — ไม่ expose ออกไป
namespace
{
   struct Lock
   {
      explicit Lock(SemaphoreHandle_t m) : _m(m), _ok(m && xSemaphoreTake(m, pdMS_TO_TICKS(100))) {}
      ~Lock()
      {
         if (_ok)
            xSemaphoreGive(_m);
      }
      explicit operator bool() const { return _ok; }
      SemaphoreHandle_t _m;
      bool _ok;
   };
} // namespace

// ============================================================
//  Constructor
// ============================================================

SharedState::SharedState()
    : _mutex(xSemaphoreCreateMutex())
{
   if (!_mutex)
      Serial.println("⚠️ SharedState: mutex creation failed!");

   strncpy(_netMsg, "ready", sizeof(_netMsg));
}

// ============================================================
//  Mode
// ============================================================

void SharedState::setMode(SystemMode mode)
{
   if (Lock lk{_mutex})
      _status.mode = mode;
}

SystemMode SharedState::getMode() const
{
   SystemMode temp = SystemMode::IDLE;
   if (Lock lk{_mutex})
      temp = _status.mode;
   return temp;
}

// ============================================================
//  Sensors
// ============================================================

void SharedState::updateSensors(float lux, bool luxValid,
                                float ec, bool ecValid, uint32_t ts)
{
   if (Lock lk{_mutex})
   {
      _status.light = SensorReading(lux, luxValid, ts);
      _status.ec = SensorReading(ec, ecValid, ts);
   }
   else
      Serial.println("⚠️ SharedState: updateSensors timeout");
}

void SharedState::updateTemperature(float temp, bool tempValid, uint32_t ts)
{
   if (Lock lk{_mutex})
      _status.temperature = SensorReading(temp, tempValid, ts);
   else
      Serial.println("⚠️ SharedState: updateTemperature timeout");
}

void SharedState::updateHumidity(float hum, bool humValid, uint32_t ts)
{
   if (Lock lk{_mutex})
      _status.humidity = SensorReading(hum, humValid, ts);
}

void SharedState::updateWaterTemps(const WaterTempReading *readings, uint8_t count)
{
   if (Lock lk{_mutex})
   {
      const uint8_t n = count < MAX_WATER_TEMP_SENSORS ? count : MAX_WATER_TEMP_SENSORS;
      for (uint8_t i = 0; i < n; i++)
         _status.waterTemp[i] = readings[i];
      _status.waterTempCount = n;
   }
}

void SharedState::updateWaterLevelSensors(bool ch1Low, bool ch2Low, uint32_t ts)
{
   (void)ts;
   if (Lock lk{_mutex})
   {
      _status.waterLevelSensors.ch1Low = ch1Low;
      _status.waterLevelSensors.ch2Low = ch2Low;
   }
   else
      Serial.println("⚠️ SharedState: updateWaterLevelSensors timeout");
}

// ============================================================
//  Actuators
// ============================================================

void SharedState::updateActuators(bool pump, bool mist, bool air)
{
   if (Lock lk{_mutex})
   {
      _status.isPumpActive = pump;
      _status.isMistActive = mist;
      _status.isAirPumpActive = air;
   }
}

// ============================================================
//  Snapshot
// ============================================================

SystemStatus SharedState::getSnapshot() const
{
   SystemStatus temp{};
   if (Lock lk{_mutex})
      temp = _status;
   return temp;
}

// ============================================================
//  Manual Overrides
// ============================================================

ManualOverrides SharedState::getManualOverrides() const
{
   ManualOverrides temp{};
   if (Lock lk{_mutex})
      temp = _manualOverrides;
   return temp;
}

void SharedState::setManualOverrides(const ManualOverrides &m)
{
   if (Lock lk{_mutex})
      _manualOverrides = m;
   else
      Serial.println("⚠️ SharedState: setManualOverrides timeout");
}

// ============================================================
//  Pending: Clock
// ============================================================

void SharedState::requestSetClockTime(uint8_t hour, uint8_t minute, uint8_t second)
{
   if (Lock lk{_mutex})
   {
      _timeSetReq.pending = true;
      _timeSetReq.hour = hour;
      _timeSetReq.minute = minute;
      _timeSetReq.second = second;
   }
   else
      Serial.println("⚠️ SharedState: requestSetClockTime timeout");
}

bool SharedState::consumeSetClockTime(TimeSetRequest &out)
{
   if (Lock lk{_mutex})
   {
      if (!_timeSetReq.pending)
         return false;
      out = _timeSetReq;
      _timeSetReq.pending = false;
      return true;
   }
   Serial.println("⚠️ SharedState: consumeSetClockTime timeout");
   return false;
}

// ============================================================
//  Network State
// ============================================================

void SharedState::setNetState(NetState st)
{
   if (Lock lk{_mutex})
      _netState = st;
}

NetState SharedState::getNetState() const
{
   NetState temp = NetState::AP_PRIMARY;
   if (Lock lk{_mutex})
      temp = _netState;
   return temp;
}

// ============================================================
//  Pending: Network Commands
// ============================================================

void SharedState::requestNetOn()
{
   if (Lock lk{_mutex})
   {
      _netCmd.pending = true;
      _netCmd.type = NetCmdType::NetOn;
   }
}

void SharedState::requestNetOff()
{
   if (Lock lk{_mutex})
   {
      _netCmd.pending = true;
      _netCmd.type = NetCmdType::NetOff;
   }
}

void SharedState::requestSyncNtp()
{
   if (Lock lk{_mutex})
   {
      _netCmd.pending = true;
      _netCmd.type = NetCmdType::SyncNtp;
   }
}

bool SharedState::consumeNetCommand(NetCommand &out)
{
   if (Lock lk{_mutex})
   {
      if (!_netCmd.pending)
         return false;
      out = _netCmd;
      _netCmd.pending = false;
      return true;
   }
   return false;
}

// ============================================================
//  Network Message
// ============================================================

void SharedState::setNetMessage(const char *msg)
{
   if (!msg)
      return;
   if (Lock lk{_mutex})
   {
      strncpy(_netMsg, msg, sizeof(_netMsg) - 1);
      _netMsg[sizeof(_netMsg) - 1] = '\0';
   }
}

void SharedState::getNetMessage(char *out, size_t outLen) const
{
   if (!out || outLen == 0)
      return;
   if (Lock lk{_mutex})
   {
      strncpy(out, _netMsg, outLen - 1);
      out[outLen - 1] = '\0';
   }
   else
      out[0] = '\0';
}