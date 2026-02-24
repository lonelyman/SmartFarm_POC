#include <Arduino.h>
#include <LittleFS.h>

#include "Config.h"
#include "infrastructure/SharedState.h"
#include "infrastructure/ScheduleStore.h"

#include "application/FarmManager.h"

#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/Esp32Relay.h"
#include "drivers/RtcDs3231Time.h"
#include "domain/AirPumpSchedule.h"

// --- Global Objects ---
SharedState state;
AirPumpSchedule airSchedule;

// เซนเซอร์
Esp32Bh1750Light lightSensor("Main-Light");
Esp32FakeTemperature tempSensor("Main-Temp", 30.0f);

// รีเลย์
Esp32Relay waterPump(PIN_RELAY_WATER_PUMP, "Water-Pump");
Esp32Relay mistSystem(PIN_RELAY_MIST, "Mist-System");
Esp32Relay airPump(PIN_RELAY_AIR_PUMP, "Air-Pump");

// เวลา
RtcDs3231Time rtcTime;

// สมองกลาง (ผูก schedule จาก JSON ผ่าน pointer)
FarmManager manager(waterPump, mistSystem, airPump, &airSchedule);

// --- Task Forward Declarations ---
void inputTask(void *pvParameters);
void controlTask(void *pvParameters);
void commandTask(void *pvParameters);

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(100);

    Serial.println();
    Serial.println("===== SmartFarm Booting... =====");

    // 0) Filesystem + schedule
    if (!LittleFS.begin())
    {
        Serial.println("⚠️ LittleFS mount failed, air schedule disabled");
        airSchedule.enabled = false;
        airSchedule.windowCount = 0;
    }
    else
    {
        if (!loadAirScheduleFromFS("/schedule.json", airSchedule))
        {
            Serial.println("⚠️ load schedule failed, use no schedule");
            airSchedule.enabled = false;
            airSchedule.windowCount = 0;
        }
    }

    // 1) ตั้งค่า input ของสวิตช์โหมด
    pinMode(PIN_SW_MODE_A, INPUT_PULLUP);
    pinMode(PIN_SW_MODE_B, INPUT_PULLUP);

    // 2) ตื่นตัวฮาร์ดแวร์ (drivers)
    lightSensor.begin();
    tempSensor.begin();
    waterPump.begin();
    mistSystem.begin();
    airPump.begin();

#ifndef USE_FAKE_TIME
    rtcTime.begin();
#endif

    // 3) โหมดเริ่มต้น → IDLE (ทุกอย่างดับ, ปลอดภัย)
    state.setMode(SystemMode::IDLE);

    // 4) สร้าง Tasks
    xTaskCreatePinnedToCore(inputTask, "In", INPUT_TASK_STACK, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(controlTask, "Ctrl", CONTROL_TASK_STACK, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(commandTask, "Cmd", COMMAND_TASK_STACK, NULL, 1, NULL, 1);

    Serial.println("🚀 SmartFarm System: Ready and Linked.");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(LOOP_IDLE_MS));
}