// include/drivers/Esp32Bh1750Light.h
#pragma once

#include <Arduino.h>
#include <BH1750.h>
#include "../interfaces/ISensor.h"

// ============================================================
//  Esp32Bh1750Light
//  Driver เซนเซอร์แสง BH1750 ผ่าน I2C
//
//  วงจรต่อขา:
//    [BH1750 VCC] ──── [3.3V]
//    [BH1750 GND] ──── [GND]
//    [BH1750 SDA](1) ──(1)[GPIO8 (SDA)](1.1)──(1.1)[R4.7kΩ](1.2)──(1.2)[3V3]
//    [BH1750 SCL](2) ──(2)[GPIO9 (SCL)](2.1)──(2.1)[R4.7kΩ](2.2)──(2.2)[3V3]
//    [BH1750 ADD] ── ยังไม่ใช้ (GND=0x23 / 3V3=0x5C)
//
//  ⚠️  R4.7kΩ ค่อมครั้งเดียวที่ bus — ไม่ต้องค่อมซ้ำถ้ามีหลาย device (SHT40, DS3231)
//
//  ⚠️  Wire.begin() ต้องทำใน AppBoot เท่านั้น
//      driver นี้ไม่เรียก Wire.begin() เอง
//
//  ⚠️  ต้องมี external pull-up 4.7kΩ–10kΩ ที่ SDA และ SCL
//      ถ้าลืม: I2C scan จะไม่เจอ device เลย
// ============================================================

class Esp32Bh1750Light : public ISensor
{
public:
    // address: 0x23 (ADD=GND, default) หรือ 0x5C (ADD=VCC)
    Esp32Bh1750Light(const char *name, uint8_t address = 0x23);

    bool begin() override;
    SensorReading read() override;
    const char *getName() const override { return _name; }

private:
    const char *_name;
    uint8_t _address;
    BH1750 _bh1750;
};