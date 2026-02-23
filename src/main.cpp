#include <Arduino.h>
#include <LittleFS.h>
#include "Config.h"
#include "application/FarmManager.h"
#include "infrastructure/SharedState.h"
#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/RtcDs3231Time.h"
#include "drivers/Esp32Relay.h"
#include "domain/AirPumpSchedule.h"

// --- Global Objects (ตัวจริงถูกสร้างที่นี่) ---
SharedState state;
AirPumpSchedule airSchedule;
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

// สมองกลาง
FarmManager manager(waterPump, mistSystem, airPump);

// --- Task Forward Declarations ---
void inputTask(void *pvParameters);
void controlTask(void *pvParameters);
void commandTask(void *pvParameters);

// ==== Forward declarations (เดี๋ยวจะใช้ใน setup) ====
bool initFilesystem();
void loadScheduleFromJson();

// ==== init LittleFS ====
bool initFilesystem()
{
    if (!LittleFS.begin())
    {
        Serial.println("❌ LittleFS mount failed (schedule.json จะยังใช้ไม่ได้)");
        return false;
    }
    Serial.println("✅ LittleFS mounted (พร้อมอ่าน schedule.json)");
    return true;
}

// ==== โหลด schedule.json แบบง่าย ๆ (ยังไม่ผูกเข้า logic, แค่ prove ว่าอ่านได้) ====
void loadScheduleFromJson()
{
    const char *path = "/schedule.json";

    if (!LittleFS.exists(path))
    {
        Serial.println("⚠️ schedule.json not found in LittleFS");
        return;
    }

    File f = LittleFS.open(path, "r");
    if (!f)
    {
        Serial.println("⚠️ cannot open /schedule.json");
        return;
    }

    Serial.println("📂 schedule.json loaded, content:");
    while (f.available())
    {
        String line = f.readStringUntil('\n');
        Serial.println(line);
    }
    f.close();

    // ตอนนี้แค่ print ออกมาดูก่อน
    // ขั้นต่อไปค่อยทำ parser จริง → แปลงเป็น windows ใน FarmManager
}

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println("===== SmartFarm Booting... =====");

    // 0) เริ่ม Filesystem (สำหรับอ่าน schedule.json)
    bool fsOk = initFilesystem();
    if (fsOk)
    {
        loadScheduleFromJson();   // แค่ลองอ่าน/แสดงผลก่อน
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
    xTaskCreatePinnedToCore(inputTask,   "In",   4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(controlTask, "Ctrl", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(commandTask, "Cmd",  4096, NULL, 1, NULL, 1);

    Serial.println("🚀 SmartFarm System: Ready and Linked.");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}