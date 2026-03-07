#pragma once

#include <Arduino.h>
#include "application/CommandService.h"

// ============================================================
// CLI Parse Result
// ============================================================

enum class CliParseCode
{
   Ok,
   Empty,
   MissingPrefix,
   MissingCommand,
   UnknownCommand,
   InvalidArgs
};

struct CliParseResult
{
   CliParseCode code;
   const char *message;
};

// ============================================================
// CommandParser
// ============================================================

class CommandParser
{
public:
   CliParseResult parse(CommandService &svc, const String &line);

private:
   int tokenize(const String &input, String tokens[], int maxTokens);
};