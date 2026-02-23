#include "drivers/Esp32Bh1750Light.h"
#include <math.h> // สำหรับ isnan()

// static member ต้องนิยามตัวแปรจริงที่นี่
bool Esp32Bh1750Light::_i2cInitialized = false;

Esp32Bh1750Light::Esp32Bh1750Light(const char* name, uint8_t address)
    : _name(name), _address(address), _bh1750(address)
{
}

bool Esp32Bh1750Light::begin() {
    // เริ่ม I2C bus แค่ครั้งแรกครั้งเดียว
    if (!_i2cInitialized) {
        // กรณีทั่วไปของ ESP32: SDA=21, SCL=22
        Wire.begin(21, 22);
        _i2cInitialized = true;
    }

    // เริ่มต้น BH1750 ในโหมดอ่านต่อเนื่องความละเอียดสูง
    if (!_bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("[Esp32Bh1750Light] Failed to init BH1750!");
        return false;
    }

    Serial.print("[Esp32Bh1750Light] Init OK on address 0x");
    Serial.println(_address, HEX);
    return true;
}

SensorReading Esp32Bh1750Light::read() {
    // อ่านค่าแสงเป็น Lux ตรง ๆ จากเซนเซอร์
    float lux = _bh1750.readLightLevel(); // หน่วย Lux

    bool ok = !isnan(lux) && lux >= 0.0f;

    if (!ok) {
        Serial.println("[Esp32Bh1750Light] Read error, mark invalid");
        return SensorReading(0.0f, false, (uint32_t)millis());
    }

    // ข้อมูลปกติ
    return SensorReading(lux, true, (uint32_t)millis());
}