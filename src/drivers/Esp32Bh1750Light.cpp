#include "drivers/Esp32Bh1750Light.h"

#include <Arduino.h>
#include <math.h> // isnan()
#include <Wire.h> // Wire (I2C)

// NOTE:
// - I2C bus MUST be started once in AppBoot (Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)).
// - This driver only initializes and reads the BH1750 device.

Esp32Bh1750Light::Esp32Bh1750Light(const char *name, uint8_t address)
    : _name(name), _address(address), _bh1750(address)
{
}

bool Esp32Bh1750Light::begin()
{
    // Do NOT call Wire.begin() here (AppBoot owns I2C bus init)

    // Initialize BH1750 in continuous high resolution mode
    if (!_bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    {
        Serial.println("[Esp32Bh1750Light] Failed to init BH1750!");
        return false;
    }

    Serial.print("[Esp32Bh1750Light] Init OK on address 0x");
    Serial.println(_address, HEX);
    return true;
}

SensorReading Esp32Bh1750Light::read()
{
    // Read lux from sensor
    float lux = _bh1750.readLightLevel(); // unit: Lux

    const bool ok = !isnan(lux) && lux >= 0.0f;

    if (!ok)
    {
        // mark invalid
        return SensorReading(0.0f, false, (uint32_t)millis());
    }

    return SensorReading(lux, true, (uint32_t)millis());
}