// include/interfaces/IModeSource.h
#pragma once

#include "Types.h"

// ============================================================
//  IModeSource — Interface สำหรับอ่าน SystemMode
//
//  Implementation ที่มีอยู่:
//    - Esp32ModeSwitchSource : rotary switch 3 ตำแหน่ง (hardware)
//
//  ใช้งาน:
//    - Esp32ModeSwitchSource อยู่ใน SystemContext::modeSource
//    - ControlTask เรียก readMode() ทุก loop เพื่ออัปเดต mode
// ============================================================

class IModeSource
{
public:
   virtual ~IModeSource() = default;

   // init pins / resources — เรียกครั้งเดียวตอน boot
   virtual void begin() = 0;

   // อ่าน mode ปัจจุบัน — เรียกได้บ่อย มี debounce ใน implementation
   virtual SystemMode readMode() = 0;
};