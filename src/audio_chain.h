#pragma once
#include <Arduino.h>

#include "config.h"
//Three Band EQ, although in current state only using low and high dB shel EQ
//`low_db` low shelf gain at 250Hz
//`high_db` high shelf gain at 3kHz
struct DspParams{
    float low_db;
    float mid_db;
    float high_db;
};

namespace audio_chain { //namespace means you call audiochain::begin() for example
    
    bool begin();

    void apply(const DspParams &p);
    
    const DspParams &current();
}