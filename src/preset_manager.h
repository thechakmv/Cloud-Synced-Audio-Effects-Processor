#pragma once

#include <Arduino.h>

#include "audio_chain.h"

namespace presets{
    void begin();

    void on_genre(const char *genre_csv);

    void apply_ui_override(const DspParams & params);
    void clear_ui_override();

    const char *last_genre();

    bool ui_override_active();
}
