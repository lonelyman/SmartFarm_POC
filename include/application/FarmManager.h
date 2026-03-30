// include/application/FarmManager.h
#pragma once

#include "application/FarmModels.h"

// ============================================================
//  FarmManager — Brain ของระบบ
//  ตัดสินใจเรื่อง water pump และ mist system เท่านั้น
//
//  Air pump ถูกแยกออกไปแล้ว — ScheduledRelay จัดการเอง
//  ใน ControlTask: ctx->scheduledAirPump->update(minutesOfDay)
//
//  ใช้งาน:
//    FarmDecision d = manager.update(in);
//    d.pumpOn → water pump
//    d.mistOn → mist system
// ============================================================

class FarmManager
{
public:
    FarmManager() = default;

    FarmDecision update(const FarmInput &in);

private:
    // hysteresis latch
    bool _pumpLatched = false;
    bool _mistLatched = false;

    // boot guard
    bool _bootGuardDone = false;
    uint32_t _bootTime = 0;

    // mist time guard
    uint32_t _mistOnSince = 0;
    uint32_t _mistOffSince = 0;
    bool _mistForced = false;

    // mist confirmation counter
    // ป้องกันการเปิด/ยกเลิกจากค่า sensor ที่ข้ามเส้นชั่วคราวเพียงรอบเดียว
    uint8_t _mistOnCount  = 0; // นับรอบที่ sensorWantsOn=TRUE ติดต่อกัน
    uint8_t _mistOffCount = 0; // นับรอบที่ sensorWantsOn=FALSE ติดต่อกัน

    FarmDecision applyManual(const ManualOverrides &m);
    FarmDecision applyAuto(const FarmInput &in);

    bool decideMistByTemp(float tempC, bool valid);
    bool decideMistByHumidity(float humRH, bool valid);
    bool decideMistByTempAndHumidity(float tempC, float humRH);
    bool applyMistGuard(bool sensorWantsOn);
};