#ifndef FARM_MANAGER_H
#define FARM_MANAGER_H

#include <Arduino.h>
#include "../Config.h" // ใช้ DEBUG_CONTROL_LOG / DEBUG_AIR_LOG ถ้าต้องการ
#include "../interfaces/Types.h"
#include "../interfaces/IActuator.h"
#include "../domain/AirPumpSchedule.h"

/**
 * สมองกลกลาง:
 * - AUTO  : ใช้ temp คุมหมอก + ใช้ "ตารางเวลา" (minutesOfDay) คุมปั๊มลม
 * - MANUAL: ตอนนี้ยังไม่สั่ง actuator เอง (ปล่อยให้ CommandTask คุมรีเลย์ตรงอยู่เหมือนเดิม)
 * - IDLE  : ปิดทุกอย่าง (โหมดพัก / ปลอดภัย)
 */
class FarmManager
{
public:
    // มี 3 มือ: ปั๊มน้ำ, หมอก, ปั๊มลม + ตารางเวลา (อาจมาจาก JSON)
    FarmManager(IActuator &pump,
                IActuator &mist,
                IActuator &air,
                const AirPumpSchedule *schedule)
        : _pump(pump),
          _mist(mist),
          _air(air),
          _schedule(schedule) {}

    /**
     * ถูกเรียกจาก ControlTask ทุก ๆ รอบ
     * @param status       ภาพรวมสถานะจาก SharedState
     * @param minutesOfDay นาทีของวัน 0..1439 (ใช้กับตารางเวลา)
     * @param manual       ตอนนี้ยังไม่ใช้ (MANUAL ยังให้ CommandTask คุมรีเลย์ตรง)
     */
    void update(const SystemStatus &status,
                uint16_t minutesOfDay,
                const ManualOverrides &manual)
    {
        (void)manual; // ยังไม่ใช้ manual ในสมอง (กัน warning ไปก่อน)

        switch (status.mode)
        {
        case SystemMode::AUTO:
            runAutoLogic(status, minutesOfDay);
            break;

        case SystemMode::MANUAL:
            // ตอนนี้ MANUAL: สมองไม่สั่ง relay ใด ๆ
            // ให้ CommandTask ไปสั่ง waterPump/mistSystem/airPump ตรงแทน
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
    const AirPumpSchedule *_schedule; // ตารางเวลาจาก JSON (หรือ nullptr ถ้าไม่มี)

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
    void runAutoLogic(const SystemStatus &status, uint16_t minutesOfDay)
    {
        runMistByTemperature(status);   // ใช้ temp คุมหมอก
        runAirBySchedule(minutesOfDay); // ใช้เวลา (+ schedule) คุมปั๊มลม
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

        const float TEMP_ON = 32.0f;  // >= 32°C → อยาก "เปิดหมอก"
        const float TEMP_OFF = 29.0f; // <= 29°C → อยาก "ปิดหมอก"

        bool tooHot = (t >= TEMP_ON);
        bool coolDown = (t <= TEMP_OFF);

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

    // ========== ส่วนที่ 2: ปั๊มลม ตาม “ตารางเวลา” ==========

    // helper: เช็คว่าอยู่ในช่วงเวลาหรือเปล่า
    bool isWithinWindow(uint16_t minutes,
                        uint16_t startMin,
                        uint16_t endMin) const
    {
        // ตอนนี้รองรับเฉพาะช่วงปกติ start < end (ยังไม่รองรับข้ามเที่ยงคืน)
        return (minutes >= startMin) && (minutes < endMin);
    }

    // ตัดสินใจว่า "อยากเปิดปั๊มลมไหม" จาก schedule
    bool shouldAirOn(uint16_t minutesOfDay) const
    {
        // 1) ถ้ามี schedule จาก JSON และถูก enable → ใช้รายการ windows นั้นเลย
        if (_schedule && _schedule->enabled && _schedule->windowCount > 0)
        {
            for (uint8_t i = 0; i < _schedule->windowCount; ++i)
            {
                const TimeWindow &w = _schedule->windows[i];
                if (isWithinWindow(minutesOfDay, w.startMin, w.endMin))
                {
                    return true;
                }
            }
            return false;
        }

        // 2) ถ้าไม่มี schedule (หรือ disabled) → fallback เป็นค่า hardcode เดิม
        if (isWithinWindow(minutesOfDay, 7 * 60, 12 * 60)) // 07:00–12:00
            return true;
        if (isWithinWindow(minutesOfDay, 14 * 60, 17 * 60 + 30)) // 14:00–17:30
            return true;

        return false;
    }

    void runAirBySchedule(uint16_t minutesOfDay)
    {
        bool wantOn = shouldAirOn(minutesOfDay);
        bool nowOn = _air.isOn();

#if DEBUG_CONTROL_LOG
        // ถ้าอยากดูละเอียด เปิด DEBUG_CONTROL_LOG ใน Config.h ได้เลย
        uint16_t hh = minutesOfDay / 60;
        uint16_t mm = minutesOfDay % 60;
        Serial.printf("[AUTO][AIR] %02u:%02u wantOn=%d now=%d\n",
                      hh, mm,
                      wantOn ? 1 : 0,
                      nowOn ? 1 : 0);
#endif

        if (wantOn && !nowOn)
        {
            _air.turnOn();
            Serial.println("[AUTO][AIR] relay ON (by schedule)");
        }
        else if (!wantOn && nowOn)
        {
            _air.turnOff();
            Serial.println("[AUTO][AIR] relay OFF (outside schedule)");
        }
    }
};

#endif