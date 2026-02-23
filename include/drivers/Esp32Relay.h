#ifndef ESP32_RELAY_H
#define ESP32_RELAY_H
#include <Arduino.h>
#include "../interfaces/IActuator.h"

class Esp32Relay : public IActuator
{
public:
    Esp32Relay(uint8_t pin, const char *name);
    bool begin() override;
    void turnOn() override;
    void turnOff() override;
    bool isOn() const override;
    const char *getName() const override;

private:
    uint8_t _pin;
    const char *_name;
    bool _state;
};
#endif