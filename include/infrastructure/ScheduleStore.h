// include/infrastructure/ScheduleStore.h
#pragma once

#include "interfaces/ISchedule.h"

// ============================================================
//  ScheduleStore — โหลดตารางเวลาจาก LittleFS → ISchedule
//
//  ใช้งาน:
//    AppBoot เรียก loadScheduleFromFS("/schedule.json", "air_pump", airSchedule)
//    ถ้าโหลดไม่สำเร็จ → schedule จะ disabled อัตโนมัติ
//
//  Format JSON (/schedule.json):
//    {
//      "air_pump": {
//        "enabled": true,
//        "windows": [
//          { "start": "07:00", "end": "12:00" },
//          { "start": "14:00", "end": "17:30" }
//        ]
//      }
//    }
//
//  ⚠️  LittleFS.begin() ต้องถูกเรียกก่อนแล้ว (AppBoot จัดการ)
//  ✅  รองรับหลาย schedule ในไฟล์เดียว — ระบุด้วย key
// ============================================================

bool loadScheduleFromFS(const char *path, const char *key, ISchedule &out);