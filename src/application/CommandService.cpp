#include "application/CommandService.h"
#include "interfaces/Types.h"

CommandService::CommandService(SystemContext &ctx)
    : _ctx(ctx)
{
}

void CommandService::printHelp() const
{
   Serial.println("=== SmartFarm CLI ===");

   Serial.println("\nMode:");
   Serial.println("  sm mode auto");
   Serial.println("  sm mode manual");
   Serial.println("  sm mode idle");

   Serial.println("\nRelay (MANUAL mode only):");
   Serial.println("  sm relay pump on/off");
   Serial.println("  sm relay mist on/off");
   Serial.println("  sm relay air  on/off");

   Serial.println("\nNetwork:");
   Serial.println("  sm net on");
   Serial.println("  sm net off");
   Serial.println("  sm net ntp");

   Serial.println("\nClock:");
   Serial.println("  sm clock set HH:MM[:SS]");

   Serial.println("\nOther:");
   Serial.println("  sm status");
   Serial.println("  sm clear");
   Serial.println("  sm help");
}

void CommandService::printStatus() const
{
   const SystemStatus snap = _ctx.state->getSnapshot();

   Serial.println("=== SmartFarm Status ===");

   Serial.printf("mode  : %d\n", (int)snap.mode);
   Serial.printf("pump  : %s\n", snap.isPumpActive ? "ON" : "OFF");
   Serial.printf("mist  : %s\n", snap.isMistActive ? "ON" : "OFF");
   Serial.printf("air   : %s\n", snap.isAirPumpActive ? "ON" : "OFF");

   Serial.printf("lux   : %.1f (%s)\n", snap.light.value, snap.light.isValid ? "valid" : "invalid");

   Serial.printf("temp  : %.2f (%s)\n", snap.temperature.value, snap.temperature.isValid ? "valid" : "invalid");
}

void CommandService::setModeAuto()
{
   _ctx.state->setMode(SystemMode::AUTO);
   Serial.println("[CMD] mode -> AUTO");
}

void CommandService::setModeManual()
{
   _ctx.state->setMode(SystemMode::MANUAL);
   Serial.println("[CMD] mode -> MANUAL");
}

void CommandService::setModeIdle()
{
   _ctx.state->setMode(SystemMode::IDLE);
   Serial.println("[CMD] mode -> IDLE");
}

void CommandService::clearManualOverrides()
{
   ManualOverrides m{};
   m.isUpdated = true;

   _ctx.state->setManualOverrides(m);

   Serial.println("[CMD] manual overrides cleared");
}

bool CommandService::isManualMode() const
{
   const SystemStatus snap = _ctx.state->getSnapshot();
   return snap.mode == SystemMode::MANUAL;
}

void CommandService::setManual(bool *pump, bool *mist, bool *air)
{
   ManualOverrides m = _ctx.state->getManualOverrides();

   if (pump)
      m.wantPumpOn = *pump;

   if (mist)
      m.wantMistOn = *mist;

   if (air)
      m.wantAirOn = *air;

   m.isUpdated = true;

   _ctx.state->setManualOverrides(m);
}

void CommandService::relayPump(bool on)
{
   if (!isManualMode())
   {
      Serial.println("[CMD] ignored: relay control allowed only in MANUAL mode");
      return;
   }

   setManual(&on, nullptr, nullptr);

   Serial.printf("[CMD] pump -> %s\n", on ? "ON" : "OFF");
}

void CommandService::relayMist(bool on)
{
   if (!isManualMode())
   {
      Serial.println("[CMD] ignored: relay control allowed only in MANUAL mode");
      return;
   }

   setManual(nullptr, &on, nullptr);

   Serial.printf("[CMD] mist -> %s\n", on ? "ON" : "OFF");
}

void CommandService::relayAir(bool on)
{
   if (!isManualMode())
   {
      Serial.println("[CMD] ignored: relay control allowed only in MANUAL mode");
      return;
   }

   setManual(nullptr, nullptr, &on);

   Serial.printf("[CMD] air -> %s\n", on ? "ON" : "OFF");
}

void CommandService::netOn()
{
   _ctx.state->requestNetOn();
   Serial.println("[CMD] net on requested");
}

void CommandService::netOff()
{
   _ctx.state->requestNetOff();
   Serial.println("[CMD] net off requested");
}

void CommandService::netNtp()
{
   _ctx.state->requestSyncNtp();
   Serial.println("[CMD] ntp sync requested");
}

bool CommandService::parseTimePayload(
    const String &payload,
    int &h,
    int &m,
    int &s) const
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

void CommandService::clockSet(const String &payload)
{
   int h = 0, m = 0, s = 0;

   if (!parseTimePayload(payload, h, m, s))
   {
      Serial.println("[CMD] invalid time format (use HH:MM or HH:MM:SS)");
      return;
   }

   if (h < 0 || h > 23 ||
       m < 0 || m > 59 ||
       s < 0 || s > 59)
   {
      Serial.println("[CMD] time value out of range");
      return;
   }

   _ctx.state->requestSetClockTime(
       static_cast<uint8_t>(h),
       static_cast<uint8_t>(m),
       static_cast<uint8_t>(s));

   Serial.printf("[CMD] time set requested: %02d:%02d:%02d\n", h, m, s);
}