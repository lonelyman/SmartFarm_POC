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
#include "drivers/Esp32Sht40.h"
#include "drivers/Esp32Relay.h"
#include "drivers/Esp32WaterLevelInput.h"
#include "drivers/Esp32ManualSwitch.h"
#include "drivers/Esp32Ds18b20.h"

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
   Esp32Sht40 *tempSensor;
   // Water level sensors (XKC-Y25)
   Esp32WaterLevelInput *waterLevelInput;

   Esp32Ds18b20 *waterTempSensor;

   // Actuators
   Esp32Relay *waterPump;
   Esp32Relay *mistSystem;
   Esp32Relay *airPump;

   Esp32ManualSwitch *swManualPump;
   Esp32ManualSwitch *swManualMist;
   Esp32ManualSwitch *swManualAir;

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