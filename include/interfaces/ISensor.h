// include/interfaces/ISensor.h
#pragma once

#include "Types.h"

// ============================================================
//  ISensor — Interface สำหรับเซนเซอร์ทุกประเภท
//
//  Implementation ที่มีอยู่:
//    - Esp32Bh1750Light : เซนเซอร์แสง BH1750 (I2C)
//    - Esp32Sht40       : เซนเซอร์อุณหภูมิ/ความชื้น SHT40 (I2C)
//
//  ใช้งาน:
//    - lightSensor, tempSensor อยู่ใน SystemContext
//    - inputTask เรียก read() ทุก loop แล้วส่งค่าเข้า SharedState
// ============================================================

class ISensor
{
public:
    virtual ~ISensor() = default;

    // init hardware — เรียกครั้งเดียวตอน boot
    virtual bool begin() = 0;

    // อ่านค่าเซนเซอร์ — คืน SensorReading พร้อม isValid flag
    virtual SensorReading read() = 0;

    virtual const char *getName() const = 0;
};