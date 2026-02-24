#ifndef FARM_MANAGER_H
#define FARM_MANAGER_H

#include "../interfaces/Types.h"
#include "../interfaces/IActuator.h"
#include "../domain/AirPumpSchedule.h"

/**
 * สมองกลกลาง:
 * - AUTO  : ใช้ temp คุมหมอก + ใช้ตารางเวลา (JSON) คุมปั๊มลม
 * - MANUAL: อ่าน ManualOverrides จาก SharedState แล้วสั่งรีเลย์
 * - IDLE  : ปิดทุกอย่าง (โหมดพัก / ปลอดภัย)
 */
class FarmManager
{
public:
    FarmManager(IActuator &pump,
                IActuator &mist,
                IActuator &air,
                const AirPumpSchedule *schedule)
        : _pump(pump), _mist(mist), _air(air), _schedule(schedule) {}

    void update(const SystemStatus &status,
                const ManualOverrides &manual,
                uint16_t minutesOfDay)
    {
        switch (status.mode)
        {
        case SystemMode::IDLE:
            applyIdle();
            break;
        case SystemMode::MANUAL:
            applyManual(manual);
            break;
        case SystemMode::AUTO:
            applyAuto(status, minutesOfDay);
            break;
        }
    }

private:
    IActuator &_pump;
    IActuator &_mist;
    IActuator &_air;
    const AirPumpSchedule *_schedule; // อ่านอย่างเดียว

    // ----------------------- IDLE -----------------------
    void applyIdle()
    {
        _pump.turnOff();
        _mist.turnOff();
        _air.turnOff();
    }

    // ---------------------- MANUAL ----------------------
    void applyManual(const ManualOverrides &m)
    {
        // ใช้ค่าที่ user สั่งผ่าน CommandTask
        if (m.wantPumpOn)
            _pump.turnOn();
        else
            _pump.turnOff();

        if (m.wantMistOn)
            _mist.turnOn();
        else
            _mist.turnOff();

        if (m.wantAirOn)
            _air.turnOn();
        else
            _air.turnOff();
    }

    // ----------------------- AUTO -----------------------
    void applyAuto(const SystemStatus &status, uint16_t minutesOfDay)
    {
        controlMistByTemperature(status);
        controlAirBySchedule(minutesOfDay);
        // ตอนนี้ยังไม่มี logic น้ำ → ปล่อย pump ปิดไว้
    }

    // หมอกตามอุณหภูมิ (hysteresis)
    void controlMistByTemperature(const SystemStatus &status)
    {
        if (!status.temperature.isValid)
        {
            _mist.turnOff();
            return;
        }

        constexpr float TEMP_ON = 32.0f;
        constexpr float TEMP_OFF = 29.0f;

        float t = status.temperature.value;

        if (t >= TEMP_ON)
        {
            _mist.turnOn();
        }
        else if (t <= TEMP_OFF)
        {
            _mist.turnOff();
        }
        // ถ้าอยู่ระหว่างกลาง → ไม่เปลี่ยนสถานะ
    }

    // ปั๊มลมตามตารางเวลาใน AirPumpSchedule
    void controlAirBySchedule(uint16_t minutesOfDay)
    {
        bool wantOn = false;

        if (_schedule && _schedule->enabled && _schedule->windowCount > 0)
        {
            for (uint8_t i = 0; i < _schedule->windowCount; ++i)
            {
                const TimeWindow &w = _schedule->windows[i];
                if (minutesOfDay >= w.startMin && minutesOfDay < w.endMin)
                {
                    wantOn = true;
                    break;
                }
            }
        }
        else
        {
            // ไม่มี schedule หรือ disabled → ปั๊มลมปิดตลอด
            wantOn = false;
        }

        if (wantOn)
            _air.turnOn();
        else
            _air.turnOff();
    }
};

#endif