// include/domain/TimeSchedule.h
#pragma once

#include "interfaces/ISchedule.h"
#include "domain/TimeWindow.h"

// ============================================================
//  TimeSchedule — ตารางเวลาแบบ window list
//
//  Pure domain logic — ไม่มี Arduino API ไม่มี filesystem
//  ทดสอบได้โดยไม่ต้องมี hardware
//
//  ใช้งาน:
//    TimeSchedule s;
//    s.setEnabled(true);
//    s.addWindow(420, 720);   // 07:00–12:00
//    s.isInWindow(480);       // → true (08:00)
//    s.isInWindow(800);       // → false
// ============================================================

class TimeSchedule : public ISchedule
{
public:
   static constexpr uint8_t MAX_WINDOWS = 24;

   bool isInWindow(uint16_t minutesOfDay) const override;
   void addWindow(uint16_t startMin, uint16_t endMin) override;
   void clear() override;
   bool isEnabled() const override;
   void setEnabled(bool enabled) override;

   uint8_t windowCount() const;

private:
   TimeWindow _windows[MAX_WINDOWS];
   uint8_t _count = 0;
   bool _enabled = false;
};