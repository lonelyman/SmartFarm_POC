#include <Arduino.h>

#include "Config.h"

#include "domain/AirPumpSchedule.h"

#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/Esp32Relay.h"
#include "drivers/RtcDs3231Time.h"

#include "application/FarmManager.h"

#include "infrastructure/Esp32ModeSwitchSource.h"
#include "infrastructure/Esp32WiFiNetwork.h"
#include "infrastructure/SharedState.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/AppBoot.h"
#include "infrastructure/FakeClock.h"
#include "infrastructure/RtcClock.h"

// --- Global Objects (Composition Root) ---

SharedState state;
AirPumpSchedule airSchedule;

Esp32ModeSwitchSource modeSource(PIN_SW_MODE_A, PIN_SW_MODE_B);
Esp32WiFiNetwork wifiNet(WIFI_SSID, WIFI_PASSWORD);
// Sensors
Esp32Bh1750Light lightSensor("Main-Light");
Esp32FakeTemperature tempSensor("Main-Temp", 30.0f);

// Relays
Esp32Relay waterPump(PIN_RELAY_WATER_PUMP, "Water-Pump");
Esp32Relay mistSystem(PIN_RELAY_MIST, "Mist-System");
Esp32Relay airPump(PIN_RELAY_AIR_PUMP, "Air-Pump");

// Time
RtcDs3231Time rtcTime;

// Brain
FarmManager manager(&airSchedule);

#ifdef USE_FAKE_TIME
FakeClock sysClock(FAKE_MINUTES_OF_DAY);
#else
RtcClock sysClock(rtcTime);
#endif

// Shared task context (avoid extern globals inside tasks)
static SystemContext ctx{
    &state,
    &airSchedule,
    &lightSensor,
    &tempSensor,
    &waterPump,
    &mistSystem,
    &airPump,
    &sysClock,
    &wifiNet,
    &modeSource,
    &manager,
};

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(100);

    AppBoot::setup(ctx);
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(LOOP_IDLE_MS));
}