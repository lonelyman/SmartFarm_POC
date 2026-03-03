#pragma once
#include <stdint.h>

class INetwork
{
public:
    virtual ~INetwork() = default;

    virtual void begin() = 0;
    virtual bool ensureConnected(uint32_t timeoutMs) = 0;
    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;

    // ✅ เพิ่มอันนี้ (เพื่อให้ NetworkTask รู้ว่ามี config จริงไหม)
    virtual bool hasValidConfig() const = 0;
};