#pragma once
#include <stdint.h>

class INetwork {
public:
    virtual ~INetwork() = default;

    // init wifi stack (does not have to connect)
    virtual void begin() = 0;

    // connect if needed, wait until connected or timeout
    virtual bool ensureConnected(uint32_t timeoutMs) = 0;

    virtual bool isConnected() const = 0;

    virtual void disconnect() = 0;
};