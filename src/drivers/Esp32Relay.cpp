// src/drivers/Esp32Relay.cpp
#include "drivers/Esp32Relay.h"
#include "Config.h"

Esp32Relay::Esp32Relay(uint8_t pin, const char *name)
    : _pin(pin), _name(name), _state(false) {}

bool Esp32Relay::begin()
{
    pinMode(_pin, OUTPUT);
    turnOff(); // safety: ปิดเสมอตอน boot
    return true;
}

void Esp32Relay::turnOn()
{
    // normalize ตาม polarity ของโมดูล (ดู RELAY_ACTIVE_LOW ใน Config.h)
    digitalWrite(_pin, RELAY_ACTIVE_LOW ? LOW : HIGH);
    _state = true;
}

void Esp32Relay::turnOff()
{
    // normalize ตาม polarity ของโมดูล (ดู RELAY_ACTIVE_LOW ใน Config.h)
    digitalWrite(_pin, RELAY_ACTIVE_LOW ? HIGH : LOW);
    _state = false;
}

bool Esp32Relay::isOn() const
{
    return _state;
}

const char *Esp32Relay::getName() const
{
    return _name;
}