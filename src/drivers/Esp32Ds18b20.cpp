// src/drivers/Esp32Ds18b20.cpp
#include "drivers/Esp32Ds18b20.h"

Esp32Ds18b20::Esp32Ds18b20(int pin, uint8_t maxSensors)
    : _ow(pin), _dt(&_ow), _maxSensors(maxSensors), _count(0) {}

bool Esp32Ds18b20::begin()
{
   // _dt.begin() เรียกได้เองใน driver เพราะ 1-Wire ไม่ใช่ shared bus
   // แต่ละ Esp32Ds18b20 มี OneWire object เป็นของตัวเอง ผูกกับ pin ที่กำหนด
   // ต่างจาก I2C (Wire) ที่เป็น shared bus — ต้องให้ AppBoot init ครั้งเดียว
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

   for (uint8_t i = 0; i < _count; i++)
   {
      _dt.getAddress(_readings[i].address, i);
      snprintf(_readings[i].label, sizeof(_readings[i].label), "Tank-%d", i + 1);
      _readings[i].tempC = 0.0f;
      _readings[i].isValid = false;

      // แสดง ROM address เพื่อช่วย identify ตัวที่ต้องการ setLabel()
      Serial.printf("[DS18B20] Sensor %d: ", i + 1);
      for (uint8_t b = 0; b < 8; b++)
         Serial.printf("%02X ", _readings[i].address[b]);
      Serial.println();
   }

   _dt.setResolution(12); // 12-bit = ละเอียด 0.0625°C
   return true;
}

void Esp32Ds18b20::readAll()
{
   _dt.requestTemperatures();

   for (uint8_t i = 0; i < _count; i++)
   {
      const float t = _dt.getTempC(_readings[i].address);
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

uint8_t Esp32Ds18b20::count() const
{
   return _count;
}

const Ds18b20Reading &Esp32Ds18b20::get(uint8_t i) const
{
   return _readings[i];
}

void Esp32Ds18b20::setLabel(uint8_t i, const char *label)
{
   if (i < _count)
      snprintf(_readings[i].label, sizeof(_readings[i].label), "%s", label);
}