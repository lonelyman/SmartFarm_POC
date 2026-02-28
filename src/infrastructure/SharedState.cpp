#include "infrastructure/SharedState.h"

ManualOverrides SharedState::getManualOverrides()
{
   ManualOverrides temp{};
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      temp = _manualOverrides;
      xSemaphoreGive(_mutex);
   }
   return temp;
}

void SharedState::setManualOverrides(const ManualOverrides &m)
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      _manualOverrides = m;
      xSemaphoreGive(_mutex);
   }
   else
   {
      Serial.println("⚠️ SharedState: Set ManualOverrides Lock Timeout!");
   }
}

void SharedState::requestSetClockTime(uint8_t hour, uint8_t minute, uint8_t second)
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      _timeSetReq.pending = true;
      _timeSetReq.hour = hour;
      _timeSetReq.minute = minute;
      _timeSetReq.second = second;
      xSemaphoreGive(_mutex);
   }
   else
   {
      Serial.println("⚠️ SharedState: Request SetClockTime Lock Timeout!");
   }
}

bool SharedState::consumeSetClockTime(TimeSetRequest &out)
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      if (!_timeSetReq.pending)
      {
         xSemaphoreGive(_mutex);
         return false;
      }

      out = _timeSetReq;
      _timeSetReq.pending = false;

      xSemaphoreGive(_mutex);
      return true;
   }

   Serial.println("⚠️ SharedState: Consume SetClockTime Lock Timeout!");
   return false;
}

void SharedState::requestNetOn()
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      _netCmd.pending = true;
      _netCmd.type = NetCmdType::NetOn;
      xSemaphoreGive(_mutex);
   }
}

void SharedState::requestNetOff()
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      _netCmd.pending = true;
      _netCmd.type = NetCmdType::NetOff;
      xSemaphoreGive(_mutex);
   }
}

void SharedState::requestSyncNtp()
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      _netCmd.pending = true;
      _netCmd.type = NetCmdType::SyncNtp;
      xSemaphoreGive(_mutex);
   }
}

bool SharedState::consumeNetCommand(NetCommand &out)
{
   if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)))
   {
      if (!_netCmd.pending)
      {
         xSemaphoreGive(_mutex);
         return false;
      }
      out = _netCmd;
      _netCmd.pending = false;
      _netCmd.type = NetCmdType::None;
      xSemaphoreGive(_mutex);
      return true;
   }
   return false;
}