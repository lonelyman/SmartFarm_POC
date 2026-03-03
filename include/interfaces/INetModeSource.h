#pragma once
#include <cstdint>

enum class NetDesiredMode : uint8_t
{
   AP_PRIMARY = 0,   // บังคับ AP เป็นหลัก
   STA_PREFERRED = 1 // อยากให้ลอง STA (ถ้ามี config)
};

class INetModeSource
{
public:
   virtual ~INetModeSource() = default;
   virtual void begin() = 0;
   virtual NetDesiredMode read() = 0;
};