#include <Arduino.h>
#include "Config.h"
#include "application/FarmManager.h"
#include "infrastructure/SharedState.h"
#include "drivers/Esp32Bh1750Light.h"
#include "drivers/Esp32FakeTemperature.h"
#include "drivers/RtcDs3231Time.h"
#include "drivers/Esp32Relay.h"
#include "domain/AirPumpSchedule.h"

// ====== Global externs จาก main.cpp ======
extern SharedState state;
extern Esp32Bh1750Light lightSensor;
extern Esp32FakeTemperature tempSensor;
extern Esp32Relay waterPump;
extern Esp32Relay mistSystem;
extern Esp32Relay airPump;
extern FarmManager manager;
extern RtcDs3231Time rtcTime;

// helper: แปลงนาทีของวัน → HH:MM แล้วพิมพ์ log
static void logMinutesAsClock(const char *tag, uint16_t minutesOfDay)
{
	uint16_t hh = minutesOfDay / 60;
	uint16_t mm = minutesOfDay % 60;
	Serial.printf("[%s] %02u:%02u (minOfDay=%u)\n",
					  tag, hh, mm, minutesOfDay);
}

// ======================= InputTask =======================
void inputTask(void *pvParameters)
{
	while (true)
	{
		// 1) อ่านค่าแสง
		SensorReading lux = lightSensor.read();

		// 2) อ่าน temp ปลอมจาก driver
		SensorReading t = tempSensor.read();
		float tempValue = t.value;
		bool tempIsValid = t.isValid;

		uint32_t now = millis();

		// 3) อัปเดตค่าลง SharedState
		state.updateSensors(lux.value, lux.isValid, 0.0f, false, now);
		state.updateTemperature(tempValue, tempIsValid, now);

		// 4) LOG แยกตามหมวด
#if DEBUG_BH1750_LOG
		Serial.printf("[InputTask] BH1750 lux=%.1f\n", lux.value);
#endif

#if DEBUG_TEMP_LOG
		Serial.printf("[InputTask] TEMP=%.1f C (valid=%d)\n",
						  tempValue,
						  tempIsValid ? 1 : 0);
#endif

		vTaskDelay(pdMS_TO_TICKS(INPUT_TASK_INTERVAL_MS));
	}
}

// ======================= ControlTask =======================
void controlTask(void *pvParameters)
{
	Serial.println("ℹ️ ControlTask: Active");

	while (true)
	{
		// 1) ดึงเวลาเป็นนาทีของวันจาก RTC หรือ FAKE
		uint16_t minutesOfDay = 0;

#ifdef USE_FAKE_TIME
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

		// 2) ดึง snapshot จาก SharedState
		SystemStatus status = state.getSnapshot();
		ManualOverrides manual = state.getManualOverrides();

		// 3) DEBUG log สภาพรวม (เปิด/ปิดได้ด้วย DEBUG_CONTROL_LOG)
#if DEBUG_CONTROL_LOG
		Serial.printf(
			 "[ControlTask] mode=%d pump=%d mist=%d air=%d\n",
			 (int)status.mode,
			 waterPump.isOn() ? 1 : 0,
			 mistSystem.isOn() ? 1 : 0,
			 airPump.isOn() ? 1 : 0);
#endif

		// 4) ส่งเข้า FarmManager พร้อมเวลา + manual
		manager.update(status, manual, minutesOfDay);

		// 5) sync สถานะ actuator กลับเข้า SharedState
		state.updateActuators(
			 waterPump.isOn(),
			 mistSystem.isOn(),
			 airPump.isOn());

		vTaskDelay(pdMS_TO_TICKS(CONTROL_TASK_INTERVAL_MS));
	}
}