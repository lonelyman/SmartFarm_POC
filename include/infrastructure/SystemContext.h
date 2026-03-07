// include/infrastructure/SystemContext.h
#pragma once

// --- Infrastructure ---
#include "infrastructure/SharedState.h"

// --- Interfaces ---
#include "interfaces/IClock.h"
#include "interfaces/IModeSource.h"
#include "interfaces/INetModeSource.h"
#include "interfaces/INetwork.h"
#include "interfaces/IUi.h"

// --- Drivers ---
#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32Ds18b20.h"
#include "drivers/Esp32ManualSwitch.h"
#include "drivers/Esp32NetModeSwitch.h"
#include "drivers/Esp32Relay.h"
#include "drivers/Esp32Sht40.h"
#include "drivers/Esp32WaterLevelInput.h"

// --- Application ---
#include "application/FarmManager.h"
#include "application/ScheduledRelay.h"

// ============================================================
//  SystemContext — Object Graph Holder
//  สร้างใน main.cpp (Composition Root) และส่งเป็น pointer
//  เข้า FreeRTOS tasks แทนการใช้ extern global
// ============================================================

struct SystemContext
{
   // --- Core ---
   SharedState *state;
   IUi *ui;

   // --- Sensors ---
   Esp32Bh1750Light *lightSensor;
   Esp32Sht40 *tempSensor;
   Esp32WaterLevelInput *waterLevelInput;
   Esp32Ds18b20 *waterTempSensor;

   // --- Actuators ---
   Esp32Relay *waterPump;
   Esp32Relay *mistSystem;
   Esp32Relay *airPump;

   // --- Switches ---
   Esp32ManualSwitch *swManualPump;
   Esp32ManualSwitch *swManualMist;
   Esp32ManualSwitch *swManualAir;

   // --- Time / Network / Mode ---
   IClock *clock;
   INetwork *network;
   IModeSource *modeSource;
   INetModeSource *netModeSource;

   // --- Brain ---
   FarmManager *manager;
   ScheduledRelay *scheduledAirPump; // air pump — ScheduledRelay เป็นเจ้าของ
};