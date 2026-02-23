#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "../interfaces/Types.h"

struct SystemStatus {
    SensorReading light;
    SensorReading ec;
    
    bool isPumpActive = false;
    bool isMistActive = false;
    
    SystemMode mode = SystemMode::MANUAL;
};

#endif