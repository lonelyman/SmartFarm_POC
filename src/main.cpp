#include <Arduino.h>

#include "Config.h"

#include "domain/AirPumpSchedule.h"

#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32Sht40.h"
#include "drivers/Esp32Relay.h"
#include "drivers/RtcDs3231Time.h"
#include "drivers/Esp32WaterLevelInput.h"
#include "drivers/Esp32NetModeSwitch.h"

#include "application/FarmManager.h"

#include "infrastructure/Esp32ModeSwitchSource.h"
#include "infrastructure/Esp32WiFiNetwork.h"
#include "infrastructure/SharedState.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/AppBoot.h"
#include "infrastructure/RtcClock.h"
#include "infrastructure/Esp32WebUi.h"

// --- Global Objects (Composition Root) ---

SharedState state;

AirPumpSchedule airSchedule;
Esp32NetModeSwitch gNetSwitch(PIN_SW_NET);
Esp32ModeSwitchSource modeSource(PIN_SW_MODE_A, PIN_SW_MODE_B);
Esp32WiFiNetwork wifiNet;
// Sensors
Esp32Bh1750Light lightSensor("Main-Light");
Esp32Sht40 tempSensor("Main-Temp");

Esp32WaterLevelInput waterLevelInput(PIN_WATER_LEVEL_CH1_LOW_SENSOR, PIN_WATER_LEVEL_CH2_LOW_SENSOR);
// Relays
Esp32Relay waterPump(PIN_RELAY_WATER_PUMP, "Water-Pump");
Esp32Relay mistSystem(PIN_RELAY_MIST, "Mist-System");
Esp32Relay airPump(PIN_RELAY_AIR_PUMP, "Air-Pump");

// Time
RtcDs3231Time rtcTime;

// Brain
FarmManager manager(&airSchedule);

// เราไม่ใช้เวลาปลอมอีกต่อไป – เสมอเชื่อมต่อกับ RTC
RtcClock sysClock(rtcTime);

// Shared task context (avoid extern globals inside tasks)
static SystemContext ctx{
    &state, // state
    nullptr,
    &airSchedule, // airSchedule

    &lightSensor,     // lightSensor
    &tempSensor,      // tempSensor
    &waterLevelInput, // waterLevelInput

    &waterPump,  // waterPump
    &mistSystem, // mistSystem
    &airPump,    // airPump

    &sysClock,   // clock
    &wifiNet,    // network
    &modeSource, // modeSource
    &gNetSwitch,
    &manager, // manager
};

static Esp32WebUi webUi(ctx, 80);

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(100);
    ctx.ui = &webUi;
    AppBoot::setup(ctx);
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(LOOP_IDLE_MS));
}