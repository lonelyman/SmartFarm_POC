#ifndef FARM_MANAGER_H
#define FARM_MANAGER_H

#include "../domain/AirPumpSchedule.h"
#include "FarmModels.h"

/**
 * สมองกลกลาง (pure application logic)
 * - ไม่แตะ Arduino/FreeRTOS/Relay/SharedState
 * - รับ FarmInput และคืน FarmDecision
 */
class FarmManager
{
public:
    explicit FarmManager(const AirPumpSchedule *schedule);

    FarmDecision update(const FarmInput &in);

private:
    const AirPumpSchedule *_schedule = nullptr;

    // hysteresis latch (internal memory)
    bool _pumpLatched = false;
    bool _mistLatched = false;
    bool _airLatched = false;

private:
    FarmDecision applyManual(const ManualOverrides &m);
    FarmDecision applyAuto(const FarmInput &in);

    bool decideMistByTemp(float tempC, bool valid);
    bool decideAirBySchedule(uint16_t minutesOfDay) const;
};

#endif