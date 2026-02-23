#ifndef I_SENSOR_H
#define I_SENSOR_H

#include "Types.h"

class ISensor {
public:
    virtual ~ISensor() {}
    virtual bool begin() = 0;
    virtual SensorReading read() = 0;
    virtual const char* getName() const = 0; // เติม const ตามที่ตกลงกัน
};

#endif