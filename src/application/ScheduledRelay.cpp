// src/application/ScheduledRelay.cpp
#include "application/ScheduledRelay.h"

ScheduledRelay::ScheduledRelay(IActuator &relay, ISchedule &schedule)
    : _relay(relay), _schedule(schedule) {}

void ScheduledRelay::update(uint16_t minutesOfDay)
{
    _schedule.isInWindow(minutesOfDay)
        ? _relay.turnOn()
        : _relay.turnOff();
}

bool ScheduledRelay::isOn() const
{
    return _relay.isOn();
}

ISchedule &ScheduledRelay::getSchedule()
{
    return _schedule;
}