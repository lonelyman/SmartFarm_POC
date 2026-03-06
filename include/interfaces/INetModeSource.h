// include/interfaces/INetModeSource.h
#pragma once

#include <cstdint>

// ============================================================
//  INetModeSource — Interface สำหรับอ่าน NetDesiredMode
//
//  Implementation ที่มีอยู่:
//    - Esp32NetModeSwitch : toggle switch 2 ตำแหน่ง (hardware)
//
//  ใช้งาน:
//    - Esp32NetModeSwitch อยู่ใน SystemContext::netModeSource
//    - NetworkTask เรียก read() ทุก loop เพื่อตรวจ edge และส่ง NetCommand
// ============================================================

enum class NetDesiredMode : uint8_t
{
   AP_PRIMARY = 0,    // บังคับ AP เป็นหลัก
   STA_PREFERRED = 1, // อยากให้ลอง STA (ถ้ามี config)
};

class INetModeSource
{
public:
   virtual ~INetModeSource() = default;

   // init pins / resources — เรียกครั้งเดียวตอน boot
   virtual void begin() = 0;

   // อ่าน mode ปัจจุบัน — เรียกได้บ่อย มี debounce ใน implementation
   virtual NetDesiredMode read() = 0;
};