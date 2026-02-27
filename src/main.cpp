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

#include <WiFi.h>
#include "time.h"

// ปรับตาม Wi-Fi คุณ
const char *WIFI_SSID = "S21 Ultra ของ นิพน";
const char *WIFI_PASSWORD = "ppvz7847";

// ใช้ NTP server ทั่วไป
const char *NTP_SERVER = "pool.ntp.org";
// เวลาไทย = UTC+7
const long GMT_OFFSET_SEC = 7 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0; // ไม่มี DST

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

static void connectWifiIfNeeded()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println("[NET] Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint8_t retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20)
    {
        delay(500);
        Serial.print(".");
        retry++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("[NET] WiFi connected, IP=");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("[NET] WiFi connect FAILED");
    }
}
static void syncRtcFromNtp()
{
    connectWifiIfNeeded();
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[NTP] Cannot sync: WiFi not connected");
        return;
    }

    // ตั้งค่า NTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("[NTP] Failed to obtain time");
        return;
    }

    uint8_t h = timeinfo.tm_hour;
    uint8_t m = timeinfo.tm_min;
    uint8_t s = timeinfo.tm_sec;

    Serial.printf("[NTP] Time from net = %02u:%02u:%02u\n", h, m, s);

    if (!rtcTime.isOk())
    {
        Serial.println("[NTP] RTC not ready, cannot set");
        return;
    }

    if (rtcTime.setTimeOfDay(h, m, s))
    {
        Serial.println("[NTP] RTC synced from internet");
    }
    else
    {
        Serial.println("[NTP] Failed to set RTC");
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(100);

    Serial.println();
    Serial.println("===== SmartFarm Booting... =====");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    // 0) Filesystem + schedule
    if (!LittleFS.begin())
    {
        Serial.println("⚠️ LittleFS mount failed, air schedule disabled");
        airSchedule.enabled = false;
        airSchedule.windowCount = 0;
    }
    else
    {
        Serial.println(" LittleFS mount success, loading air schedule...");
        if (!loadAirScheduleFromFS("/schedule.json", airSchedule))
        {
            Serial.println("⚠️ load schedule failed, use no schedule");
            airSchedule.enabled = false;
            airSchedule.windowCount = 0;
        }
    }

    // 1) ตั้งค่า input ของสวิตช์โหมด
    pinMode(PIN_SW_MODE_A, INPUT);
    pinMode(PIN_SW_MODE_B, INPUT);

    // 2) ตื่นตัวฮาร์ดแวร์ (drivers)
    lightSensor.begin();
    tempSensor.begin();
    waterPump.begin();
    mistSystem.begin();
    airPump.begin();

    // 3) เริ่ม RTC DS3231 (ไม่ต้อง ifdef แล้ว ใช้จริงตลอด)
#if DEBUG_TIME_LOG
    Serial.println("[MAIN] rtcTime.begin() ...");
#endif
    if (!rtcTime.begin())
    {
#if DEBUG_TIME_LOG
        Serial.println("⚠️ MAIN: rtcTime.begin() failed");
#endif
    }
    syncRtcFromNtp();
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