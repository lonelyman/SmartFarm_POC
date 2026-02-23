#include <Arduino.h>
#include <LittleFS.h>

#include "Config.h"
#include "application/FarmManager.h"
#include "infrastructure/SharedState.h"
#include "infrastructure/ScheduleStore.h"

#include "domain/AirPumpSchedule.h"

#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/Esp32Relay.h"
#include "drivers/RtcDs3231Time.h"

// --- Global Objects ---
SharedState state;

// เซนเซอร์แสง
Esp32Bh1750Light lightSensor("Main-Light");

// เซนเซอร์อุณหภูมิปลอม (เอาไว้ให้ flow ทำงานเหมือนของจริง)
Esp32FakeTemperature tempSensor("Main-Temp", 30.0f);

// รีเลย์
Esp32Relay waterPump(PIN_RELAY_WATER_PUMP, "Water-Pump");
Esp32Relay mistSystem(PIN_RELAY_MIST, "Mist-System");
Esp32Relay airPump(PIN_RELAY_AIR_PUMP, "Air-Pump");

// เวลา (ตอนนี้ยังใช้ FAKE_TIME อยู่ แต่ object นี้ก็พร้อมใช้แล้ว)
RtcDs3231Time rtcTime;

// ตารางเวลา air pump (โหลดจาก JSON)
AirPumpSchedule airSchedule;

// สมองกลาง (ส่ง pointer ของ schedule เข้าไป)
FarmManager manager(waterPump, mistSystem, airPump);

// --- Task Forward Declarations ---
void inputTask(void *pvParameters);
void controlTask(void *pvParameters);
void commandTask(void *pvParameters);

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_SW_MODE_A, INPUT_PULLUP);
    pinMode(PIN_SW_MODE_B, INPUT_PULLUP);

    // 1. ตื่นตัวฮาร์ดแวร์
    lightSensor.begin();
    tempSensor.begin();
    waterPump.begin();
    mistSystem.begin();
    airPump.begin();

#ifndef USE_FAKE_TIME
    rtcTime.begin();
#endif

    // 1.5 mount LittleFS + โหลด schedule
    if (!LittleFS.begin())
    {
        Serial.println("⚠️ LittleFS mount failed, air schedule disabled");
        airSchedule.enabled = false;
    }
    else
    {
        if (!loadAirScheduleFromFS("/schedule.json", airSchedule))
        {
            Serial.println("⚠️ load schedule failed, air schedule disabled");
            airSchedule.enabled = false;
        }
    }

    // 2. โหมดเริ่มต้น
    state.setMode(SystemMode::IDLE);

    // 3. สร้าง Tasks
    xTaskCreatePinnedToCore(inputTask, "In", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(controlTask, "Ctrl", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(commandTask, "Cmd", 4096, NULL, 1, NULL, 1);

    Serial.println("🚀 SmartFarm System: Ready and Linked.");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_INTERVAL_MS));
}