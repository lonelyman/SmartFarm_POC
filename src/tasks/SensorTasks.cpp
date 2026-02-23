#include <Arduino.h>
#include "Config.h"
#include "infrastructure/SharedState.h"
#include "interfaces/Types.h"
#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/RtcDs3231Time.h"
#include "drivers/Esp32Relay.h"
#include "application/FarmManager.h"

// ======= Global Objects ที่มาจาก main.cpp =======
extern SharedState state;
extern Esp32Bh1750Light lightSensor;
extern Esp32FakeTemperature tempSensor;
extern Esp32Relay waterPump;
extern Esp32Relay mistSystem;
extern Esp32Relay airPump;
extern FarmManager manager;
extern RtcDs3231Time rtcTime;

// helper ล่วงหน้า
static uint16_t resolveMinutesOfDay();
static void logMinutesAsClock(const char *tag, uint16_t minutesOfDay);
// ---------- extern globals ----------
extern SharedState state;
extern Esp32Bh1750Light lightSensor;
extern Esp32FakeTemperature tempSensor;
extern Esp32Relay waterPump;
extern Esp32Relay mistSystem;
extern Esp32Relay airPump;
extern FarmManager manager;
extern RtcDs3231Time rtcTime;

// ---------- helper: อ่านสวิตช์โหมด ----------
static void updateModeFromSwitch()
{
	// ใช้ INPUT_PULLUP → LOW = กดอยู่, HIGH = ปล่อย
	int a = digitalRead(PIN_SW_MODE_A);
	int b = digitalRead(PIN_SW_MODE_B);

	bool aActive = (a == LOW);
	bool bActive = (b == LOW);

	SystemMode newMode;
	bool hasNewMode = true;

	if (aActive && !bActive)
	{
		newMode = SystemMode::AUTO;
	}
	else if (!aActive && bActive)
	{
		newMode = SystemMode::MANUAL;
	}
	else if (!aActive && !bActive)
	{
		newMode = SystemMode::IDLE;
	}
	else
	{
		// aActive && bActive == สวิตช์ลัดวงจรสองทางพร้อมกัน (ผิดปกติ)
		hasNewMode = false;
	}

	if (!hasNewMode)
	{
		// ไม่เปลี่ยนโหมด
		return;
	}

	// ดูโหมดปัจจุบันก่อน ถ้าเหมือนเดิมก็ไม่ต้อง set ซ้ำ
	SystemStatus snap = state.getSnapshot();
	if (snap.mode == newMode)
	{
		return;
	}

	state.setMode(newMode);

	switch (newMode)
	{
	case SystemMode::AUTO:
		Serial.println("🎚 MODE SWITCH → AUTO (จากสวิตช์หน้าเครื่อง)");
		break;
	case SystemMode::MANUAL:
		Serial.println("🎚 MODE SWITCH → MANUAL (จากสวิตช์หน้าเครื่อง)");
		break;
	case SystemMode::IDLE:
		// เข้า IDLE = ปิดทุกอย่างด้วย เพื่อความปลอดภัย
		waterPump.turnOff();
		mistSystem.turnOff();
		airPump.turnOff();
		state.updateActuators(false, false, false);
		Serial.println("🎚 MODE SWITCH → IDLE (ALL OFF)");
		break;
	}
}
// แปลงสถานะสวิตช์ A/B → SystemMode
static SystemMode readModeFromSwitch()
{
	int swA = digitalRead(PIN_SW_MODE_A); // ใช้ INPUT_PULLUP → LOW = ถูกเลือก
	int swB = digitalRead(PIN_SW_MODE_B);

	// กำหนดแมป:
	// A=LOW,  B=HIGH → AUTO
	// A=HIGH, B=LOW  → MANUAL
	// อย่างอื่น (ปกติ HIGH,HIGH = กลาง) → IDLE (โหมดพัก)
	if (swA == LOW && swB == HIGH)
	{
		return SystemMode::AUTO;
	}
	else if (swA == HIGH && swB == LOW)
	{
		return SystemMode::MANUAL;
	}
	else
	{
		// ปลอดภัยสุด: ถ้าลายสวิตช์แปลก ๆ → IDLE
		return SystemMode::IDLE;
	}
}

// ======================= InputTask =======================
void inputTask(void *pvParameters)
{
	while (true)
	{
		// 1) อ่านค่าแสง
		SensorReading lux = lightSensor.read();

		// 2) อ่านอุณหภูมิจากเซนเซอร์ปลอม (แทนของจริงไปก่อน)
		SensorReading temp = tempSensor.read();

		uint32_t now = millis();

		// 3) อัปเดตค่าลง SharedState
		state.updateSensors(lux.value, lux.isValid, 0.0f, false, now);
		state.updateTemperature(temp.value, temp.isValid, now);

		// 4) LOG แยกตามหมวด เปิด/ปิดด้วย DEBUG_* ทีละตัว
#if DEBUG_BH1750_LOG
		Serial.printf("[InputTask] BH1750 lux=%.1f\n", lux.value);
#endif

#if DEBUG_TEMP_LOG
		Serial.printf("[InputTask] TEMP=%.1f C (valid=%d)\n",
						  temp.value,
						  temp.isValid ? 1 : 0);
#endif

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

// ======================= ControlTask =======================
void controlTask(void *pvParameters)
{
	Serial.println("ℹ️ ControlTask: Active");
	while (true)
	{
		updateModeFromSwitch();
		// 1) ดึง "นาทีของวัน"
		uint16_t minutesOfDay = resolveMinutesOfDay();

		// 2) ดึง snapshot จาก SharedState
		SystemStatus status = state.getSnapshot();

		// 3) อ่านสวิตช์ แล้วบังคับโหมดตาม hardware
		SystemMode swMode = readModeFromSwitch();
		if (swMode != status.mode)
		{
			state.setMode(swMode); // อัปเดตเข้า SharedState
			status.mode = swMode;  // sync ค่า local ที่ใช้ส่งเข้า manager

			Serial.print("[SWITCH] MODE -> ");
			switch (swMode)
			{
			case SystemMode::AUTO:
				Serial.println("AUTO");
				break;
			case SystemMode::MANUAL:
				Serial.println("MANUAL");
				break;
			case SystemMode::IDLE:
				Serial.println("IDLE");
				break;
			}
		}

		// 3) DEBUG log สภาพรวม (เปิด/ปิดได้ด้วย DEBUG_CONTROL_LOG)
#if DEBUG_CONTROL_LOG
		Serial.printf(
			 "[ControlTask] mode=%d pump=%d mist=%d air=%d\n",
			 (int)status.mode,
			 waterPump.isOn() ? 1 : 0,
			 mistSystem.isOn() ? 1 : 0,
			 airPump.isOn() ? 1 : 0);
#endif

		// 4) ส่งเข้า FarmManager พร้อมเวลา
		manager.update(status, minutesOfDay);

		// 5) sync สถานะ actuator กลับเข้า SharedState
		state.updateActuators(
			 waterPump.isOn(),
			 mistSystem.isOn(),
			 airPump.isOn());

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

// ======================= helper: เวลาของวัน =======================

static uint16_t resolveMinutesOfDay()
{
	uint16_t minutesOfDay = 0;

#ifdef USE_FAKE_TIME
	// ใช้เวลาปลอมจาก Config.h เช่น 07:30 => 450
	minutesOfDay = FAKE_MINUTES_OF_DAY;
#if DEBUG_TIME_LOG
	logMinutesAsClock("TIME", minutesOfDay);
#endif
#else
	if (rtcTime.getMinutesOfDay(minutesOfDay))
	{
#if DEBUG_TIME_LOG
		logMinutesAsClock("RTC", minutesOfDay);
#endif
	}
	else
	{
		minutesOfDay = 0;
#if DEBUG_TIME_LOG
		Serial.println("[RTC] read failed, use 0");
#endif
	}
#endif

	return minutesOfDay;
}

static void logMinutesAsClock(const char *tag, uint16_t minutesOfDay)
{
	uint16_t hh = minutesOfDay / 60;
	uint16_t mm = minutesOfDay % 60;
	Serial.printf("[%s] %02u:%02u (minOfDay=%u)\n",
					  tag, hh, mm, minutesOfDay);
}