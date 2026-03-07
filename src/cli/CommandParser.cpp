#include "cli/CommandParser.h"
#include "cli/CommandTable.h"
#include "Config.h"

// ============================================================
// Forward declarations
// ============================================================

static void cmdHelp(CommandService &svc, int argc, String argv[]);
static void cmdStatus(CommandService &svc, int argc, String argv[]);
static void cmdClear(CommandService &svc, int argc, String argv[]);
static void cmdMode(CommandService &svc, int argc, String argv[]);
static void cmdRelay(CommandService &svc, int argc, String argv[]);
static void cmdNet(CommandService &svc, int argc, String argv[]);
static void cmdClock(CommandService &svc, int argc, String argv[]);

// ============================================================
// Command Table
// ============================================================

static CliCommand commandTable[] =
    {
        {
            "help",
            cmdHelp,
            "help",
            "show command list",
        },
        {
            "status",
            cmdStatus,
            "status",
            "show system status",
        },
        {
            "clear",
            cmdClear,
            "clear",
            "clear manual overrides",
        },
        {
            "mode",
            cmdMode,
            "mode auto|manual|idle",
            "change system mode",
        },
        {
            "relay",
            cmdRelay,
            "relay pump|mist|air on|off",
            "control relay in MANUAL mode",
        },
        {
            "net",
            cmdNet,
            "net on|off|ntp",
            "network control",
        },
        {
            "clock",
            cmdClock,
            "clock set HH:MM[:SS]",
            "set system clock",
        },
};

static const int commandCount =
    sizeof(commandTable) / sizeof(commandTable[0]);

// ============================================================
// Command Handlers
// ============================================================

static void cmdHelp(CommandService &svc, int argc, String argv[])
{
   (void)svc;
   (void)argc;
   (void)argv;

   Serial.println("=== SmartFarm CLI ===");
   Serial.println();

   for (int i = 0; i < commandCount; i++)
   {
      Serial.printf("  %s %-28s %s\n",
                    CLI_PREFIX,
                    commandTable[i].usage,
                    commandTable[i].description);
   }

   Serial.println();
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
}

static void cmdRelay(CommandService &svc, int argc, String argv[])
{
   if (argc < 3)
   {
      return;
   }

   const bool on = (argv[2] == "on");
   const bool off = (argv[2] == "off");

   if (!on && !off)
   {
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
}

static void cmdNet(CommandService &svc, int argc, String argv[])
{
   if (argc < 2)
   {
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
}

static void cmdClock(CommandService &svc, int argc, String argv[])
{
   if (argc < 3)
   {
      return;
   }

   if (argv[1] != "set")
   {
      return;
   }

   svc.clockSet(argv[2]);
}

// ============================================================
// Tokenizer
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

// ============================================================
// Parser
// ============================================================

CliParseResult CommandParser::parse(CommandService &svc, const String &line)
{
   String input = line;
   input.trim();
   input.toLowerCase();

   String tokens[6];

   const int argc = tokenize(input, tokens, 6);

   if (argc == 0)
   {
      return {CliParseCode::Empty, nullptr};
   }

   if (tokens[0] != CLI_PREFIX)
   {
      return {CliParseCode::MissingPrefix, "[CLI] command must start with CLI prefix"};
   }

   if (argc < 2)
   {
      return {CliParseCode::MissingCommand, "[CLI] missing command"};
   }

   const String cmd = tokens[1];

   for (int i = 0; i < commandCount; i++)
   {
      if (cmd == commandTable[i].name)
      {
         commandTable[i].handler(svc, argc - 1, &tokens[1]);
         return {CliParseCode::Ok, nullptr};
      }
   }

   return {CliParseCode::UnknownCommand, "[CLI] unknown command"};
}