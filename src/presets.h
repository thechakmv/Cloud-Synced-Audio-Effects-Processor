#pragma once

#include "audio_chain.h"

// Resolve a comma-separated Spotify genre list to a preset.
// Falls through to a flat catch-all if nothing matches.
const DspParams &preset_lookup(const char *genre_csv);
