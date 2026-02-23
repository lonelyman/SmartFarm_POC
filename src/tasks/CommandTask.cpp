#include <Arduino.h>
#include "infrastructure/SharedState.h"
#include "interfaces/Types.h"
#include "drivers/Esp32Relay.h"

// ตัวแปรจริง ถูกสร้างไว้ใน main.cpp
extern SharedState state;
extern Esp32Relay waterPump;
extern Esp32Relay mistSystem;
extern Esp32Relay airPump;

// กัน spam command
static uint32_t lastCommandMs = 0;
static const uint32_t CMD_COOLDOWN_MS = 200; // ms

// helper: เคลียร์ manual overrides เป็น OFF ทั้งหมด
static void clearManualOverrides()
{
    ManualOverrides m;
    m.wantPumpOn = false;
    m.wantMistOn = false;
    m.wantAirOn = false;
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
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            uint32_t now = millis();
            if (now - lastCommandMs < CMD_COOLDOWN_MS)
            {
                Serial.println("⏱ CMD ignored: too fast");
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
            lastCommandMs = now;

            input.toLowerCase();

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
                    clearManualOverrides(); // เคลียร์คำสั่ง manual เก่า
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
                    state.setMode(SystemMode::MANUAL);

                    // เข้า MANUAL เคลียร์รีเลย์ + overrides ก่อน (เริ่มจากพื้นสะอาด)
                    clearManualOverrides();
                    waterPump.turnOff();
                    mistSystem.turnOff();
                    airPump.turnOff();
                    state.updateActuators(false, false, false);

                    Serial.println("✅ MODE -> MANUAL (ALL OFF, waiting manual commands)");
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

                    clearManualOverrides();
                    waterPump.turnOff();
                    mistSystem.turnOff();
                    airPump.turnOff();
                    state.updateActuators(false, false, false);

                    Serial.println("✅ MODE -> IDLE (ALL OFF, no control)");
                }
            }

            // ================== คำสั่งเคลียร์ทั้งหมด ==================

            else if (input == "-clear")
            {
                clearManualOverrides();
                waterPump.turnOff();
                mistSystem.turnOff();
                airPump.turnOff();
                state.updateActuators(false, false, false);
                Serial.println("🧹 CLEAR: ALL RELAYS OFF (mode unchanged)");
            }

            // ================== คำสั่ง MANUAL ต่ออุปกรณ์ ==================
            // รูปแบบ:
            //   -mist on  / -mist off
            //   -pump on  / -pump off
            //   -air on   / -air off

            else if (input == "-mist on" || input == "-mist off" ||
                     input == "-pump on" || input == "-pump off" ||
                     input == "-air on" || input == "-air off")
            {
                if (snap.mode != SystemMode::MANUAL)
                {
                    Serial.println("⚠️ manual relay commands work only in MANUAL mode");
                }
                else
                {
                    ManualOverrides m = state.getManualOverrides();

                    if (input.startsWith("-mist "))
                    {
                        bool on = input.endsWith("on");
                        m.wantMistOn = on;
                        Serial.printf("✅ MIST -> %s (override)\n", on ? "ON" : "OFF");
                    }
                    else if (input.startsWith("-pump "))
                    {
                        bool on = input.endsWith("on");
                        m.wantPumpOn = on;
                        Serial.printf("✅ PUMP -> %s (override)\n", on ? "ON" : "OFF");
                    }
                    else if (input.startsWith("-air "))
                    {
                        bool on = input.endsWith("on");
                        m.wantAirOn = on;
                        Serial.printf("✅ AIR -> %s (override)\n", on ? "ON" : "OFF");
                    }

                    state.setManualOverrides(m);
                }
            }

            // ================== ไม่รู้จักคำสั่ง ==================
            else
            {
                Serial.print("⚠️ Unknown Command: ");
                Serial.println(input);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}