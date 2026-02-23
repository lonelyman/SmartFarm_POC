#ifndef ESP32_FAKE_TEMPERATURE_H
#define ESP32_FAKE_TEMPERATURE_H

#include <Arduino.h>
#include "../interfaces/ISensor.h"
#include "../interfaces/Types.h"

/**
 * เซนเซอร์อุณหภูมิปลอม เอาไว้จำลองจนกว่าจะมีเซนเซอร์จริง
 * - มี begin()
 * - มี read() คืนค่า SensorReading
 * - มี getName() ตามสัญญา ISensor
 */
class Esp32FakeTemperature : public ISensor
{
public:
   Esp32FakeTemperature(const char *name, float baseTempC = 30.0f)
       : _name(name), _baseTempC(baseTempC) {}

   // --- implement interface ISensor ---

   bool begin() override
   {
      Serial.printf("[TempFake] %s init (base=%.1f C)\n",
                    _name.c_str(), _baseTempC);
      return true;
   }

   SensorReading read() override
   {
      // ตอนนี้ปล่อยเป็นค่าคงที่ไปก่อน
      return SensorReading(_baseTempC, true, millis());
   }

   const char *getName() const override
   {
      return _name.c_str();
   }

   // --- extra สำหรับปรับค่าเล่นระหว่าง dev ---
   void setBaseTemp(float t)
   {
      _baseTempC = t;
   }

private:
   String _name;
   float _baseTempC;
};

#endif