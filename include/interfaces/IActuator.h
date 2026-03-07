// include/interfaces/IActuator.h
#pragma once

// ============================================================
//  IActuator — Interface สำหรับอุปกรณ์สั่งการ (Output)
//
//  Implementation ที่มีอยู่:
//    - Esp32Relay : relay / SSR module (active LOW หรือ HIGH ตาม Config.h)
//
//  ใช้งาน:
//    - waterPump, mistSystem, airPump อยู่ใน SystemContext
//    - controlTask เรียก turnOn()/turnOff() ตาม FarmDecision
// ============================================================

class IActuator
{
public:
    virtual ~IActuator() = default;

    // init pin / resources — เรียกครั้งเดียวตอน boot
    virtual bool begin() = 0;

    virtual void turnOn() = 0;
    virtual void turnOff() = 0;

    // อ่านสถานะปัจจุบัน — const เพราะไม่แก้ไข state
    virtual bool isOn() const = 0;
    virtual const char *getName() const = 0;
};