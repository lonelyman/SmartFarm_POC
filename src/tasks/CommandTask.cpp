#include <Arduino.h>
#include "Config.h"
#include "infrastructure/SharedState.h"
#include "interfaces/Types.h"
#include "drivers/RtcDs3231Time.h"

// ---- extern จาก main.cpp ----
extern SharedState state;
extern RtcDs3231Time rtcTime;

// กัน spam command
static uint32_t lastCommandMs = 0;
static const uint32_t CMD_COOLDOWN_MS = 200; // ms

// ----------------------------------------------------
// helper: ตั้งเวลา RTC จากคำสั่งที่มีคำว่า time=
// ตัวอย่างที่รับได้:
//   time=23:35
//   TIME=23:35
//   -time=23:35
//   set time=23:35
// ----------------------------------------------------
static void setRtcFromCommand(String cmd)
{
    cmd.trim();
    cmd.toLowerCase();

    Serial.printf("[SETTIME RAW] '%s'\n", cmd.c_str());

    int pos = cmd.indexOf("time=");
    if (pos < 0)
    {
        Serial.println("[CMD] invalid TIME format (need time=HH:MM or time=HH:MM:SS)");
        return;
    }

    // ตัดตั้งแต่หลังคำว่า "time=" ไปจนจบ
    String payload = cmd.substring(pos + 5);
    payload.trim(); // เผื่อมี space

    int firstColon = payload.indexOf(':');
    int secondColon = payload.indexOf(':', firstColon + 1);

    if (firstColon < 0)
    {
        Serial.println("[CMD] TIME format error, need HH:MM");
        return;
    }

    int h = 0, m = 0, s = 0;

    if (secondColon < 0)
    {
        // รูปแบบ HH:MM
        String sh = payload.substring(0, firstColon);
        String sm = payload.substring(firstColon + 1);
        h = sh.toInt();
        m = sm.toInt();
        s = 0;
    }
    else
    {
        // รูปแบบ HH:MM:SS
        String sh = payload.substring(0, firstColon);
        String sm = payload.substring(firstColon + 1, secondColon);
        String ss = payload.substring(secondColon + 1);
        h = sh.toInt();
        m = sm.toInt();
        s = ss.toInt();
    }

    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59)
    {
        Serial.println("[CMD] TIME value out of range");
        return;
    }

    if (!rtcTime.isOk())
    {
        Serial.println("[CMD] RTC not ready, cannot set time");
        return;
    }

    bool ok = rtcTime.setTimeOfDay((uint8_t)h, (uint8_t)m, (uint8_t)s);
    if (ok)
    {
        Serial.printf("[CMD] RTC time set to %02d:%02d:%02d\n", h, m, s);
    }
    else
    {
        Serial.println("[CMD] failed to set RTC time");
    }
}

// ----------------------------------------------------
// helper: อัปเดต ManualOverrides ใน SharedState
// ----------------------------------------------------
static void applyManualChange(bool pumpOn, bool mistOn, bool airOn,
                              bool setPump, bool setMist, bool setAir)
{
    ManualOverrides m = state.getManualOverrides();

    if (setPump)
        m.wantPumpOn = pumpOn;
    if (setMist)
        m.wantMistOn = mistOn;
    if (setAir)
        m.wantAirOn = airOn;

    m.isUpdated = true;
    state.setManualOverrides(m);
}

void commandTask(void *pvParameters)
{
    Serial.println("ℹ️ Command Processor: Active");

    while (true)
    {
        if (Serial.available() > 0)
        {
            String input = Serial.readStringUntil('\n');
            input.trim();

            if (input.length() == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
                continue;
            }

            uint32_t now = millis();
            if (now - lastCommandMs < CMD_COOLDOWN_MS)
            {
                Serial.println("⏱ CMD ignored: too fast");
                vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
                continue;
            }
            lastCommandMs = now;

            // debug raw
            Serial.printf("[CMD RAW] '%s' (len=%d)\n", input.c_str(), input.length());

            input.toLowerCase();

            SystemStatus snap = state.getSnapshot();

            // ---------- เปลี่ยนโหมด ----------
            if (input == "-auto" || input == "--a")
            {
                if (snap.mode == SystemMode::AUTO)
                {
                    Serial.println("ℹ️ MODE already AUTO (ignored)");
                }
                else
                {
                    state.setMode(SystemMode::AUTO);
                    Serial.println("✅ MODE -> AUTO");
                }
            }
            else if (input == "-manual" || input == "--m")
            {
                if (snap.mode == SystemMode::MANUAL)
                {
                    Serial.println("ℹ️ MODE already MANUAL (ignored)");
                }
                else
                {
                    ManualOverrides m{};
                    m.isUpdated = true;
                    state.setManualOverrides(m);

                    state.setMode(SystemMode::MANUAL);
                    Serial.println("✅ MODE -> MANUAL (ALL OFF, wait manual)");
                }
            }
            else if (input == "-idle" || input == "--i")
            {
                if (snap.mode == SystemMode::IDLE)
                {
                    Serial.println("ℹ️ MODE already IDLE (ignored)");
                }
                else
                {
                    state.setMode(SystemMode::IDLE);

                    ManualOverrides m{};
                    m.isUpdated = true;
                    state.setManualOverrides(m);

                    Serial.println("✅ MODE -> IDLE (ALL OFF, no control)");
                }
            }
            else if (input.indexOf("time=") >= 0)
            {
                // คำสั่งตั้งเวลา RTC
                setRtcFromCommand(input);
            }
            // ---------- CLEAR manual ----------
            else if (input == "-clear")
            {
                ManualOverrides m{};
                m.isUpdated = true;
                state.setManualOverrides(m);
                Serial.println("🧹 CLEAR: ALL MANUAL OVERRIDES OFF");
            }
            // ---------- manual control ----------
            else if (input == "-mist on")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ '-mist on' ใช้ได้เฉพาะโหมด MANUAL");
                }
                else
                {
                    applyManualChange(false, true, false, false, true, false);
                    Serial.println("✅ MIST -> ON (manual intent)");
                }
            }
            else if (input == "-mist off")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ '-mist off' ใช้ได้เฉพาะโหมด MANUAL");
                }
                else
                {
                    applyManualChange(false, false, false, false, true, false);
                    Serial.println("✅ MIST -> OFF (manual intent)");
                }
            }
            else if (input == "-pump on")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ '-pump on' ใช้ได้เฉพาะโหมด MANUAL");
                }
                else
                {
                    applyManualChange(true, false, false, true, false, false);
                    Serial.println("✅ PUMP -> ON (manual intent)");
                }
            }
            else if (input == "-pump off")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ '-pump off' ใช้ได้เฉพาะโหมด MANUAL");
                }
                else
                {
                    applyManualChange(false, false, false, true, false, false);
                    Serial.println("✅ PUMP -> OFF (manual intent)");
                }
            }
            else if (input == "-air on")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ '-air on' ใช้ได้เฉพาะโหมด MANUAL");
                }
                else
                {
                    applyManualChange(false, false, true, false, false, true);
                    Serial.println("✅ AIR -> ON (manual intent)");
                }
            }
            else if (input == "-air off")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ '-air off' ใช้ได้เฉพาะโหมด MANUAL");
                }
                else
                {
                    applyManualChange(false, false, false, false, false, true);
                    Serial.println("✅ AIR -> OFF (manual intent)");
                }
            }
            else
            {
                Serial.print("⚠️ Unknown Command: ");
                Serial.println(input);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
    }
}