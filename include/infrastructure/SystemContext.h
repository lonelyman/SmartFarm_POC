#pragma once

#include "interfaces/IModeSource.h"
#include "infrastructure/SharedState.h"
#include "interfaces/IClock.h"

#include "domain/AirPumpSchedule.h"

#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/Esp32Relay.h"
#include "drivers/RtcDs3231Time.h"

#include "application/FarmManager.h"

// Composition root object graph holder.
// Pass a pointer to this into FreeRTOS tasks instead of relying on extern globals.
struct SystemContext
{
   SharedState *state;
   AirPumpSchedule *airSchedule;

   // Sensors
   Esp32Bh1750Light *lightSensor;
   Esp32FakeTemperature *tempSensor;

   // Actuators
   Esp32Relay *waterPump;
   Esp32Relay *mistSystem;
   Esp32Relay *airPump;

      // Clock abstraction
   IClock *clock;

   // Mode source
   IModeSource *modeSource;
   // Brain
   FarmManager *manager;
};