// include/interfaces/INetwork.h
#pragma once

#include <cstdint>

// ============================================================
//  INetwork — Interface สำหรับ WiFi / Network layer
//
//  Implementation: Esp32WiFiNetwork
//
//  Design: AP-first (offline-first)
//    - boot → startAp() เสมอ — ไม่ต้องมี WiFi ก็ใช้งานได้
//    - ถ้ามี config → startStaConnect() เพิ่ม STA ทีหลัง (non-blocking)
//    - AP ยังเปิดอยู่ตลอดแม้ STA connected (WIFI_AP_STA)
// ============================================================

class INetwork
{
public:
    virtual ~INetwork() = default;

    // โหลด config จาก store (wifi.json) — เรียกครั้งเดียวตอน boot
    virtual void begin() = 0;

    // blocking connect — ใช้สำหรับ one-shot หรือ NTP sync
    virtual bool ensureConnected(uint32_t timeoutMs) = 0;

    virtual bool isConnected() const = 0;
    virtual bool hasValidConfig() const = 0;

    // ปิดทั้ง STA+AP
    virtual void disconnect() = 0;

    // เปิด AP (offline-first boot)
    virtual void startAp() = 0;

    // ตัดเฉพาะ STA คง AP ไว้ — ใช้ตอน STA failed / user สั่ง NetOff
    virtual void disconnectStaOnly() = 0;

    // non-blocking STA — kick off แล้ว poll ทีหลัง
    // ต้องไม่ปิด AP (คง WIFI_AP_STA ไว้)
    virtual void startStaConnect() = 0;
    virtual bool pollStaConnected() const = 0;
};