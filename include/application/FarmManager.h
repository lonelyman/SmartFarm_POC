#ifndef FARM_MANAGER_H
#define FARM_MANAGER_H

#include "../interfaces/Types.h"
#include "../domain/AirPumpSchedule.h"

struct FarmDecision
{
    bool pumpOn = false;
    bool mistOn = false;
    bool airOn = false;
};

class FarmManager
{
public:
    explicit FarmManager(const AirPumpSchedule *schedule);

    FarmDecision update(const SystemStatus &status,
                        const ManualOverrides &manual,
                        uint16_t minutesOfDay);

private:
    const AirPumpSchedule *_schedule = nullptr;

    bool _pumpLatched = false;
    bool _mistLatched = false;
    bool _airLatched = false;

private:
    FarmDecision applyManual(const ManualOverrides &m);
    FarmDecision applyAuto(const SystemStatus &status, uint16_t minutesOfDay);

    bool decideMistByTemp(const SystemStatus &status);
    bool decideAirBySchedule(uint16_t minutesOfDay) const;
};

#endif