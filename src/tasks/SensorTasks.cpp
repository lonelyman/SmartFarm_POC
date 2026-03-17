// src/tasks/SensorTasks.cpp

#include <Arduino.h>

#include "Config.h"

#include "tasks/TaskEntrypoints.h"

#include "infrastructure/SystemContext.h"
#include "infrastructure/SharedState.h"

#include "application/FarmManager.h"
#include "application/FarmModels.h"
#include "application/ScheduledRelay.h"

// ============================================================
//  Helpers
// ============================================================

static void logMinutesAsClock(const char *tag, uint16_t minutesOfDay)
{
	const uint16_t hh = minutesOfDay / 60;
	const uint16_t mm = minutesOfDay % 60;
	Serial.printf("[%s] %02u:%02u (minOfDay=%u)\n", tag, hh, mm, minutesOfDay);
}

// อ่าน mode จาก physical switch แล้วอัปเดต SharedState เมื่อเปลี่ยน
// เรียกจาก inputTask เท่านั้น — hardware อ่านได้ที่เดียว
static void updateModeFromSource(SystemContext &ctx)
{
	if (!ctx.modeSource)
		return;

	const SystemMode newMode = ctx.modeSource->readMode();
	if (newMode == ctx.state->getMode())
		return;

	ctx.state->setMode(newMode);

	switch (newMode)
	{
	case SystemMode::AUTO:
		Serial.println("🎚🟢 MODE SWITCH → AUTO");
		break;
	case SystemMode::MANUAL:
		Serial.println("🎚🟡 MODE SWITCH → MANUAL");
		break;
	case SystemMode::IDLE:
	default:
		Serial.println("🎚⚪ MODE SWITCH → IDLE");
		break;
	}
}

// กระพริบ LED เตือนเมื่อน้ำต่ำ — blink 500ms ต่อครั้ง
static void updateWaterLevelAlarmLeds(const WaterLevelSensors &wl)
{
	static uint32_t lastToggleMs = 0;
	static bool ledPhase = false;

	const uint32_t now = millis();
	if (now - lastToggleMs >= 500)
	{
		lastToggleMs = now;
		ledPhase = !ledPhase;
	}

	const uint8_t LED_ON = ALARM_LED_ACTIVE_HIGH ? HIGH : LOW;
	const uint8_t LED_OFF = ALARM_LED_ACTIVE_HIGH ? LOW : HIGH;

	digitalWrite(PIN_WATER_LEVEL_CH1_ALARM_LED, wl.ch1Low ? (ledPhase ? LED_ON : LED_OFF) : LED_OFF);
	digitalWrite(PIN_WATER_LEVEL_CH2_ALARM_LED, wl.ch2Low ? (ledPhase ? LED_ON : LED_OFF) : LED_OFF);
}

// ============================================================
//  inputTask — อ่าน sensor และ switch ทุก INPUT_TASK_INTERVAL_MS
//  Core 1 | Priority 1
// ============================================================

void inputTask(void *pvParameters)
{
	auto *ctx = static_cast<SystemContext *>(pvParameters);
	if (!ctx || !ctx->state)
	{
		vTaskDelete(nullptr);
		return;
	}

	Serial.println("ℹ️ InputTask: Active");

	while (true)
	{
		const uint32_t now = millis();

		// --- Sensors ---
		const SensorReading lux = ctx->lightSensor->read();
		const SensorReading t = ctx->tempSensor->read();

		ctx->state->updateSensors(lux.value, lux.isValid, 0.0f, false, now);
		ctx->state->updateTemperature(t.value, t.isValid, now);
		ctx->state->updateHumidity(ctx->tempSensor->getLastHumidity(), ctx->tempSensor->isHumidityValid(), now);

		// --- Water Temperature (DS18B20) ---
		if (ctx->waterTempSensor && ctx->waterTempSensor->count() > 0)
		{
			ctx->waterTempSensor->readAll();

			WaterTempReading readings[MAX_WATER_TEMP_SENSORS];
			for (uint8_t i = 0; i < ctx->waterTempSensor->count(); i++)
			{
				const auto &r = ctx->waterTempSensor->get(i);
				readings[i].tempC = r.tempC;
				readings[i].isValid = r.isValid;
				strncpy(readings[i].label, r.label, sizeof(readings[i].label));
			}
			ctx->state->updateWaterTemps(readings, ctx->waterTempSensor->count());
		}

		// --- Mode Switch (physical) → SharedState ---
		updateModeFromSource(*ctx);

		// --- Manual Switches → SharedState (เฉพาะ MANUAL mode) ---
		if (ctx->swManualPump && ctx->swManualMist && ctx->swManualAir)
		{
			if (ctx->state->getMode() == SystemMode::MANUAL)
			{
				ManualOverrides sw{};
				sw.wantPumpOn = ctx->swManualPump->isOn();
				sw.wantMistOn = ctx->swManualMist->isOn();
				sw.wantAirOn = ctx->swManualAir->isOn();
				ctx->state->setManualOverrides(sw);
			}
		}

		// --- Water Level Sensors ---
		if (ctx->waterLevelInput)
		{
			const auto wl = ctx->waterLevelInput->read();
			ctx->state->updateWaterLevelSensors(wl.ch1Low, wl.ch2Low, now);

#if DEBUG_WATER_LEVEL_LOG
			Serial.printf("[WLVL] ch1Low=%d ch2Low=%d\n", wl.ch1Low ? 1 : 0, wl.ch2Low ? 1 : 0);
#endif
		}

#if DEBUG_BH1750_LOG
		Serial.printf("[InputTask] lux=%.1f valid=%d\n", lux.value, lux.isValid ? 1 : 0);
#endif
#if DEBUG_TEMP_LOG
		Serial.printf("[InputTask] temp=%.2f hum=%.1f valid=%d\n", t.value, ctx->tempSensor->getLastHumidity(), t.isValid ? 1 : 0);
#endif
		vTaskDelay(pdMS_TO_TICKS(INPUT_TASK_INTERVAL_MS));
	}
}

// ============================================================
//  controlTask — ตัดสินใจ และสั่ง actuator ทุก CONTROL_TASK_INTERVAL_MS
//  Core 1 | Priority 2
// ============================================================

void controlTask(void *pvParameters)
{
	auto *ctx = static_cast<SystemContext *>(pvParameters);
	if (!ctx || !ctx->state)
	{
		vTaskDelete(nullptr);
		return;
	}

	Serial.println("ℹ️ ControlTask: Active");

	while (true)
	{
		// 0) Apply pending time-set request จาก CommandTask / WebUI
		if (ctx->clock)
		{
			TimeSetRequest req{};
			if (ctx->state->consumeSetClockTime(req))
			{
				if (ctx->clock->setTimeOfDay(req.hour, req.minute, req.second))
					Serial.printf("[CLOCK] time set: %02u:%02u:%02u\n", req.hour, req.minute, req.second);
				else
					Serial.println("[CLOCK] time set failed");
			}
		}

		// 1) Read time
		uint16_t minutesOfDay = 0;
		if (!ctx->clock || !ctx->clock->getMinutesOfDay(minutesOfDay))
		{
			minutesOfDay = 0;
#if DEBUG_TIME_LOG
			Serial.println("[CLOCK] read failed, use 0");
#endif
		}
#if DEBUG_TIME_LOG
		logMinutesAsClock("TIME", minutesOfDay);
#endif

		// 2) Read current state
		SystemStatus status = ctx->state->getSnapshot();
		ManualOverrides manual = ctx->state->getManualOverrides();

		// 3) Update alarm LEDs ตาม water level
		updateWaterLevelAlarmLeds(status.waterLevelSensors);

		// 4) mode มาจาก getSnapshot() ด้านบนแล้ว — physical switch อ่านใน inputTask

		// 5) Map snapshot → FarmInput
		FarmInput in{};
		in.mode = status.mode;
		in.minutesOfDay = minutesOfDay;
		in.manual = manual;
		in.temperatureC = status.temperature.value;
		in.temperatureValid = status.temperature.isValid;
		in.humidityRH = status.humidity.value;
		in.humidityValid = status.humidity.isValid;

		// 6) FarmManager ตัดสินใจ (pump + mist เท่านั้น)
		const FarmDecision decision = ctx->manager->update(in);

		// 7) Apply decision → hardware
		decision.pumpOn ? ctx->waterPump->turnOn() : ctx->waterPump->turnOff();
		decision.mistOn ? ctx->mistSystem->turnOn() : ctx->mistSystem->turnOff();

		// 8) Air pump — ScheduledRelay เป็นเจ้าของ ไม่ผ่าน FarmDecision
		if (ctx->scheduledAirPump)
			ctx->scheduledAirPump->update(minutesOfDay);

		// 9) Sync actuator state กลับ SharedState (ให้ WebUI / API อ่านได้)
		ctx->state->updateActuators(ctx->waterPump->isOn(), ctx->mistSystem->isOn(), ctx->airPump->isOn());

		// 10) Log สถานะหลัง sync — ข้อมูลตรงกับรอบนี้แน่นอน
#if DEBUG_CONTROL_LOG
		Serial.printf("[ControlTask] mode=%d pump=%d mist=%d air=%d\n", (int)status.mode, ctx->waterPump->isOn() ? 1 : 0, ctx->mistSystem->isOn() ? 1 : 0, ctx->airPump->isOn() ? 1 : 0);
#endif
		vTaskDelay(pdMS_TO_TICKS(CONTROL_TASK_INTERVAL_MS));
	}
}