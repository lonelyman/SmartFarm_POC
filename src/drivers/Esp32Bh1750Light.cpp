// src/drivers/Esp32Bh1750Light.cpp
#include "drivers/Esp32Bh1750Light.h"

#include <math.h> // isnan()
#include <Wire.h>

Esp32Bh1750Light::Esp32Bh1750Light(const char *name, uint8_t address)
    : _name(name), _address(address), _bh1750(address) {}

bool Esp32Bh1750Light::begin()
{
    // Wire.begin() อยู่ใน AppBoot — ห้ามเรียกซ้ำที่นี่
    if (!_bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    {
        Serial.printf("[BH1750] %s — ไม่พบอุปกรณ์บน I2C (0x%02X)\n", _name, _address);
        return false;
    }

    Serial.printf("[BH1750] %s — init OK (0x%02X)\n", _name, _address);
    return true;
}

SensorReading Esp32Bh1750Light::read()
{
    const float lux = _bh1750.readLightLevel();
    const bool ok = !isnan(lux) && lux >= 0.0f;
    const uint32_t now = (uint32_t)millis();

    return SensorReading(ok ? lux : 0.0f, ok, now);
}