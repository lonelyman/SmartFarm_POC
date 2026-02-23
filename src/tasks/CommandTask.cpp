#include <Arduino.h>
#include "Config.h"
#include "infrastructure/SharedState.h"
#include "interfaces/Types.h"
#include "drivers/Esp32Relay.h"

// ======================================================
//  EXTERN GLOBAL OBJECTS (สร้างตัวจริงใน main.cpp)
// ======================================================
extern SharedState state;
extern Esp32Relay waterPump;
extern Esp32Relay mistSystem;
extern Esp32Relay airPump;

// ======================================================
//  COMMAND RATE LIMIT (กันกดรัว / กัน noise จาก serial)
// ======================================================
static uint32_t lastCommandMs = 0;
static const uint32_t CMD_COOLDOWN_MS = 200; // ms

// ======================================================
//  HELPER: sync สถานะรีเลย์กลับเข้า SharedState
// ======================================================
static void syncActuatorsToState()
{
    state.updateActuators(
        waterPump.isOn(),
        mistSystem.isOn(),
        airPump.isOn());
}

// ======================================================
//  HELPER: ใช้เฉพาะคำสั่งที่ต้องอยู่ในโหมด MANUAL เท่านั้น
// ======================================================
static bool ensureManualMode(const char *cmd, const SystemStatus &snap)
{
    if (snap.mode != SystemMode::MANUAL)
    {
        Serial.printf("⚠️ '%s' ใช้ได้เฉพาะโหมด MANUAL\n", cmd);
        return false;
    }
    return true;
}

// ======================================================
//  MAIN TASK: commandTask
//  - อ่านคำสั่งจาก Serial (แบบบรรทัด เช่น "-auto", "-mist on")
//  - แบ่งหมวดชัดเจน: เปลี่ยนโหมด / เคลียร์ / สั่งอุปกรณ์
// ======================================================
void commandTask(void *pvParameters)
{
    Serial.println("ℹ️ Command Processor: Active");

    while (true)
    {
        if (Serial.available() > 0)
        {
            // ----------------------------------------------
            // 1) อ่านทั้งบรรทัดจาก Serial
            // ----------------------------------------------
            String input = Serial.readStringUntil('\n');
            input.trim();

            // ถ้ากดแค่ Enter เปล่า ๆ ก็ข้ามไป
            if (input.length() == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            // ----------------------------------------------
            // 2) กัน spam command ถี่เกินไป
            // ----------------------------------------------
            uint32_t now = millis();
            if (now - lastCommandMs < CMD_COOLDOWN_MS)
            {
                Serial.println("⏱ CMD ignored: too fast");
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
            lastCommandMs = now;

            // ทำเป็นตัวพิมพ์เล็กทั้งหมด (จะได้ไม่สนใจ case)
            input.toLowerCase();

            // snapshot ปัจจุบัน (ใช้เช็ค mode ฯลฯ)
            SystemStatus snap = state.getSnapshot();

            // ==================================================
            //  A) คำสั่งเปลี่ยนโหมด: -auto / -manual / -idle
            // ==================================================
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
                    state.setMode(SystemMode::MANUAL);

                    // เข้า MANUAL เคลียร์รีเลย์ก่อน (เริ่มจากพื้นสะอาด)
                    waterPump.turnOff();
                    mistSystem.turnOff();
                    airPump.turnOff();
                    syncActuatorsToState();

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

                    // IDLE = ทุกอย่างดับ และต้องดับตลอด
                    waterPump.turnOff();
                    mistSystem.turnOff();
                    airPump.turnOff();
                    syncActuatorsToState();

                    Serial.println("✅ MODE -> IDLE (ALL OFF, no control)");
                }
            }

            // ==================================================
            //  B) คำสั่งเคลียร์ทั้งหมด (STOP outputs แต่ไม่เปลี่ยน mode)
            //      -clear
            // ==================================================
            else if (input == "-clear")
            {
                waterPump.turnOff();
                mistSystem.turnOff();
                airPump.turnOff();
                syncActuatorsToState();
                Serial.println("🧹 CLEAR: ALL RELAYS OFF (mode unchanged)");
            }

            // ==================================================
            //  C) คำสั่ง MANUAL: สั่งรีเลย์รายตัว
            //      -mist on/off
            //      -pump on/off
            //      -air  on/off
            //      (ใช้ได้เฉพาะตอน mode = MANUAL)
            // ==================================================

            // ---------- หมอก ----------
            else if (input == "-mist on")
            {
                if (!ensureManualMode("-mist on", snap))
                {
                    // ไม่ใช่โหมด MANUAL → ข้าม
                }
                else
                {
                    mistSystem.turnOn();
                    syncActuatorsToState();
                    Serial.println("✅ MIST -> ON (manual)");
                }
            }
            else if (input == "-mist off")
            {
                if (!ensureManualMode("-mist off", snap))
                {
                    // ไม่ใช่โหมด MANUAL → ข้าม
                }
                else
                {
                    mistSystem.turnOff();
                    syncActuatorsToState();
                    Serial.println("✅ MIST -> OFF (manual)");
                }
            }

            // ---------- ปั๊มน้ำ ----------
            else if (input == "-pump on")
            {
                if (!ensureManualMode("-pump on", snap))
                {
                    // ไม่ใช่โหมด MANUAL → ข้าม
                }
                else
                {
                    waterPump.turnOn();
                    syncActuatorsToState();
                    Serial.println("✅ PUMP -> ON (manual)");
                }
            }
            else if (input == "-pump off")
            {
                if (!ensureManualMode("-pump off", snap))
                {
                    // ไม่ใช่โหมด MANUAL → ข้าม
                }
                else
                {
                    waterPump.turnOff();
                    syncActuatorsToState();
                    Serial.println("✅ PUMP -> OFF (manual)");
                }
            }

            // ---------- ปั๊มลม ----------
            else if (input == "-air on")
            {
                if (!ensureManualMode("-air on", snap))
                {
                    // ไม่ใช่โหมด MANUAL → ข้าม
                }
                else
                {
                    airPump.turnOn();
                    syncActuatorsToState();
                    Serial.println("✅ AIR -> ON (manual)");
                }
            }
            else if (input == "-air off")
            {
                if (!ensureManualMode("-air off", snap))
                {
                    // ไม่ใช่โหมด MANUAL → ข้าม
                }
                else
                {
                    airPump.turnOff();
                    syncActuatorsToState();
                    Serial.println("✅ AIR -> OFF (manual)");
                }
            }

            // ==================================================
            //  D) ไม่รู้จักคำสั่ง
            // ==================================================
            else
            {
                Serial.print("⚠️ Unknown Command: ");
                Serial.println(input);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}