// include/drivers/Esp32Ds18b20.h
#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../interfaces/Types.h"
#include "../Config.h"

// ============================================================
//  Esp32Ds18b20
//  Driver เซนเซอร์อุณหภูมิน้ำ DS18B20 ผ่าน 1-Wire
//  รองรับหลายตัวบน bus เดียวกัน (สูงสุด DS18B20_MAX_SENSORS)
//
//  วงจรต่อขา:
//    [DS18B20 VCC](1) ──(1)[3.3V]
//    [DS18B20 GND](2) ──(2)[GND]
//    [DS18B20 DATA](3)──(3)[GPIO4](3.1)──(3.1)[R4.7kΩ](3.2)──(3.2)[3V3]
//
//  ⚠️  ต้องมี external pull-up 4.7kΩ ที่ DATA — บังคับสำหรับ 1-Wire
//      สายยาว/หลายตัว: ปรับช่วง 2.2kΩ–4.7kΩ ตามผลทดสอบจริง
//  ⚠️  หลายตัวต่อ DATA bus เดียวกันได้ — R ค่อมครั้งเดียวที่ bus
//
//  ใช้งาน:
//    readAll()  — เรียกทุก loop เพื่ออ่านทุกตัวพร้อมกัน
//    get(i)     — ดึงผลของตัวที่ i
//    setLabel() — ตั้งชื่อ เช่น setLabel(0, "Tank-A")
// ============================================================

struct Ds18b20Reading
{
   uint8_t address[8] = {}; // ROM address ของตัวนี้ (unique ทุกตัว)
   float tempC = 0.0f;
   bool isValid = false;
   char label[16] = ""; // ชื่อที่กำหนดเอง default: "Tank-N"
};

class Esp32Ds18b20
{
public:
   Esp32Ds18b20(int pin, uint8_t maxSensors = DS18B20_MAX_SENSORS);

   // ค้นหา sensor บน bus + ตั้งค่าเริ่มต้น — เรียกครั้งเดียวตอน boot
   bool begin();

   // อ่านค่าทุกตัวพร้อมกัน — เรียกทุก loop ก่อน get()
   void readAll();

   uint8_t count() const;
   const Ds18b20Reading &get(uint8_t i) const;

   // ตั้งชื่อ sensor เช่น setLabel(0, "Tank-A")
   void setLabel(uint8_t i, const char *label);

private:
   OneWire _ow;
   DallasTemperature _dt;
   uint8_t _maxSensors;
   uint8_t _count = 0;

   // ใช้ DS18B20_MAX_SENSORS จาก Config.h — ไม่ hardcode
   Ds18b20Reading _readings[DS18B20_MAX_SENSORS];
};