// include/domain/TimeWindow.h
#pragma once

#include <stdint.h>

// ============================================================
//  TimeWindow — ช่วงเวลา 1 รอบในรูปแบบนาทีของวัน
//
//  range: [startMin, endMin) — รวม start ไม่รวม end
//  ตัวอย่าง: 07:00–12:00 = startMin=420, endMin=720
// ============================================================

struct TimeWindow
{
   uint16_t startMin = 0; // 0–1439
   uint16_t endMin = 0;   // 0–1439
};