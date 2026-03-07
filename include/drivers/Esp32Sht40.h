// include/drivers/Esp32Sht40.h
#pragma once

#include <Arduino.h>
#include <Adafruit_SHT4x.h>
#include "../interfaces/ISensor.h"

// ============================================================
//  Esp32Sht40
//  Driver เซนเซอร์อุณหภูมิและความชื้น SHT40 ผ่าน I2C
//
//  วงจรต่อขา:
//    [SHT40 VCC] ──── [3.3V]
//    [SHT40 GND] ──── [GND]
//    [SHT40 SDA](1) ──(1)[GPIO8 (SDA)](1.1)──(1.1)[R4.7kΩ](1.2)──(1.2)[3V3]
//    [SHT40 SCL](2) ──(2)[GPIO9 (SCL)](2.1)──(2.1)[R4.7kΩ](2.2)──(2.2)[3V3]
//
//  ⚠️  R4.7kΩ ค่อมครั้งเดียวที่ bus — ไม่ต้องค่อมซ้ำถ้ามีหลาย device (BH1750, DS3231)
//  ⚠️  Wire.begin() ต้องทำใน AppBoot เท่านั้น — driver นี้ไม่เรียก Wire.begin() เอง
//
//  read() คืนค่าอุณหภูมิเป็น SensorReading
//  ความชื้นอ่านแยกผ่าน getLastHumidity() / isHumidityValid()
// ============================================================

class Esp32Sht40 : public ISensor
{
public:
   explicit Esp32Sht40(const char *name = "SHT40");

   bool begin() override;
   SensorReading read() override;
   const char *getName() const override;

   float getLastHumidity() const;
   bool isHumidityValid() const;

private:
   String _name;
   Adafruit_SHT4x _sht;
   float _lastHumidity = 0.0f;
   bool _humidityValid = false;
};