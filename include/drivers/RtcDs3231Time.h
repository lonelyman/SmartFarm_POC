// include/drivers/RtcDs3231Time.h
#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include "../interfaces/Types.h"

// ============================================================
//  RtcDs3231Time
//  Driver อ่าน/เขียนเวลากับ DS3231 ผ่าน I2C
//
//  วงจรต่อขา:
//    [DS3231 VCC] ──── [3.3V]
//    [DS3231 GND] ──── [GND]
//    [DS3231 SDA](1) ──(1)[GPIO8 (SDA)](1.1)──(1.1)[R4.7kΩ](1.2)──(1.2)[3V3]
//    [DS3231 SCL](2) ──(2)[GPIO9 (SCL)](2.1)──(2.1)[R4.7kΩ](2.2)──(2.2)[3V3]
//
//  ⚠️  R4.7kΩ ค่อมครั้งเดียวที่ bus — ไม่ต้องค่อมซ้ำถ้ามีหลาย device (BH1750, SHT40)
//  ⚠️  Wire.begin() ต้องทำใน AppBoot เท่านั้น
//
//  หน้าที่หลัก:
//    begin()           — ตรวจหา DS3231 บน I2C + ตั้งเวลาจาก compile time ถ้า lostPower
//    getMinutesOfDay() — คืนเวลาเป็นนาทีของวัน (0–1439) ใช้กับตารางเวลา air pump
//    setTimeOfDay()    — ตั้งเวลาจาก CommandTask / WebUI
// ============================================================

class RtcDs3231Time
{
public:
   RtcDs3231Time();

   bool begin();
   bool isOk() const;

   bool getTimeOfDay(TimeOfDay &out);
   bool getMinutesOfDay(uint16_t &minutes);
   bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second = 0);

private:
   RTC_DS3231 _rtc;
   bool _isOk = false;
};