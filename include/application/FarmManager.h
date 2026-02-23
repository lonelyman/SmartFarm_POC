#ifndef FARM_MANAGER_H
#define FARM_MANAGER_H

#include <Arduino.h>
#include "../interfaces/Types.h"
#include "../interfaces/IActuator.h"
#include "../domain/AirPumpSchedule.h"
#include "../Config.h" // เพื่อใช้ DEBUG_CONTROL_LOG

/**
 * สมองกลกลาง:
 * - AUTO  : ใช้ temp คุมหมอก + ใช้เวลา (จาก schedule.json) คุมปั๊มลม
 * - MANUAL: สมองไม่สั่งอะไร รอคนสั่งเองผ่านคำสั่ง -mist/-pump/-air
 * - IDLE  : ปิดทุกอย่าง (โหมดพัก / ปลอดภัย)
 */
class FarmManager
{
public:
    // ตอนนี้มี 3 มือ: ปั๊มน้ำ, หมอก, ปั๊มลม
    FarmManager(IActuator &pump, IActuator &mist, IActuator &air)
        : _pump(pump), _mist(mist), _air(air) {}

    /**
     * ถูกเรียกจาก controlTask ทุก ๆ รอบ
     * @param status        ภาพรวมสถานะจาก SharedState
     * @param minutesOfDay  นาทีของวัน 0..1439
     * @param schedule      ตารางเวลาปั๊มลมที่อ่านจาก schedule.json
     */
    void update(const SystemStatus &status,
                uint16_t minutesOfDay,
                const AirPumpSchedule &schedule)
    {
        switch (status.mode)
        {
        case SystemMode::AUTO:
            runAutoLogic(status, minutesOfDay, schedule);
            break;

        case SystemMode::MANUAL:
            // โหมด MANUAL: สมองไม่สั่ง actuator ใด ๆ
            // ทุกอย่างให้คนสั่งผ่าน commandTask เท่านั้น
            break;

        case SystemMode::IDLE:
            // โหมดพัก: บังคับปิดทุกอย่าง
            forceAllOff();
            break;
        }
    }

private:
    IActuator &_pump;
    IActuator &_mist;
    IActuator &_air;

    // ===================== โหมด IDLE: ปิดทุกอย่าง =====================
    void forceAllOff()
    {
        if (_pump.isOn())
        {
            _pump.turnOff();
            Serial.println("[IDLE] pump OFF");
        }
        if (_mist.isOn())
        {
            _mist.turnOff();
            Serial.println("[IDLE] mist OFF");
        }
        if (_air.isOn())
        {
            _air.turnOff();
            Serial.println("[IDLE] air OFF");
        }
    }

    // ===================== AUTO: รวม logic ทั้งหมด ====================
    void runAutoLogic(const SystemStatus &status,
                      uint16_t minutesOfDay,
                      const AirPumpSchedule &schedule)
    {
        runMistByTemperature(status);             // ใช้ temp คุมหมอก
        runAirBySchedule(minutesOfDay, schedule); // ใช้ schedule.json คุมปั๊มลม
    }

    // ========== ส่วนที่ 1: หมอก ตาม “อุณหภูมิ” (Hysteresis) ==========

    void runMistByTemperature(const SystemStatus &status)
    {
        if (!status.temperature.isValid)
        {
            // ถ้า temp พัง/ไม่ valid → ปิดหมอกเพื่อความปลอดภัย
            if (_mist.isOn())
            {
                _mist.turnOff();
                Serial.println("[AUTO][MIST] OFF (temp invalid)");
            }
            return;
        }

        float t = status.temperature.value;

        // เกณฑ์เบื้องต้น (ปรับทีหลังได้)
        const float TEMP_ON = 32.0f;  // >= 32°C → อยาก "เปิดหมอก"
        const float TEMP_OFF = 29.0f; // <= 29°C → อยาก "ปิดหมอก"

        bool tooHot = (t >= TEMP_ON);
        bool coolDown = (t <= TEMP_OFF);

        // ไม่พ่น log ทุก loop → log เฉพาะตอน "เปลี่ยนสถานะ"
        if (tooHot && !_mist.isOn())
        {
            _mist.turnOn();
            Serial.printf("[AUTO][MIST] ON (temp=%.1f >= %.1f)\n", t, TEMP_ON);
        }
        else if (coolDown && _mist.isOn())
        {
            _mist.turnOff();
            Serial.printf("[AUTO][MIST] OFF (temp=%.1f <= %.1f)\n", t, TEMP_OFF);
        }
        // ถ้าอยู่ระหว่างกลาง (29 < t < 32) → ไม่เปลี่ยนอะไร / ไม่ log
    }

    // ========== ส่วนที่ 2: ปั๊มลม ตาม “ตารางเวลา” จาก JSON ==========

    void runAirBySchedule(uint16_t minutesOfDay,
                          const AirPumpSchedule &schedule)
    {
        bool wantOn = false;

        if (schedule.enabled)
        {
            for (uint8_t i = 0; i < schedule.windowCount; ++i)
            {
                const TimeWindow &w = schedule.windows[i];

                // ตอนนี้รองรับช่วงปกติ ยังไม่รองรับช่วงข้ามเที่ยงคืน
                if (minutesOfDay >= w.startMin &&
                    minutesOfDay < w.endMin)
                {
                    wantOn = true;
                    break;
                }
            }
        }

        bool nowOn = _air.isOn();

#if DEBUG_CONTROL_LOG
        uint16_t hh = minutesOfDay / 60;
        uint16_t mm = minutesOfDay % 60;
        Serial.printf(
            "[AUTO][AIR] %02u:%02u wantOn=%d now=%d windows=%u enabled=%d\n",
            hh, mm,
            wantOn ? 1 : 0,
            nowOn ? 1 : 0,
            (unsigned)schedule.windowCount,
            schedule.enabled ? 1 : 0);
#endif

        // เปลี่ยนสถานะเฉพาะตอนจำเป็น + log แค่ตอนเปลี่ยน
        if (wantOn && !nowOn)
        {
            _air.turnOn();
            Serial.println("[AUTO][AIR] relay ON (by JSON schedule)");
        }
        else if (!wantOn && nowOn)
        {
            _air.turnOff();
            Serial.println("[AUTO][AIR] relay OFF (outside JSON schedule)");
        }
    }
};

#endif