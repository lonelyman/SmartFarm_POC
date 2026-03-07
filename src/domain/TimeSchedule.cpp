// src/domain/TimeSchedule.cpp
#include "domain/TimeSchedule.h"

bool TimeSchedule::isInWindow(uint16_t minutesOfDay) const
{
   if (!_enabled)
      return false;

   for (uint8_t i = 0; i < _count; i++)
   {
      if (minutesOfDay >= _windows[i].startMin && minutesOfDay < _windows[i].endMin)
         return true;
   }
   return false;
}

void TimeSchedule::addWindow(uint16_t startMin, uint16_t endMin)
{
   if (_count >= MAX_WINDOWS)
      return;
   if (startMin >= endMin)
      return;

   _windows[_count].startMin = startMin;
   _windows[_count].endMin = endMin;
   _count++;
}

void TimeSchedule::clear()
{
   _count = 0;
   _enabled = false;
}

bool TimeSchedule::isEnabled() const
{
   return _enabled;
}

void TimeSchedule::setEnabled(bool enabled)
{
   _enabled = enabled;
}

uint8_t TimeSchedule::windowCount() const
{
   return _count;
}