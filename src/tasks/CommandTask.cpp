#include <Arduino.h>
#include "Config.h"
#include "infrastructure/SharedState.h"
#include "interfaces/Types.h"

// ตัวแปรจริง ถูกสร้างไว้ใน main.cpp
extern SharedState state;

// กัน spam command
static uint32_t lastCommandMs = 0;
static const uint32_t CMD_COOLDOWN_MS = 200; // ms

// helper: อัปเดต ManualOverrides ที่ SharedState
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

    m.isUpdated = true; // เผื่ออนาคตถ้าอยากใช้ flag นี้
    state.setManualOverrides(m);
}

void commandTask(void *pvParameters)
{
    Serial.println("ℹ️ Command Processor: Active");

    while (true)
    {
        if (Serial.available() > 0)
        {
            // อ่านทั้งบรรทัดจาก Serial
            String input = Serial.readStringUntil('\n');
            input.trim();

            if (input.length() == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
                continue;
            }

            // กันกดรัวเกินไป
            uint32_t now = millis();
            if (now - lastCommandMs < CMD_COOLDOWN_MS)
            {
                Serial.println("⏱ CMD ignored: too fast");
                vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
                continue;
            }
            lastCommandMs = now;

            input.toLowerCase();

            // อ่าน snapshot เพื่อดู mode ปัจจุบัน
            SystemStatus snap = state.getSnapshot();

            // ================== เปลี่ยนโหมด ==================

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
                    // เข้า MANUAL เคลียร์ manual overrides ก่อน
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
                    // IDLE = สมองบังคับปิดหมด
                    state.setMode(SystemMode::IDLE);

                    // เคลียร์ manual overrides ทิ้ง
                    ManualOverrides m{};
                    m.isUpdated = true;
                    state.setManualOverrides(m);

                    Serial.println("✅ MODE -> IDLE (ALL OFF, no control)");
                }
            }

            // ================== CLEAR (ปิดทุกอย่าง แต่ไม่เปลี่ยนโหมด) ==================

            else if (input == "-clear")
            {
                ManualOverrides m{};
                m.isUpdated = true;
                state.setManualOverrides(m);
                Serial.println("🧹 CLEAR: ALL MANUAL OVERRIDES OFF");
            }

            // ================== คำสั่ง MANUAL ต่ออุปกรณ์ ==================
            // ใช้ได้เฉพาะตอน mode = MANUAL

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