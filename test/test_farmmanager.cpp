#include <Arduino.h>
#include <unity.h>

#include "application/FarmManager.h"
#include "application/FarmModels.h"
#include "application/ScheduledRelay.h"
#include "domain/TimeSchedule.h"

// ============================================================
//  Helpers
// ============================================================

// Mock IActuator สำหรับทดสอบ ScheduledRelay โดยไม่ต้องมี hardware
class MockActuator : public IActuator
{
public:
   bool begin() override { return true; }
   void turnOn() override { _on = true; }
   void turnOff() override { _on = false; }
   bool isOn() const override { return _on; }
   const char *getName() const override { return "MockActuator"; }

private:
   bool _on = false;
};

static FarmInput makeInput(SystemMode mode, float temp, bool tempValid,
                           uint16_t minOfDay, float hum = 0.0f, bool humValid = false)
{
   FarmInput in{};
   in.mode = mode;
   in.temperatureC = temp;
   in.temperatureValid = tempValid;
   in.humidityRH = hum;
   in.humidityValid = humValid;
   in.minutesOfDay = minOfDay;
   return in;
}

// ============================================================
//  FarmManager tests
// ============================================================

void test_idle_resets_actuators(void)
{
   FarmManager mgr;

   // temp สูงมากให้ mist ติด
   mgr.update(makeInput(SystemMode::AUTO, 100.0f, true, 0));

   // IDLE ต้องปิดทุกอย่าง
   FarmDecision dec = mgr.update(makeInput(SystemMode::IDLE, 100.0f, true, 0));
   TEST_ASSERT_FALSE(dec.pumpOn);
   TEST_ASSERT_FALSE(dec.mistOn);
   // หมายเหตุ: airOn ไม่อยู่ใน FarmDecision แล้ว — ดูแลโดย ScheduledRelay
}

void test_auto_mist_temp_hysteresis(void)
{
   FarmManager mgr;

   // ต่ำกว่า OFF threshold (29°C) → off
   FarmDecision dec1 = mgr.update(makeInput(SystemMode::AUTO, 28.0f, true, 0));
   TEST_ASSERT_FALSE(dec1.mistOn);

   // ข้าม ON threshold (32°C) — ต้องครบ MIST_CONFIRM_ON_COUNT รอบ (=3) ถึงเปิด
   mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0));                     // รอบ 1
   mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0));                     // รอบ 2
   FarmDecision dec2 = mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0)); // รอบ 3 → on
   TEST_ASSERT_TRUE(dec2.mistOn);

   // dead-band (30°C ระหว่าง 29–32) → latch previous = true
   FarmDecision dec3 = mgr.update(makeInput(SystemMode::AUTO, 30.0f, true, 0));
   TEST_ASSERT_TRUE(dec3.mistOn);

   // ต่ำกว่า OFF threshold (28°C) → ต้องครบ MIST_CONFIRM_OFF_COUNT รอบ (=3) ถึงปิด
   mgr.update(makeInput(SystemMode::AUTO, 28.0f, true, 0));                     // รอบ 1
   mgr.update(makeInput(SystemMode::AUTO, 28.0f, true, 0));                     // รอบ 2
   FarmDecision dec4 = mgr.update(makeInput(SystemMode::AUTO, 28.0f, true, 0)); // รอบ 3 → off
   TEST_ASSERT_FALSE(dec4.mistOn);
}

// ทดสอบ: ค่าข้ามเส้น 1 รอบ → ยังไม่เปิด (MIST_CONFIRM_ON_COUNT=3)
void test_mist_confirm_single_spike_ignored(void)
{
   FarmManager mgr;

   // รอบเดียวที่ข้ามเส้น ON (temp>=32, hum<=65) → ต้องยังไม่เปิด
   FarmDecision dec = mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));
   TEST_ASSERT_FALSE(dec.mistOn);
}

// ทดสอบ: ค่าข้ามเส้นติดต่อกัน 3 รอบ → เปิด
void test_mist_confirm_opens_after_n_rounds(void)
{
   FarmManager mgr;

   // รอบ 1 และ 2 — ยังไม่เปิด
   mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));
   FarmDecision dec2 = mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));
   TEST_ASSERT_FALSE(dec2.mistOn);

   // รอบ 3 — ครบ confirm → เปิด
   FarmDecision dec3 = mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));
   TEST_ASSERT_TRUE(dec3.mistOn);
}

// ทดสอบ: ค่าข้ามเส้น 2 รอบ แล้วหาย 1 รอบ → reset ไม่เปิด
void test_mist_confirm_reset_on_break(void)
{
   FarmManager mgr;

   // รอบ 1, 2 — สะสม count
   mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));
   mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));

   // รอบ 3 — ค่ากลับเข้า deadband → reset count
   mgr.update(makeInput(SystemMode::AUTO, 30.0f, true, 0, 67.0f, true));

   // รอบ 4 — เริ่มนับใหม่ → ยังไม่เปิด
   FarmDecision dec = mgr.update(makeInput(SystemMode::AUTO, 33.0f, true, 0, 64.0f, true));
   TEST_ASSERT_FALSE(dec.mistOn);
}

void test_manual_overrides(void)
{
   FarmManager mgr;

   ManualOverrides m{};
   m.wantPumpOn = true;
   m.wantMistOn = false;

   FarmInput in = makeInput(SystemMode::MANUAL, 0.0f, false, 0);
   in.manual = m;

   FarmDecision dec = mgr.update(in);
   TEST_ASSERT_TRUE(dec.pumpOn);
   TEST_ASSERT_FALSE(dec.mistOn);
}

// ============================================================
//  ScheduledRelay tests — air pump ตาม TimeSchedule
// ============================================================

void test_scheduled_relay_in_window(void)
{
   TimeSchedule schedule;
   schedule.setEnabled(true);
   schedule.addWindow(60, 120); // 01:00–02:00

   MockActuator relay;
   ScheduledRelay sr(relay, schedule);

   sr.update(30); // ก่อน window → off
   TEST_ASSERT_FALSE(sr.isOn());

   sr.update(60); // start (รวม) → on
   TEST_ASSERT_TRUE(sr.isOn());

   sr.update(119); // ใน window → on
   TEST_ASSERT_TRUE(sr.isOn());

   sr.update(120); // end (ไม่รวม) → off
   TEST_ASSERT_FALSE(sr.isOn());
}

void test_scheduled_relay_disabled(void)
{
   TimeSchedule schedule;
   schedule.setEnabled(false);  // disabled → ปิดทุก window
   schedule.addWindow(0, 1439); // ครอบทั้งวัน

   MockActuator relay;
   ScheduledRelay sr(relay, schedule);

   sr.update(500);
   TEST_ASSERT_FALSE(sr.isOn()); // ต้องปิดเสมอเมื่อ disabled
}

void test_scheduled_relay_multiple_windows(void)
{
   TimeSchedule schedule;
   schedule.setEnabled(true);
   schedule.addWindow(60, 120);  // 01:00–02:00
   schedule.addWindow(300, 360); // 05:00–06:00

   MockActuator relay;
   ScheduledRelay sr(relay, schedule);

   sr.update(90);
   TEST_ASSERT_TRUE(sr.isOn());

   sr.update(200);
   TEST_ASSERT_FALSE(sr.isOn());

   sr.update(330);
   TEST_ASSERT_TRUE(sr.isOn());
}

// ============================================================

void setup()
{
   delay(2000);
   UNITY_BEGIN();

   // FarmManager
   RUN_TEST(test_idle_resets_actuators);
   RUN_TEST(test_auto_mist_temp_hysteresis);
   RUN_TEST(test_manual_overrides);

   // Mist Confirmation Counter
   RUN_TEST(test_mist_confirm_single_spike_ignored);
   RUN_TEST(test_mist_confirm_opens_after_n_rounds);
   RUN_TEST(test_mist_confirm_reset_on_break);

   // ScheduledRelay + TimeSchedule
   RUN_TEST(test_scheduled_relay_in_window);
   RUN_TEST(test_scheduled_relay_disabled);
   RUN_TEST(test_scheduled_relay_multiple_windows);

   UNITY_END();
}

void loop() {}