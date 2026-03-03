// include/infrastructure/SystemContext.h
#pragma once

#include "infrastructure/SharedState.h"
#include "interfaces/INetwork.h"

#include "interfaces/IModeSource.h"
#include "interfaces/IClock.h"
#include "interfaces/IUi.h"
#include "interfaces/INetModeSource.h"

#include "domain/AirPumpSchedule.h"

#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/Esp32Relay.h"
#include "drivers/Esp32WaterLevelInput.h"

#include "application/FarmManager.h"

// Composition root object graph holder.
// Pass a pointer to this into FreeRTOS tasks instead of relying on extern globals.
struct SystemContext
{
   SharedState *state;
   IUi *ui;
   AirPumpSchedule *airSchedule;

   // Sensors
   Esp32Bh1750Light *lightSensor;
   Esp32FakeTemperature *tempSensor;
   // Water level sensors (XKC-Y25)
   Esp32WaterLevelInput *waterLevelInput;

   // Actuators
   Esp32Relay *waterPump;
   Esp32Relay *mistSystem;
   Esp32Relay *airPump;

   // Clock abstraction
   IClock *clock;

   INetwork *network;

   // Mode source
   IModeSource *modeSource;

   // Net mode source (AP/STA switch)
   INetModeSource *netModeSource;
   // Brain
   FarmManager *manager;
};