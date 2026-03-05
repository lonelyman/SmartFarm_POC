#ifndef FARM_MANAGER_H
#define FARM_MANAGER_H

#include "../domain/AirPumpSchedule.h"
#include "FarmModels.h"

class FarmManager
{
public:
    explicit FarmManager(const AirPumpSchedule *schedule);

    FarmDecision update(const FarmInput &in);

private:
    const AirPumpSchedule *_schedule = nullptr;

    // hysteresis latch
    bool _pumpLatched = false;
    bool _mistLatched = false;
    bool _airLatched = false;

    // boot guard
    bool _bootGuardDone = false;
    uint32_t _bootTime = 0;

    // mist time guard
    uint32_t _mistOnSince = 0;
    uint32_t _mistOffSince = 0;
    bool _mistForced = false;

    FarmDecision applyManual(const ManualOverrides &m);
    FarmDecision applyAuto(const FarmInput &in);

    bool decideMistByTemp(float tempC, bool valid);
    bool decideMistByHumidity(float humRH, bool valid);
    bool decideMistByTempAndHumidity(float tempC, float humRH);
    bool applyMistGuard(bool sensorWantsOn);
    bool decideAirBySchedule(uint16_t minutesOfDay) const;
};

#endif