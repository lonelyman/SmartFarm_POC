#include "drivers/Esp32Relay.h"

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
    digitalWrite(_pin, HIGH); 
    _state = true;
}

/**
 * @brief สั่งปิดอุปกรณ์ (Digital LOW)
 */
void Esp32Relay::turnOff() {
    digitalWrite(_pin, LOW);
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