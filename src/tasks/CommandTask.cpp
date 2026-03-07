// src/tasks/CommandTask.cpp
#include <Arduino.h>

#include "Config.h"
#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "interfaces/Types.h"

// ============================================================
//  Helpers
// ============================================================

static void printHelp()
{
    Serial.println("=== SmartFarm Commands ===");
    Serial.println("Mode:");
    Serial.println("  -auto  | --a");
    Serial.println("  -manual | --m");
    Serial.println("  -idle  | --i");
    Serial.println("Manual relay (MANUAL mode only):");
    Serial.println("  -pump on/off");
    Serial.println("  -mist on/off");
    Serial.println("  -air  on/off");
    Serial.println("  -clear           reset manual overrides");
    Serial.println("Network:");
    Serial.println("  -net on/off      connect / disconnect WiFi STA");
    Serial.println("  -ntp             sync RTC from NTP (STA required)");
    Serial.println("Clock:");
    Serial.println("  time=HH:MM[:SS]  set RTC time");
    Serial.println("  -help            show this help");
}

// อ่าน Serial ทีละตัวอักษร แบบ non-blocking
// คืน true เมื่อได้ครบ 1 บรรทัด (newline)
static bool readLineNonBlocking(String &lineBuffer, String &out)
{
    while (Serial.available() > 0)
    {
        const char c = static_cast<char>(Serial.read());

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            out = lineBuffer;
            lineBuffer = "";
            out.trim();
            return out.length() > 0;
        }

        // backspace / delete
        if (c == 8 || c == 127)
        {
            if (lineBuffer.length() > 0)
                lineBuffer.remove(lineBuffer.length() - 1);
            continue;
        }

        if (lineBuffer.length() < 96)
            lineBuffer += c;
    }
    return false;
}

static bool parseTimePayload(const String &payload, int &h, int &m, int &s)
{
    String p = payload;
    p.trim();

    const int c1 = p.indexOf(':');
    if (c1 < 0)
        return false;

    const int c2 = p.indexOf(':', c1 + 1);

    if (c2 < 0)
    {
        h = p.substring(0, c1).toInt();
        m = p.substring(c1 + 1).toInt();
        s = 0;
    }
    else
    {
        h = p.substring(0, c1).toInt();
        m = p.substring(c1 + 1, c2).toInt();
        s = p.substring(c2 + 1).toInt();
    }
    return true;
}

static void requestTimeSet(SystemContext &ctx, const String &inputRaw)
{
    String cmd = inputRaw;
    cmd.trim();
    cmd.toLowerCase();

    const int pos = cmd.indexOf("time=");
    if (pos < 0)
    {
        Serial.println("[CMD] invalid format (use time=HH:MM or time=HH:MM:SS)");
        return;
    }

    String payload = cmd.substring(pos + 5);
    payload.trim();

    int h = 0, m = 0, s = 0;
    if (!parseTimePayload(payload, h, m, s))
    {
        Serial.println("[CMD] time format error");
        return;
    }

    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59)
    {
        Serial.println("[CMD] time value out of range");
        return;
    }

    ctx.state->requestSetClockTime(
        static_cast<uint8_t>(h),
        static_cast<uint8_t>(m),
        static_cast<uint8_t>(s));

    Serial.printf("[CMD] time set requested: %02d:%02d:%02d\n", h, m, s);
}

// set manual override field เดียว — อ่าน state ปัจจุบันก่อนเสมอ (read-modify-write)
static void setManual(SystemContext &ctx,
                      bool *pump, bool *mist, bool *air)
{
    ManualOverrides m = ctx.state->getManualOverrides();
    if (pump)
        m.wantPumpOn = *pump;
    if (mist)
        m.wantMistOn = *mist;
    if (air)
        m.wantAirOn = *air;
    m.isUpdated = true;
    ctx.state->setManualOverrides(m);
}

// ============================================================
//  commandTask
// ============================================================

void commandTask(void *pvParameters)
{
    auto *ctx = static_cast<SystemContext *>(pvParameters);
    if (!ctx || !ctx->state)
    {
        vTaskDelete(nullptr);
        return;
    }

    Serial.println("ℹ️ CommandTask: Active");
    printHelp();

    String lineBuffer;
    uint32_t lastCommandMs = 0;

    while (true)
    {
        String input;
        if (!readLineNonBlocking(lineBuffer, input))
        {
            vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
            continue;
        }

        Serial.printf("\n> %s\n", input.c_str());

        // cooldown กัน spam
        const uint32_t now = millis();
        if (now - lastCommandMs < 200)
        {
            Serial.println("⏱ CMD ignored: too fast");
            vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
            continue;
        }
        lastCommandMs = now;

        String lower = input;
        lower.toLowerCase();

        const SystemStatus snap = ctx->state->getSnapshot();
        const bool isManual = snap.mode == SystemMode::MANUAL;

        // --- Help ---
        if (lower == "-help" || lower == "--help" || lower == "help")
        {
            printHelp();
        }
        // --- Mode ---
        else if (lower == "-auto" || lower == "--a")
        {
            ctx->state->setMode(SystemMode::AUTO);
            Serial.println("[CMD] mode → AUTO");
        }
        else if (lower == "-manual" || lower == "--m")
        {
            ctx->state->setMode(SystemMode::MANUAL);
            Serial.println("[CMD] mode → MANUAL");
        }
        else if (lower == "-idle" || lower == "--i")
        {
            ctx->state->setMode(SystemMode::IDLE);
            Serial.println("[CMD] mode → IDLE");
        }
        // --- Clear overrides ---
        else if (lower == "-clear")
        {
            ManualOverrides m{};
            m.isUpdated = true;
            ctx->state->setManualOverrides(m);
            Serial.println("[CMD] manual overrides cleared");
        }
        // --- Manual relay (MANUAL only) ---
        else if (lower == "-pump on" || lower == "-pump off")
        {
            if (!isManual)
            {
                Serial.println("[CMD] ignored: not MANUAL");
            }
            else
            {
                bool v = (lower == "-pump on");
                setManual(*ctx, &v, nullptr, nullptr);
                Serial.printf("[CMD] pump → %s\n", v ? "ON" : "OFF");
            }
        }
        else if (lower == "-mist on" || lower == "-mist off")
        {
            if (!isManual)
            {
                Serial.println("[CMD] ignored: not MANUAL");
            }
            else
            {
                bool v = (lower == "-mist on");
                setManual(*ctx, nullptr, &v, nullptr);
                Serial.printf("[CMD] mist → %s\n", v ? "ON" : "OFF");
            }
        }
        else if (lower == "-air on" || lower == "-air off")
        {
            if (!isManual)
            {
                Serial.println("[CMD] ignored: not MANUAL");
            }
            else
            {
                bool v = (lower == "-air on");
                setManual(*ctx, nullptr, nullptr, &v);
                Serial.printf("[CMD] air → %s\n", v ? "ON" : "OFF");
            }
        }
        // --- Network ---
        else if (lower == "-net on")
        {
            ctx->state->requestNetOn();
            Serial.println("[CMD] net on requested");
        }
        else if (lower == "-net off")
        {
            ctx->state->requestNetOff();
            Serial.println("[CMD] net off requested");
        }
        else if (lower == "-ntp")
        {
            ctx->state->requestSyncNtp();
            Serial.println("[CMD] NTP sync requested");
        }
        // --- Clock ---
        else if (lower.indexOf("time=") >= 0)
        {
            requestTimeSet(*ctx, input);
        }
        else
        {
            Serial.println("[CMD] unknown command (use -help)");
        }

        vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
    }
}