#ifndef RTC_DS3231_TIME_H
#define RTC_DS3231_TIME_H

#include <Arduino.h>
#include <RTClib.h>              // ใช้ไลบรารี DS3231 (Adafruit RTClib)
#include "../interfaces/Types.h" // ใช้ TimeOfDay / toMinutesOfDay

/**
 * @brief ตัวอ่านเวลาแบบใช้ DS3231 ผ่าน I2C (21/22)
 *
 * ใช้หน้าที่หลัก 2 อย่าง:
 *  - begin()          : เรียกตอน boot เพื่อตรวจว่าเจอ DS3231 ไหม
 *  - getMinutesOfDay(): ขอเวลาเป็นนาทีของวัน (0..1439)
 */
class RtcDs3231Time
{
public:
   RtcDs3231Time();

   // เรียกจาก setup()
   bool begin();

   bool isOk() const;

   // ขอเวลาแบบชั่วโมง/นาที/วินาที
   bool getTimeOfDay(TimeOfDay &out);

   // ขอเป็น "นาทีของวัน" 0..1439 (ใช้กับตารางเวลา)
   bool getMinutesOfDay(uint16_t &minutes);

   bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second = 0);

private:
   RTC_DS3231 _rtc;
   bool _isOk;
};

#endif