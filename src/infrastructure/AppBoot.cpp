#include "infrastructure/AppBoot.h"

#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>

#include "Config.h"
#include "tasks/TaskEntrypoints.h"
#include "infrastructure/ScheduleStore.h"
#include "infrastructure/NetTimeSync.h"

namespace
{
   void printBanner()
   {
      Serial.println();
      Serial.println("===== SmartFarm Booting... =====");
   }

   void initI2C()
   {
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
   }

   void initFileSystemAndSchedule(AirPumpSchedule &airSchedule)
   {
      if (!LittleFS.begin())
      {
         Serial.println("⚠️ LittleFS mount failed, air schedule disabled");
         airSchedule.enabled = false;
         airSchedule.windowCount = 0;
         return;
      }

      Serial.println("✅ LittleFS mount success, loading air schedule...");
      if (!loadAirScheduleFromFS("/schedule.json", airSchedule))
      {
         Serial.println("⚠️ load schedule failed, no schedule");
         airSchedule.enabled = false;
         airSchedule.windowCount = 0;
         return;
      }

      Serial.printf("✅ air schedule loaded: enabled=%d windows=%u\n",
                    airSchedule.enabled ? 1 : 0,
                    (unsigned)airSchedule.windowCount);
   }

   void initModeSource(SystemContext &ctx)
   {
      if (ctx.modeSource == nullptr)
      {
         Serial.println("⚠️ modeSource is null (mode switch disabled)");
         return;
      }
      ctx.modeSource->begin();
   }

   void initDrivers(SystemContext &ctx)
   {
      ctx.lightSensor->begin();
      ctx.tempSensor->begin();

      ctx.waterPump->begin();
      ctx.mistSystem->begin();
      ctx.airPump->begin();
   }

   void initClock(SystemContext &ctx)
   {
      if (ctx.clock == nullptr)
      {
         Serial.println("⚠️ clock is null (time disabled)");
         return;
      }

#if DEBUG_TIME_LOG
      Serial.println("[BOOT] clock.begin() ...");
#endif
      ctx.clock->begin();
   }

   void setInitialSafeState(SystemContext &ctx)
   {
      ctx.state->setMode(SystemMode::IDLE);
   }

   void startTasks(SystemContext &ctx)
   {
      xTaskCreatePinnedToCore(inputTask, "In", INPUT_TASK_STACK, &ctx, 1, nullptr, 1);
      xTaskCreatePinnedToCore(controlTask, "Ctrl", CONTROL_TASK_STACK, &ctx, 2, nullptr, 1);
      xTaskCreatePinnedToCore(commandTask, "Cmd", COMMAND_TASK_STACK, &ctx, 1, nullptr, 1);
   }
}

void AppBoot::setup(SystemContext &ctx)
{
   printBanner();

   initI2C();
   initFileSystemAndSchedule(*ctx.airSchedule);

   // ✅ แทน initModeSwitchPins() ด้วย ModeSource (Adapter)
   initModeSource(ctx);

   initDrivers(ctx);
   initClock(ctx);
   setInitialSafeState(ctx);
   startTasks(ctx);

   Serial.println("🚀 SmartFarm System: Ready and Linked.");
}