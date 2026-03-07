// src/drivers/Esp32Sht40.cpp
#include "drivers/Esp32Sht40.h"

Esp32Sht40::Esp32Sht40(const char *name)
    : _name(name), _lastHumidity(0.0f), _humidityValid(false) {}

bool Esp32Sht40::begin()
{
   // Wire.begin() อยู่ใน AppBoot — ห้ามเรียกซ้ำที่นี่
   if (!_sht.begin())
   {
      Serial.printf("[SHT40] %s — ไม่พบอุปกรณ์บน I2C\n", _name.c_str());
      return false;
   }

   _sht.setPrecision(SHT4X_HIGH_PRECISION);
   _sht.setHeater(SHT4X_NO_HEATER);

   Serial.printf("[SHT40] %s — init OK\n", _name.c_str());
   return true;
}

SensorReading Esp32Sht40::read()
{
   sensors_event_t tempEvent, humEvent;
   if (!_sht.getEvent(&humEvent, &tempEvent))
   {
      _humidityValid = false;
      return SensorReading(0.0f, false, millis());
   }

   _lastHumidity = humEvent.relative_humidity;
   _humidityValid = true;

   return SensorReading(tempEvent.temperature, true, millis());
}

const char *Esp32Sht40::getName() const
{
   return _name.c_str();
}

float Esp32Sht40::getLastHumidity() const
{
   return _lastHumidity;
}

bool Esp32Sht40::isHumidityValid() const
{
   return _humidityValid;
}