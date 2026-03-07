#include <Arduino.h>

#include "Config.h"
#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"

#include "application/CommandService.h"
#include "cli/CommandParser.h"

// ============================================================
// Serial Line Reader
// ============================================================

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

        if (lineBuffer.length() < 128)
            lineBuffer += c;
    }

    return false;
}

// ============================================================
// Globals
// ============================================================

static CommandParser g_cli;

// ============================================================
// CommandTask
// ============================================================

void commandTask(void *pvParameters)
{
    auto *ctx = static_cast<SystemContext *>(pvParameters);

    if (!ctx || !ctx->state)
    {
        vTaskDelete(nullptr);
        return;
    }

    CommandService svc(*ctx);

    Serial.println("ℹ️ CommandTask: Active");
    svc.printHelp();

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

        // กัน spam command
        const uint32_t now = millis();

        if (now - lastCommandMs < 200)
        {
            Serial.println("⏱ CMD ignored: too fast");
            vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
            continue;
        }

        lastCommandMs = now;

        // ส่ง command เข้า parser
        CliParseResult r = g_cli.parse(svc, input);

        if (r.code != CliParseCode::Ok && r.message)
        {
            Serial.println(r.message);
        }

        vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_INTERVAL_MS));
    }
}