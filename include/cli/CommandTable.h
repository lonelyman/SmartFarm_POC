#pragma once

#include <Arduino.h>
#include "application/CommandService.h"

typedef void (*CliHandler)(CommandService &svc, int argc, String argv[]);

struct CliCommand
{
   const char *name;
   CliHandler handler;

   const char *usage;
   const char *description;
};