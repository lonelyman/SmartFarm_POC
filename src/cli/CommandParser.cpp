#include "cli/CommandParser.h"
#include "cli/CommandTable.h"
#include "Config.h"

// ============================================================
// Command Handlers
// ============================================================

static void cmdHelp(CommandService &svc, int argc, String argv[])
{
   (void)argc;
   (void)argv;
   svc.printHelp();
}

static void cmdStatus(CommandService &svc, int argc, String argv[])
{
   (void)argc;
   (void)argv;
   svc.printStatus();
}

static void cmdClear(CommandService &svc, int argc, String argv[])
{
   (void)argc;
   (void)argv;
   svc.clearManualOverrides();
}

static void cmdMode(CommandService &svc, int argc, String argv[])
{
   if (argc < 2)
   {
      Serial.printf("usage: %s mode auto|manual|idle\n", CLI_PREFIX);
      return;
   }

   if (argv[1] == "auto")
   {
      svc.setModeAuto();
   }
   else if (argv[1] == "manual")
   {
      svc.setModeManual();
   }
   else if (argv[1] == "idle")
   {
      svc.setModeIdle();
   }
   else
   {
      Serial.println("[CLI] unknown mode");
   }
}

static void cmdRelay(CommandService &svc, int argc, String argv[])
{
   if (argc < 3)
   {
      Serial.printf("usage: %s relay pump|mist|air on|off\n", CLI_PREFIX);
      return;
   }

   const bool on = (argv[2] == "on");
   const bool off = (argv[2] == "off");

   if (!on && !off)
   {
      Serial.println("[CLI] relay action must be on/off");
      return;
   }

   if (argv[1] == "pump")
   {
      svc.relayPump(on);
   }
   else if (argv[1] == "mist")
   {
      svc.relayMist(on);
   }
   else if (argv[1] == "air")
   {
      svc.relayAir(on);
   }
   else
   {
      Serial.println("[CLI] unknown relay");
   }
}

static void cmdNet(CommandService &svc, int argc, String argv[])
{
   if (argc < 2)
   {
      Serial.printf("usage: %s net on|off|ntp\n", CLI_PREFIX);
      return;
   }

   if (argv[1] == "on")
   {
      svc.netOn();
   }
   else if (argv[1] == "off")
   {
      svc.netOff();
   }
   else if (argv[1] == "ntp")
   {
      svc.netNtp();
   }
   else
   {
      Serial.println("[CLI] unknown net command");
   }
}

static void cmdClock(CommandService &svc, int argc, String argv[])
{
   if (argc < 3)
   {
      Serial.printf("usage: %s clock set HH:MM[:SS]\n", CLI_PREFIX);
      return;
   }

   if (argv[1] != "set")
   {
      Serial.println("[CLI] clock action must be 'set'");
      return;
   }

   svc.clockSet(argv[2]);
}

// ============================================================
// Command Table
// ============================================================

static CliCommand commandTable[] =
    {
        {"help", cmdHelp},
        {"status", cmdStatus},
        {"clear", cmdClear},
        {"mode", cmdMode},
        {"relay", cmdRelay},
        {"net", cmdNet},
        {"clock", cmdClock},
};

static const int commandCount =
    sizeof(commandTable) / sizeof(commandTable[0]);

// ============================================================
// CommandParser
// ============================================================

int CommandParser::tokenize(const String &input, String tokens[], int maxTokens)
{
   int count = 0;
   int start = 0;
   const int len = input.length();

   while (start < len && count < maxTokens)
   {
      while (start < len && input[start] == ' ')
      {
         start++;
      }

      if (start >= len)
      {
         break;
      }

      int end = start;
      while (end < len && input[end] != ' ')
      {
         end++;
      }

      tokens[count++] = input.substring(start, end);
      start = end + 1;
   }

   return count;
}

void CommandParser::parse(CommandService &svc, const String &line)
{
   String input = line;
   input.trim();
   input.toLowerCase();

   String tokens[6];
   const int argc = tokenize(input, tokens, 6);

   if (argc == 0)
   {
      return;
   }

   if (tokens[0] != CLI_PREFIX)
   {
      Serial.printf("[CLI] ignored: command must start with '%s'\n", CLI_PREFIX);
      return;
   }

   if (argc < 2)
   {
      Serial.println("[CLI] missing command");
      return;
   }

   const String cmd = tokens[1];

   for (int i = 0; i < commandCount; i++)
   {
      if (cmd == commandTable[i].name)
      {
         // ส่งต่อแบบตัด prefix ออก
         // เช่น "sm mode auto"
         // argv[0] = "mode", argv[1] = "auto"
         commandTable[i].handler(svc, argc - 1, &tokens[1]);
         return;
      }
   }

   Serial.println("[CLI] unknown command");
}