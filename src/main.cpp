#include <Arduino.h>

#include "Config.h"

// --- Domain ---
#include "domain/AirPumpSchedule.h"

// --- Application ---
#include "application/FarmManager.h"

// --- Drivers ---
#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32Sht40.h"
#include "drivers/Esp32Ds18b20.h"
#include "drivers/Esp32WaterLevelInput.h"
#include "drivers/Esp32Relay.h"
#include "drivers/Esp32ManualSwitch.h"
#include "drivers/Esp32NetModeSwitch.h"
#include "drivers/RtcDs3231Time.h"

// --- Infrastructure ---
#include "infrastructure/SharedState.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/RtcClock.h"
#include "infrastructure/Esp32ModeSwitchSource.h"
#include "infrastructure/Esp32WiFiNetwork.h"
#include "infrastructure/Esp32WebUi.h"
#include "infrastructure/AppBoot.h"

// ============================================================
//  COMPOSITION ROOT
//  ไฟล์นี้มีหน้าที่เดียว: สร้าง object ทั้งหมด และ wire เข้าหากัน
//  ห้ามมี business logic ในนี้
// ============================================================

// --- Core ---
static SharedState state;
static AirPumpSchedule airSchedule;

// --- Sensors ---
static Esp32Bh1750Light lightSensor("Main-Light");
static Esp32Sht40 tempSensor("Main-Temp");
static Esp32WaterLevelInput waterLevelInput(PIN_WATER_LEVEL_CH1_LOW_SENSOR, PIN_WATER_LEVEL_CH2_LOW_SENSOR);
static Esp32Ds18b20 waterTempSensor(PIN_ONE_WIRE, DS18B20_MAX_SENSORS);

// --- Actuators ---
static Esp32Relay waterPump(PIN_RELAY_WATER_PUMP, "Water-Pump");
static Esp32Relay mistSystem(PIN_RELAY_MIST, "Mist-System");
static Esp32Relay airPump(PIN_RELAY_AIR_PUMP, "Air-Pump");

// --- Switches ---
static Esp32ManualSwitch swManualPump(PIN_SW_MANUAL_PUMP);
static Esp32ManualSwitch swManualMist(PIN_SW_MANUAL_MIST);
static Esp32ManualSwitch swManualAir(PIN_SW_MANUAL_AIR);
static Esp32ModeSwitchSource modeSource(PIN_SW_MODE_A, PIN_SW_MODE_B);
static Esp32NetModeSwitch gNetSwitch(PIN_SW_NET);

// --- Time ---
static RtcDs3231Time rtcTime;
static RtcClock sysClock(rtcTime);

// --- Network & UI ---
static Esp32WiFiNetwork wifiNet;
static Esp32WebUi webUi; // inject ctx ผ่าน setContext() ใน setup()
                         // เพราะ webUi และ ctx ต้องการกันและกัน (circular)

// --- Brain ---
static FarmManager manager(&airSchedule);

// --- System Context ---
// field order ต้องตรงกับ struct SystemContext ใน SystemContext.h ทุกบรรทัด
static SystemContext ctx{
    // Core
    &state,
    nullptr, // ui — wire ใน setup() หลัง webUi พร้อม
    &airSchedule,

    // Sensors
    &lightSensor,
    &tempSensor,
    &waterLevelInput,
    &waterTempSensor,

    // Actuators
    &waterPump,
    &mistSystem,
    &airPump,

    // Switches
    &swManualPump,
    &swManualMist,
    &swManualAir,

    // Time / Network / Mode
    &sysClock,
    &wifiNet,
    &modeSource,
    &gNetSwitch,

    // Brain
    &manager,
};

// ============================================================

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(100);

    // Wire circular dependency ก่อนส่ง AppBoot
    webUi.setContext(&ctx);
    ctx.ui = &webUi;

    AppBoot::setup(ctx);
}

void loop()
{
    // ระบบทำงานผ่าน FreeRTOS tasks ทั้งหมด — loop ไม่มีงานทำ
    vTaskDelay(pdMS_TO_TICKS(LOOP_IDLE_MS));
}