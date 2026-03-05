// include/drivers/Esp32Sht40.h
#ifndef ESP32_SHT40_H
#define ESP32_SHT40_H

#include <Arduino.h>
#include "Adafruit_SHT4x.h"
#include "../interfaces/ISensor.h"
#include "../interfaces/Types.h"

class Esp32Sht40 : public ISensor
{
public:
   Esp32Sht40(const char *name = "SHT40")
       : _name(name), _lastHumidity(0.0f), _humidityValid(false) {}

   bool begin() override
   {
      if (!_sht.begin())
      {
         Serial.printf("[SHT40] %s — ไม่พบอุปกรณ์บน I2C!\n", _name.c_str());
         return false;
      }
      _sht.setPrecision(SHT4X_HIGH_PRECISION);
      _sht.setHeater(SHT4X_NO_HEATER);
      Serial.printf("[SHT40] %s — init OK\n", _name.c_str());
      return true;
   }

   SensorReading read() override
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

   const char *getName() const override { return _name.c_str(); }
   float getLastHumidity() const { return _lastHumidity; }
   bool isHumidityValid() const { return _humidityValid; }

private:
   String _name;
   Adafruit_SHT4x _sht;
   float _lastHumidity;
   bool _humidityValid;
};

#endif