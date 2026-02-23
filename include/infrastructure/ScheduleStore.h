#ifndef SCHEDULE_STORE_H
#define SCHEDULE_STORE_H

#include "../domain/AirPumpSchedule.h"

// โหลดตารางเวลา AirPump จากไฟล์ JSON (LittleFS)
// path ปกติคือ "/schedule.json"
bool loadAirScheduleFromFS(const char *path, AirPumpSchedule &out);

#endif