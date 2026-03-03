// include/interfaces/IModeSource.h
#pragma once

#include "Types.h"

class IModeSource
{
public:
   virtual ~IModeSource() = default;

   // init resources/pins (ESP32) or do nothing (PLC)
   virtual void begin() = 0;

   // read current mode
   virtual SystemMode readMode() = 0;
};