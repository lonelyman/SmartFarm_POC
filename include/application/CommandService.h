#pragma once

#include <Arduino.h>
#include "infrastructure/SystemContext.h"

class CommandService
{
public:
   explicit CommandService(SystemContext &ctx);

   // help / status
   void printHelp() const;
   void printStatus() const;

   // mode
   void setModeAuto();
   void setModeManual();
   void setModeIdle();

   // manual control
   void clearManualOverrides();

   void relayPump(bool on);
   void relayMist(bool on);
   void relayAir(bool on);

   // network
   void netOn();
   void netOff();
   void netNtp();

   // clock
   void clockSet(const String &payload);

private:
   SystemContext &_ctx;

   bool isManualMode() const;

   void setManual(bool *pump, bool *mist, bool *air);

   bool parseTimePayload(
       const String &payload,
       int &h,
       int &m,
       int &s) const;
};