// src/tasks/SensorTasks.cpp

#include <Arduino.h>

#include "Config.h"

#include "tasks/TaskEntrypoints.h"

#include "infrastructure/SystemContext.h"
#include "infrastructure/SharedState.h"

#include "application/FarmManager.h"
#include "application/FarmModels.h"

// ---------- Helpers ----------

static void handleStaSwitchToNetCommand(SharedState *state, bool staSwitch)
{
	static bool stable = false;
	static bool lastRead = false;
	static uint32_t lastChangeMs = 0;
	static bool inited = false;

	if (!state)
		return;

	uint32_t now = millis();
	if (!inited)
	{
		stable = lastRead = staSwitch;
		lastChangeMs = now;
		inited = true;
		return;
	}

	if (staSwitch != lastRead)
	{
		lastRead = staSwitch;
		lastChangeMs = now;
		return;
	}

	if ((now - lastChangeMs) < NET_SWITCH_DEBOUNCE_MS)
		return; // debounce

	if (stable == lastRead)
		return;

	stable = lastRead;
	if (stable)
		state->requestNetOn();
	else
		state->requestNetOff();
}

static void logMinutesAsClock(const char *tag, uint16_t minutesOfDay)
{
	uint16_t hh = minutesOfDay / 60;
	uint16_t mm = minutesOfDay % 60;
	Serial.printf("[%s] %02u:%02u (minOfDay=%u)\n", tag, hh, mm, minutesOfDay);
}

static void updateModeFromSource(SystemContext &ctx, SystemStatus &status)
{
	if (!ctx.modeSource)
		return;

	SystemMode newMode = ctx.modeSource->readMode();
	if (newMode != status.mode)
	{
		ctx.state->setMode(newMode);
		status.mode = newMode;

		switch (newMode)
		{
		case SystemMode::AUTO:
			Serial.println("🎚 MODE SWITCH → AUTO");
			break;
		case SystemMode::MANUAL:
			Serial.println("🎚 MODE SWITCH → MANUAL");
			break;
		case SystemMode::IDLE:
		default:
			Serial.println("🎚 MODE SWITCH → IDLE");
			break;
		}
	}
}

static void updateWaterLevelAlarmLeds(const WaterLevelSensors &wl)
{
	static uint32_t lastToggleMs = 0;
	static bool ledPhase = false;

	const uint32_t now = millis();
	if (now - lastToggleMs >= 500) // กระพริบ 2Hz (500ms toggle)
	{
		lastToggleMs = now;
		ledPhase = !ledPhase;
	}

	// CH1
	if (wl.ch1Low)
		digitalWrite(PIN_WATER_LEVEL_CH1_ALARM_LED, ledPhase ? HIGH : LOW);
	else
		digitalWrite(PIN_WATER_LEVEL_CH1_ALARM_LED, LOW);

	// CH2
	if (wl.ch2Low)
		digitalWrite(PIN_WATER_LEVEL_CH2_ALARM_LED, ledPhase ? HIGH : LOW);
	else
		digitalWrite(PIN_WATER_LEVEL_CH2_ALARM_LED, LOW);
}

// ---------- Tasks ----------

void inputTask(void *pvParameters)
{
	auto *ctx = static_cast<SystemContext *>(pvParameters);
	if (ctx == nullptr || ctx->state == nullptr)
	{
		vTaskDelete(nullptr);
		return;
	}

	Serial.println("ℹ️ InputTask: Active");

	while (true)
	{
		// Read sensors
		SensorReading lux = ctx->lightSensor->read();
		SensorReading t = ctx->tempSensor->read();

		uint32_t now = millis();

		// Push to SharedState
		ctx->state->updateSensors(lux.value, lux.isValid, 0.0f, false, now);
		ctx->state->updateTemperature(t.value, t.isValid, now);

		ctx->state->updateHumidity(
			 ctx->tempSensor->getLastHumidity(),
			 ctx->tempSensor->isHumidityValid(),
			 now);
		// Water level sensors
		if (ctx->waterLevelInput)
		{
			auto wl = ctx->waterLevelInput->read();
			ctx->state->updateWaterLevelSensors(wl.ch1Low, wl.ch2Low, now);

#if DEBUG_WATER_LEVEL_LOG
			Serial.printf("[WLVL] ch1Low=%d ch2Low=%d\n", wl.ch1Low ? 1 : 0, wl.ch2Low ? 1 : 0);
#endif
		}

#if DEBUG_BH1750_LOG
		Serial.printf("[InputTask] BH1750 lux=%.1f valid=%d\n", lux.value, lux.isValid ? 1 : 0);
#endif
#if DEBUG_TEMP_LOG
		Serial.printf("[InputTask] Temp=%.2f valid=%d\n", t.value, t.isValid ? 1 : 0);
#endif

		// --- Network switch -> NetCommand ---
		bool staSwitch = (digitalRead(PIN_SW_NET_AP) == HIGH);
		handleStaSwitchToNetCommand(ctx->state, staSwitch);

		vTaskDelay(pdMS_TO_TICKS(INPUT_TASK_INTERVAL_MS));
	}
}

void controlTask(void *pvParameters)
{
	auto *ctx = static_cast<SystemContext *>(pvParameters);
	if (ctx == nullptr || ctx->state == nullptr)
	{
		vTaskDelete(nullptr);
		return;
	}

	Serial.println("ℹ️ ControlTask: Active");

	while (true)
	{
		// 0) Apply pending time-set request (CommandTask -> SharedState -> ControlTask -> IClock)
		if (ctx->clock)
		{
			TimeSetRequest req{};
			if (ctx->state->consumeSetClockTime(req))
			{
				if (ctx->clock->setTimeOfDay(req.hour, req.minute, req.second))
				{
					Serial.printf("[CLOCK] time set applied: %02u:%02u:%02u\n",
									  req.hour, req.minute, req.second);
				}
				else
				{
					Serial.println("[CLOCK] failed to apply time set (clock unsupported?)");
				}
			}
		}

		// 1) Read time (IClock)
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

		// 2) Read snapshot + manual overrides
		SystemStatus status = ctx->state->getSnapshot();
		ManualOverrides manual = ctx->state->getManualOverrides();

		updateWaterLevelAlarmLeds(status.waterLevelSensors);

		// 3) Update mode from source (adapter) -> sync to SharedState (inside function)
		updateModeFromSource(*ctx, status);

#if DEBUG_CONTROL_LOG
		Serial.printf(
			 "[ControlTask] mode=%d pump=%d mist=%d air=%d\n",
			 (int)status.mode,
			 ctx->waterPump->isOn() ? 1 : 0,
			 ctx->mistSystem->isOn() ? 1 : 0,
			 ctx->airPump->isOn() ? 1 : 0);
#endif

		// 4) Map to FarmInput (pure application model)
		FarmInput in{};
		in.mode = status.mode;
		in.minutesOfDay = minutesOfDay;
		in.manual = manual;

		// ใช้เฉพาะข้อมูลที่ logic ต้องใช้ (ลด coupling)
		in.temperatureC = status.temperature.value;
		in.temperatureValid = status.temperature.isValid;

		in.humidityRH = status.humidity.value;
		in.humidityValid = status.humidity.isValid;

		// 5) Decide (Application logic, pure)
		FarmDecision decision = ctx->manager->update(in);

		// 6) Apply to hardware (ONLY place touching relays in control path)
		if (decision.pumpOn)
			ctx->waterPump->turnOn();
		else
			ctx->waterPump->turnOff();

		if (decision.mistOn)
			ctx->mistSystem->turnOn();
		else
			ctx->mistSystem->turnOff();

		if (decision.airOn)
			ctx->airPump->turnOn();
		else
			ctx->airPump->turnOff();

		// 7) Sync actuator states back to SharedState
		ctx->state->updateActuators(
			 ctx->waterPump->isOn(),
			 ctx->mistSystem->isOn(),
			 ctx->airPump->isOn());

		vTaskDelay(pdMS_TO_TICKS(CONTROL_TASK_INTERVAL_MS));
	}
}