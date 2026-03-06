#include "drivers/Esp32Relay.h"
#include "Config.h"

/**
 * @brief Constructor สำหรับตั้งค่าเริ่มต้นของ Relay
 */
Esp32Relay::Esp32Relay(uint8_t pin, const char* name) 
    : _pin(pin), _name(name), _state(false) {}

/**
 * @brief เริ่มต้นกำหนดโหมดของขา Pin
 */
bool Esp32Relay::begin() {
    pinMode(_pin, OUTPUT);
    turnOff(); // เพื่อความปลอดภัย เริ่มต้นต้องปิดเสมอ
    return true;
}

/**
 * @brief สั่งเปิดอุปกรณ์ (Digital HIGH)
 */
void Esp32Relay::turnOn() {
    // normalize ตาม polarity ของโมดูล
    const uint8_t level = RELAY_ACTIVE_LOW ? LOW : HIGH;
    digitalWrite(_pin, level);
    _state = true;
}

/**
 * @brief สั่งปิดอุปกรณ์ (Digital LOW)
 */
void Esp32Relay::turnOff() {
    // normalize ตาม polarity ของโมดูล
    const uint8_t level = RELAY_ACTIVE_LOW ? HIGH : LOW;
    digitalWrite(_pin, level);
    _state = false;
}

/**
 * @brief ตรวจสอบสถานะปัจจุบัน
 */
bool Esp32Relay::isOn() const {
    return _state;
}

/**
 * @brief คืนค่าชื่ออุปกรณ์
 */
const char* Esp32Relay::getName() const {
    return _name;
}