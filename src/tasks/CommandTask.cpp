#include <Arduino.h>

#include "Config.h"
#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "interfaces/Types.h"

// cooldown กัน spam command รัวๆ
static uint32_t lastCommandMs = 0;
static const uint32_t CMD_COOLDOWN_MS = 200;

static void applyManualChange(SystemContext &ctx,
                              bool pumpOn, bool mistOn, bool airOn,
                              bool setPump, bool setMist, bool setAir)
{
    ManualOverrides m = ctx.state->getManualOverrides();

    // NOTE: ฟิลด์เหล่านี้ต้องมีใน Types.h ตามที่คุณใช้อยู่
    if (setPump)
        m.wantPumpOn = pumpOn;
    if (setMist)
        m.wantMistOn = mistOn;
    if (setAir)
        m.wantAirOn = airOn;

    // ถ้าคุณมี flag isUpdated ใช้อยู่ ให้คงไว้ (เพื่อให้ FarmManager/ControlTask รู้ว่ามีการเปลี่ยน)
    m.isUpdated = true;

    ctx.state->setManualOverrides(m);
}

static bool parseTimePayload(const String &payload, int &h, int &m, int &s)
{
    String p = payload;
    p.trim();

    int c1 = p.indexOf(':');
    if (c1 < 0)
        return false;

    int c2 = p.indexOf(':', c1 + 1);

    if (c2 < 0)
    {
        // HH:MM
        h = p.substring(0, c1).toInt();
        m = p.substring(c1 + 1).toInt();
        s = 0;
        return true;
    }

    // HH:MM:SS
    h = p.substring(0, c1).toInt();
    m = p.substring(c1 + 1, c2).toInt();
    s = p.substring(c2 + 1).toInt();
    return true;
}

static void requestTimeSet(SystemContext &ctx, const String &inputRaw)
{
    String cmd = inputRaw;
    cmd.trim();
    cmd.toLowerCase();

    int pos = cmd.indexOf("time=");
    if (pos < 0)
    {
        Serial.println("[CMD] invalid TIME format (need time=HH:MM or time=HH:MM:SS)");
        return;
    }

    String payload = cmd.substring(pos + 5);
    payload.trim();

    int h = 0, m = 0, s = 0;
    if (!parseTimePayload(payload, h, m, s))
    {
        Serial.println("[CMD] TIME format error (need HH:MM or HH:MM:SS)");
        return;
    }

    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59)
    {
        Serial.println("[CMD] TIME value out of range");
        return;
    }

    // ✅ ไม่แตะ RTC/Driver: ส่งคำขอผ่าน SharedState
    ctx.state->requestSetClockTime((uint8_t)h, (uint8_t)m, (uint8_t)s);

    Serial.printf("[CMD] time set requested: %02d:%02d:%02d\n", h, m, s);
}

static void printHelp()
{
    Serial.println("=== SmartFarm Commands ===");
    Serial.println("Modes:");
    Serial.println("  -auto | --a");
    Serial.println("  -manual | --m");
    Serial.println("  -idle | --i");
    Serial.println("Manual relay (MANUAL only):");
    Serial.println("  -pump on/off");
    Serial.println("  -mist on/off");
    Serial.println("  -air  on/off");
    Serial.println("Other:");
    Serial.println("  -clear              (reset manual overrides, keep mode)");
    Serial.println("  time=HH:MM[:SS]     (request set clock time)");
}

static String g_line;
static bool readLineNonBlocking(String &out)
{
    while (Serial.available() > 0)
    {
        char c = (char)Serial.read();

        // ignore CR
        if (c == '\r')
            continue;

        // newline -> commit
        if (c == '\n')
        {
            out = g_line;
            g_line = "";
            out.trim();
            return out.length() > 0;
        }

        // backspace / delete
        if (c == 8 || c == 127)
        {
            if (g_line.length() > 0)
                g_line.remove(g_line.length() - 1);
            continue;
        }

        // normal char (cap length)
        if (g_line.length() < 96)
            g_line += c;
    }
    return false;
}

void commandTask(void *pvParameters)
{
    auto *ctx = static_cast<SystemContext *>(pvParameters);
    if (ctx == nullptr || ctx->state == nullptr)
    {
        vTaskDelete(nullptr);
        return;
    }

    Serial.println("ℹ️ Command Processor: Active");
    printHelp();

    while (true)
    {
        String input;
        if (!readLineNonBlocking(input))
        {
            vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
            continue;
        }
        Serial.printf("\n> %s\n", input.c_str());

        uint32_t now = millis();
        if (now - lastCommandMs < CMD_COOLDOWN_MS)
        {
            Serial.println("⏱ CMD ignored: too fast");
            vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
            continue;
        }
        lastCommandMs = now;

        String lower = input;
        lower.toLowerCase();

        SystemStatus snap = ctx->state->getSnapshot();

        // ---- Help
        if (lower == "-help" || lower == "--help" || lower == "help")
        {
            printHelp();
        }
        // ---- Mode commands
        else if (lower == "-auto" || lower == "--a")
        {
            ctx->state->setMode(SystemMode::AUTO);
            Serial.println("[CMD] mode -> AUTO");
        }
        else if (lower == "-manual" || lower == "--m")
        {
            ctx->state->setMode(SystemMode::MANUAL);
            Serial.println("[CMD] mode -> MANUAL");
        }
        else if (lower == "-idle" || lower == "--i")
        {
            ctx->state->setMode(SystemMode::IDLE);
            Serial.println("[CMD] mode -> IDLE");
        }
        // ---- Clear overrides (does not change mode)
        else if (lower == "-clear")
        {
            ManualOverrides m{};
            m.isUpdated = true;
            ctx->state->setManualOverrides(m);
            Serial.println("[CMD] manual overrides cleared");
        }
        // ---- Manual relay control (MANUAL only)
        else if (lower == "-pump on")
        {
            if (snap.mode != SystemMode::MANUAL)
                Serial.println("[CMD] pump on ignored (not MANUAL)");
            else
            {
                applyManualChange(*ctx, true, false, false, true, false, false);
                Serial.println("[CMD] pump -> ON");
            }
        }
        else if (lower == "-pump off")
        {
            if (snap.mode != SystemMode::MANUAL)
                Serial.println("[CMD] pump off ignored (not MANUAL)");
            else
            {
                applyManualChange(*ctx, false, false, false, true, false, false);
                Serial.println("[CMD] pump -> OFF");
            }
        }
        else if (lower == "-mist on")
        {
            if (snap.mode != SystemMode::MANUAL)
                Serial.println("[CMD] mist on ignored (not MANUAL)");
            else
            {
                applyManualChange(*ctx, false, true, false, false, true, false);
                Serial.println("[CMD] mist -> ON");
            }
        }
        else if (lower == "-mist off")
        {
            if (snap.mode != SystemMode::MANUAL)
                Serial.println("[CMD] mist off ignored (not MANUAL)");
            else
            {
                applyManualChange(*ctx, false, false, false, false, true, false);
                Serial.println("[CMD] mist -> OFF");
            }
        }
        else if (lower == "-air on")
        {
            if (snap.mode != SystemMode::MANUAL)
                Serial.println("[CMD] air on ignored (not MANUAL)");
            else
            {
                applyManualChange(*ctx, false, false, true, false, false, true);
                Serial.println("[CMD] air -> ON");
            }
        }
        else if (lower == "-air off")
        {
            if (snap.mode != SystemMode::MANUAL)
                Serial.println("[CMD] air off ignored (not MANUAL)");
            else
            {
                applyManualChange(*ctx, false, false, false, false, false, true);
                Serial.println("[CMD] air -> OFF");
            }
        }
        else if (lower == "-net on")
        {
            ctx->state->requestNetOn();
            Serial.println("[CMD] net on requested");
            Serial.printf("CMD state=%p\n", ctx->state);
        }
        else if (lower == "-net off")
        {
            ctx->state->requestNetOff();
            Serial.println("[CMD] net off requested");
        }
        else if (lower == "-netuptime")
        {
            ctx->state->requestSyncNtp();
            Serial.println("[CMD] ntp sync requested");
        }

        // ---- Set time (request via SharedState)
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