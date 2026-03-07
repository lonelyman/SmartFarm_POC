// include/infrastructure/NetTimeSync.h
#pragma once

class RtcDs3231Time;

// ============================================================
//  NetTimeSync — Sync เวลาจาก NTP → RTC DS3231
//
//  ⚠️  WiFi ต้อง connected ก่อนแล้ว — NetworkTask จัดการ
//  ใช้งาน:
//    NetworkTask เรียก NetTimeSync::syncRtcFromNtp(rtc)
//    หลังจาก STA connected สำเร็จ
// ============================================================

namespace NetTimeSync
{
   bool syncRtcFromNtp(RtcDs3231Time &rtc);
}