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