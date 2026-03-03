#include <Arduino.h>
#include <unity.h>

#include "application/FarmManager.h"
#include "domain/AirPumpSchedule.h"

// helper to build input
static FarmInput makeInput(SystemMode mode, float temp, bool tempValid, uint16_t minOfDay)
{
   FarmInput in{};
   in.mode = mode;
   in.temperatureC = temp;
   in.temperatureValid = tempValid;
   in.minutesOfDay = minOfDay;
   return in;
}

void test_idle_resets_actuators(void)
{
   AirPumpSchedule sched;
   sched.enabled = true;
   FarmManager mgr(&sched);

   FarmInput in = makeInput(SystemMode::IDLE, 100, true, 0);
   FarmDecision dec = mgr.update(in);
   TEST_ASSERT_FALSE(dec.pumpOn);
   TEST_ASSERT_FALSE(dec.mistOn);
   TEST_ASSERT_FALSE(dec.airOn);
}

void test_auto_mist_hysteresis(void)
{
   AirPumpSchedule sched;
   sched.enabled = false; // schedule doesn't matter here
   FarmManager mgr(&sched);

   // start below OFF threshold
   FarmDecision dec1 = mgr.update(makeInput(SystemMode::AUTO, 28.0f, true, 0));
   TEST_ASSERT_FALSE(dec1.mistOn);

   // cross ON threshold
   FarmDecision dec2 = mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0));
   TEST_ASSERT_TRUE(dec2.mistOn);

   // drop to in-between; should latch previous (true)
   FarmDecision dec3 = mgr.update(makeInput(SystemMode::AUTO, 30.0f, true, 0));
   TEST_ASSERT_TRUE(dec3.mistOn);

   // drop below OFF threshold; should turn off
   FarmDecision dec4 = mgr.update(makeInput(SystemMode::AUTO, 28.0f, true, 0));
   TEST_ASSERT_FALSE(dec4.mistOn);
}

void test_auto_air_schedule(void)
{
   AirPumpSchedule sched;
   sched.enabled = true;
   sched.windowCount = 1;
   sched.windows[0].startMin = 60;
   sched.windows[0].endMin = 120;

   FarmManager mgr(&sched);

   FarmDecision d1 = mgr.update(makeInput(SystemMode::AUTO, 0, false, 30));
   TEST_ASSERT_FALSE(d1.airOn);
   FarmDecision d2 = mgr.update(makeInput(SystemMode::AUTO, 0, false, 60));
   TEST_ASSERT_TRUE(d2.airOn);
   FarmDecision d3 = mgr.update(makeInput(SystemMode::AUTO, 0, false, 119));
   TEST_ASSERT_TRUE(d3.airOn);
   FarmDecision d4 = mgr.update(makeInput(SystemMode::AUTO, 0, false, 120));
   TEST_ASSERT_FALSE(d4.airOn);
}

void setup()
{
   delay(2000); // allow serial to settle
   UNITY_BEGIN();
   RUN_TEST(test_idle_resets_actuators);
   RUN_TEST(test_auto_mist_hysteresis);
   RUN_TEST(test_auto_air_schedule);
   UNITY_END();
}

void loop() {}
