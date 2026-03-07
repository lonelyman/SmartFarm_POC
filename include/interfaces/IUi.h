// include/interfaces/IUi.h
#pragma once

// ============================================================
//  IUi — Interface สำหรับ Web UI / Display layer
//
//  Implementation: Esp32WebUi (WebServer บน ESP32-S3)
// ============================================================

class IUi
{
public:
   virtual ~IUi() = default;

   // เรียกครั้งเดียวตอน boot
   virtual bool begin() = 0;

   // เรียกถี่ๆ ใน task (ทำงาน 1 รอบ)
   virtual void tick() = 0;
};