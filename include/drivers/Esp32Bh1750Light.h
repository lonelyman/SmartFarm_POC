#ifndef ESP32_BH1750_LIGHT_H
#define ESP32_BH1750_LIGHT_H

#include <Arduino.h>
#include <BH1750.h>
#include "../interfaces/ISensor.h"

/**
 * @brief Driver สำหรับเซนเซอร์แสงรุ่น BH1750 ผ่าน I2C บน ESP32
 * NOTE: I2C (Wire.begin) ต้องทำใน AppBoot ที่เดียว
 */
class Esp32Bh1750Light : public ISensor
{
public:
    /**
     * @brief สร้างออบเจ็กต์เซนเซอร์ BH1750
     * @param name ชื่อเซนเซอร์ (ใช้สำหรับ log / debug)
     * @param address ที่อยู่ I2C ของ BH1750 (ปกติ 0x23 หรือ 0x5C)
     */
    Esp32Bh1750Light(const char *name, uint8_t address = 0x23);

    bool begin() override;
    SensorReading read() override;
    const char *getName() const override { return _name; }

private:
    const char *_name;
    uint8_t _address;
    BH1750 _bh1750;
};

#endif