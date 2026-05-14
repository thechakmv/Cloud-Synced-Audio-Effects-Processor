#pragma once

#include <Arduino.h>

namespace spotify{
    bool begin();

    void poll();

    const char *current_track_id();
}