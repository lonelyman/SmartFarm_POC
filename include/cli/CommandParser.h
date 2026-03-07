#pragma once

#include <Arduino.h>
#include "application/CommandService.h"

class CommandParser
{
public:
   void parse(CommandService &svc, const String &line);

private:
   int tokenize(const String &input, String tokens[], int maxTokens);
};