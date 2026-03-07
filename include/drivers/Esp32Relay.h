// include/drivers/Esp32Relay.h
#pragma once

#include <Arduino.h>
#include "../interfaces/IActuator.h"
#include "../Config.h"

// ============================================================
//  Esp32Relay
//  ควบคุม relay / SSR module ผ่าน GPIO
//
//  วงจรต่อขา (active LOW — ค่า default):
//
//    [ESP32-S3 GPIO] ── [IN relay module]
//    [relay module VCC] ── [3.3V หรือ 5V ตามโมดูล]
//    [relay module GND] ── [GND]
//
//  ขั้ว ON/OFF ปรับได้ด้วย RELAY_ACTIVE_LOW ใน Config.h:
//    true  = IN=LOW  → Relay ON  (โมดูลทั่วไป)
//    false = IN=HIGH → Relay ON
//
//  ⚠️  Boot glitch: AppBoot::initRelayPins() ต้อง set INACTIVE
//      ก่อนสร้าง FreeRTOS task — ห้ามรอให้ begin() จัดการเอง
//      เพราะขา floating ตอนบูต + active-low = relay ON ชั่วคราวได้
//
//  Pins (ยึดตาม Config.h):
//    Water Pump : GPIO38
//    Mist       : GPIO40
//    Air Pump   : GPIO41
// ============================================================

class Esp32Relay : public IActuator
{
public:
    Esp32Relay(uint8_t pin, const char *name);

    // init pin OUTPUT + turnOff() — เรียกครั้งเดียวตอน boot
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