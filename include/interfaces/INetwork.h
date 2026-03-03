// include/interfaces/INetwork.h
#pragma once
#include <cstdint>

class INetwork
{
public:
    virtual ~INetwork() = default;

    // init / load config (e.g. wifi.json)
    virtual void begin() = 0;

    // legacy (blocking) - keep for compatibility / one-shot usage
    virtual bool ensureConnected(uint32_t timeoutMs) = 0;

    virtual bool isConnected() const = 0;

    // ปิดทุกอย่าง (STA+AP OFF หรือแล้วแต่ impl)
    virtual void disconnect() = 0;

    // config
    virtual bool hasValidConfig() const = 0;

    // ✅ AP-first: เปิด AP (offline-first) ผ่าน network layer
    virtual void startAp() = 0;

    // ✅ ตัดเฉพาะ STA แต่คง AP ไว้ (สำหรับกลับ AP โดยไม่สะดุด)
    virtual void disconnectStaOnly() = 0;

    // ===================== Production-grade (non-blocking STA) =====================
    // Kick off STA connection attempt without blocking.
    // Must NOT disable AP; should keep WIFI_AP_STA policy.
    virtual void startStaConnect() = 0;

    // Returns true when STA is connected.
    virtual bool pollStaConnected() const = 0;
};