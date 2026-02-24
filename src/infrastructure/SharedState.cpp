#include "infrastructure/SharedState.h"

ManualOverrides SharedState::getManualOverrides()
{
   ManualOverrides temp;
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