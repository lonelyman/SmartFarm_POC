// include/drivers/Esp32Ds18b20.h
#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../interfaces/Types.h"

// ข้อมูลเซนเซอร์ 1 ตัว
struct Ds18b20Reading
{
   uint8_t address[8]; // ROM address ของตัวนี้
   float tempC;
   bool isValid;
   char label[16]; // ชื่อที่กำหนดเอง เช่น "Tank-1"
};

class Esp32Ds18b20
{
public:
   Esp32Ds18b20(int pin, uint8_t maxSensors = 4)
       : _ow(pin), _dt(&_ow), _maxSensors(maxSensors), _count(0) {}

   bool begin()
   {
      _dt.begin();
      _count = _dt.getDeviceCount();

      if (_count == 0)
      {
         Serial.println("[DS18B20] ไม่พบ sensor บน 1-Wire bus");
         return false;
      }

      if (_count > _maxSensors)
         _count = _maxSensors;

      Serial.printf("[DS18B20] พบ %d sensor\n", _count);

      // เก็บ address แต่ละตัว + ตั้ง label default
      for (uint8_t i = 0; i < _count; i++)
      {
         _dt.getAddress(_readings[i].address, i);
         snprintf(_readings[i].label, sizeof(_readings[i].label), "Tank-%d", i + 1);
         _readings[i].tempC = 0.0f;
         _readings[i].isValid = false;

         Serial.printf("[DS18B20] Sensor %d address: ", i + 1);
         for (uint8_t b = 0; b < 8; b++)
            Serial.printf("%02X ", _readings[i].address[b]);
         Serial.println();
      }

      _dt.setResolution(12); // ละเอียดสุด 0.0625°C
      return true;
   }

   // อ่านค่าทุกตัวพร้อมกัน (เรียกทุก loop)
   void readAll()
   {
      _dt.requestTemperatures();
      for (uint8_t i = 0; i < _count; i++)
      {
         float t = _dt.getTempC(_readings[i].address);
         if (t == DEVICE_DISCONNECTED_C)
         {
            _readings[i].isValid = false;
            Serial.printf("[DS18B20] Sensor %d disconnected\n", i + 1);
         }
         else
         {
            _readings[i].tempC = t;
            _readings[i].isValid = true;
         }
      }
   }

   uint8_t count() const { return _count; }
   const Ds18b20Reading &get(uint8_t i) const { return _readings[i]; }

   // ตั้งชื่อตัวไหนก็ได้ เช่น setLabel(0, "Tank-A")
   void setLabel(uint8_t i, const char *label)
   {
      if (i < _count)
         snprintf(_readings[i].label, sizeof(_readings[i].label), "%s", label);
   }

private:
   OneWire _ow;
   DallasTemperature _dt;
   uint8_t _maxSensors;
   uint8_t _count;
   Ds18b20Reading _readings[4]; // รองรับสูงสุด DS18B20_MAX_SENSORS
};